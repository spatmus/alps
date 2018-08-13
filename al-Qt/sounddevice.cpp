#include "sounddevice.h"
#include <QDebug>
#include <QAudioDeviceInfo>

QString SoundDevice::error() const
{
    return m_error;
}

SoundDevice::SoundDevice(Synchro &syn, SoundData &data) : synchro(syn), sd(data)
{

}

bool SoundDevice::start()
{
    open(QIODevice::ReadWrite);
    m_inp->start(this);
    m_outp->start(this);

    if (m_outp->state() == QAudio::StoppedState)
    {
        m_error.sprintf("in %d out %d", m_inp->state(), m_outp->state());
        return false;
    }
    return true;
}

void SoundDevice::stop()
{
    m_inp->stop();
    m_outp->stop();
    close();
}

bool SoundDevice::selectDevices(QString adc, QString dac, int inputs, int outputs, int sampling)
{
    m_error = "No error";

    QAudioFormat format;
    // Set up the format
    format.setSampleRate(sampling);
    format.setChannelCount(m_inputs = inputs);
    format.setSampleSize(32);
    format.setCodec("audio/pcm");
    format.setByteOrder(QAudioFormat::LittleEndian);
    format.setSampleType(QAudioFormat::Float);

    QAudioDeviceInfo di;

    QList<QAudioDeviceInfo> allIn = QAudioDeviceInfo::availableDevices(QAudio::Mode::AudioInput);
    if(select(adc, allIn, di))
    {
        m_adc = di.deviceName();
        emit info(m_adc);
        m_inp = new QAudioInput(di, format, this);
    }

    QList<QAudioDeviceInfo> allOut = QAudioDeviceInfo::availableDevices(QAudio::Mode::AudioOutput);
    if (select(dac, allOut, di))
    {
        m_dac = di.deviceName();
        emit info(m_dac);
        format.setChannelCount(m_outputs = outputs);
        m_outp = new QAudioOutput(di, format, this);
    }

    if (!m_inp || !m_outp)
    {
        if (!m_inp)
        {
            m_error = "Failed to open " + m_adc;
        }
        else if (!m_outp)
        {
            m_error = "Failed to open " + m_dac;
        }
        delete m_inp;
        delete m_outp;
        m_inp = nullptr;
        m_outp = nullptr;
    }
    else
    {
        connect(m_inp, SIGNAL(stateChanged(QAudio::State)), this, SLOT(handleInStateChanged(QAudio::State)));
        connect(m_outp, SIGNAL(stateChanged(QAudio::State)), this, SLOT(handleOutStateChanged(QAudio::State)));
        return true;
    }

    return false;
}

bool SoundDevice::select(QString name, QList<QAudioDeviceInfo> &all, QAudioDeviceInfo &dinfo)
{
    QString dname;
    for (QAudioDeviceInfo di : all)
    {
        qDebug() << di.deviceName();
        if (di.deviceName().indexOf(name) >= 0)
        {
            if (!dname.isEmpty())
            {
                if (di.deviceName() != dname)
                {
                    m_error = "Multiple output devices matching " + name;
                    break;
                }
                continue; // the same name repeated
            }
            dname = di.deviceName();
            dinfo = di;
        }
    }

    return !dname.isEmpty();
}

void SoundDevice::handleOutStateChanged(QAudio::State newState)
{
    emit info("Audio output state " + QString::number(newState));

}

void SoundDevice::handleInStateChanged(QAudio::State newState)
{
    emit info("Audio input state " + QString::number(newState));
    switch (newState) {
    case QAudio::StoppedState:
        if (m_inp->error() != QAudio::NoError)
        {
            emit info("Audio input error " + QString::number(m_inp->error()));
            // Error handling
        } else {
            emit info("Stopped recording");
            // Finished recording
        }
        break;

    case QAudio::ActiveState:
        // Started recording - read from IO device
        emit info("Starting recording");

        break;

    default:
        // ... other cases as appropriate
        break;
    }
}

qint64 SoundDevice::readData(char *data, qint64 maxlen)
{
    int ptr = synchro.getAudioPtr();
    long pOut = ptr * sd.channels;
    long long frames = sd.frames - ptr;
    if (frames > maxlen)
    {
        frames = maxlen;
    }
    for (qint64 i = 0; i < frames; i += sizeof (float))
    {
        *(float*)(data + i) = sd.ping.data()[i + pOut];
    }
    /*

    if (ptr < sd.frames)
    {
        memcpy(data, sd.ping.data() + pOut, sizeof(float) * frames * sd.channels);
        return maxlen;
    }
    else
    {
        memset(data, 0, maxlen);
    }
    */
    return maxlen;
}

qint64 SoundDevice::writeData(const char *data, qint64 len)
{
    int ptr = synchro.getAudioPtr();
    if (ptr == 0)
    {
        memcpy(sd.bang.data(), sd.pong.data(), sizeof(float) * sd.szIn);
        synchro.allowCompute();
    }

    if (ptr < sd.frames)
    {
        long long frames = sd.frames - ptr;
        if (frames > len)
        {
            frames = len;
        }
        long pIn = ptr * m_inputs;
        memcpy(sd.pong.data() + pIn, data, sizeof(float) * frames * m_inputs);
        synchro.addAudioPtr((int)frames, sd.frames);
    }
    return len;
}

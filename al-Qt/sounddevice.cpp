#include "sounddevice.h"
#include <QDebug>
#include <QAudioDeviceInfo>

QString SoundDevice::error() const
{
    return m_error;
}

SoundPlayer::SoundPlayer(Synchro  &syn, SoundData &data) :
    synchro(syn), sd(data)
{

}

SoundDevice::SoundDevice(Synchro &syn, SoundData &data) :
    player(syn, data), synchro(syn), sd(data)
{

}

bool SoundPlayer::start()
{
    open(QIODevice::ReadOnly);
    m_outp->start(this);

    return m_outp->state() != QAudio::StoppedState;
}

void SoundPlayer::stop()
{
    m_outp->stop();
    close();
}

bool SoundDevice::start()
{
    open(QIODevice::ReadWrite);
    m_inp->start(this);

    return player.start();
}

void SoundDevice::stop()
{
    m_inp->stop();
    close();
    player.stop();
}

bool SoundDevice::selectDevices(QString adc, QString dac, int inputs, int outputs, int sampling)
{
    m_error = "No error";

    QAudioFormat format;
    // Set up the format
    format.setSampleRate(sampling);
    format.setCodec("audio/pcm");
    format.setByteOrder(QAudioFormat::LittleEndian);
    format.setChannelCount(m_inputs = inputs);
    format.setSampleSize(16);
    format.setSampleType(QAudioFormat::SignedInt);

    QAudioDeviceInfo di;

    QList<QAudioDeviceInfo> allIn = QAudioDeviceInfo::availableDevices(QAudio::Mode::AudioInput);
    if(select(adc, allIn, di))
    {
        m_adc = di.deviceName();
        if (di.isFormatSupported(format))
        {
            emit info(m_adc + ", " + QString::number(format.bytesPerFrame()) + " bytes per frame");
            m_inp = new QAudioInput(di, format, this);
        }
        else
        {
            emit info("Not supported by " + m_adc + ": " + format.codec());
        }
    }

    QList<QAudioDeviceInfo> allOut = QAudioDeviceInfo::availableDevices(QAudio::Mode::AudioOutput);
    if (select(dac, allOut, di))
    {
        m_dac = di.deviceName();
        format.setChannelCount(m_outputs = outputs);
        if (di.isFormatSupported(format))
        {
            emit info(m_dac + ", " + QString::number(format.bytesPerFrame()) + " bytes per frame");
            player.m_outp = new QAudioOutput(di, format, this);
        }
        else
        {
            emit info("Not supported by " + m_dac + ": " + format.codec());
        }
    }

    if (!m_inp || !player.m_outp)
    {
        if (!m_inp)
        {
            m_error = "Failed to open " + m_adc;
        }
        else if (!player.m_outp)
        {
            m_error = "Failed to open " + m_dac;
        }
        delete m_inp;
        delete player.m_outp;
        m_inp = nullptr;
        player.m_outp = nullptr;
        return false;
    }
    return true;
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

qint64 SoundPlayer::readData(char *data, qint64 maxlen)
{
    int ptr = synchro.getAudioPtrOut();
    memset(data, 0, maxlen);
    if (ptr < sd.frames)
    {
        long long frames = sd.frames - ptr;
        int nf = maxlen / sizeof(short) / sd.channels;
        if (frames > nf)
        {
            frames = nf;
        }

        long pOut = ptr * sd.channels;
        size_t n = sizeof(short) * frames * sd.channels;
        memcpy(data, sd.ping.data() + pOut, n);
        synchro.addAudioPtrOut(frames);
        return maxlen;
    }
    return 0;
}

qint64 SoundDevice::writeData(const char *data, qint64 len)
{
    int ptr = synchro.getAudioPtr();
    if (ptr == 0)
    {
        memcpy(sd.bang.data(), sd.pong.data(), sizeof(short) * sd.szIn);
        synchro.allowCompute();
    }

    if (ptr < sd.frames)
    {
        long long frames = sd.frames - ptr;
        int nf = len / sizeof(short) / m_inputs;
        if (frames > nf)
        {
            frames = nf;
        }
        long pIn = ptr * m_inputs;
        memcpy(sd.pong.data() + pIn, data, sizeof(short) * frames * m_inputs);
        synchro.addAudioPtr(frames, sd.frames);
    }
    return len;
}

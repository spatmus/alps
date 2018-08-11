#include "sounddevice.h"
#include <QDebug>
#include <QAudioDeviceInfo>

QString SoundDevice::error() const
{
    return m_error;
}

SoundDevice::SoundDevice()
{

}

bool SoundDevice::selectDevices(QString adc, QString dac, int inputs, int outputs, int sampling)
{
    m_error = "No error";

    QAudioFormat format;
    // Set up the format
    format.setSampleRate(sampling);
    format.setChannelCount(inputs);
    format.setSampleSize(16);
    format.setCodec("audio/pcm");
    format.setByteOrder(QAudioFormat::LittleEndian);
    format.setSampleType(QAudioFormat::Float);

    QAudioDeviceInfo di;

    QList<QAudioDeviceInfo> allIn = QAudioDeviceInfo::availableDevices(QAudio::Mode::AudioInput);
    if(select(adc, allIn, di))
    {
        m_adc = di.deviceName();
        m_inp = new QAudioInput(di, format, this);
    }

    QList<QAudioDeviceInfo> allOut = QAudioDeviceInfo::availableDevices(QAudio::Mode::AudioOutput);
    if (select(dac, allOut, di))
    {
        m_dac = di.deviceName();
        format.setChannelCount(outputs);
        m_outp = new QAudioOutput(di, format, this);
    }

    if (!m_inp || !m_outp)
    {
        if (!m_inp)
        {
            m_error = "Failed to open " + adc;
        }
        else if (!m_outp)
        {
            m_error = "Failed to open " + dac;
        }
        delete m_inp;
        delete m_outp;
        m_inp = nullptr;
        m_outp = nullptr;
    }
    else
    {
//        outp->start(&gen);
//        QIODevice *iodev = inp->start();
//        m_outp->start(m_inp->start());
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

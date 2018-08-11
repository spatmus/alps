#ifndef SOUNDDEVICE_H
#define SOUNDDEVICE_H

#include <QObject>
#include <QAudioInput>
#include <QAudioOutput>

class SoundDevice : public QObject
{
    Q_OBJECT

    QString m_adc;
    QString m_dac;
    int m_inputs = 0;
    int m_outputs = 0;
    QString m_error = "Not initialized";

    QAudioInput     *m_inp = nullptr;
    QAudioOutput    *m_outp = nullptr;

    bool select(QString name, QList<QAudioDeviceInfo> &all, QAudioDeviceInfo &dinfo);

public:
    SoundDevice();
    bool selectDevices(QString adc, QString dac, int inputs, int outputs, int sampling);
    QString error() const;
};

#endif // SOUNDDEVICE_H

#ifndef SOUNDDEVICE_H
#define SOUNDDEVICE_H

#include <QObject>
#include <QAudioInput>
#include <QAudioOutput>
#include "Synchro.hpp"
#include <QAudioBuffer>

struct SoundData {
    long                    szIn;
    long                    szOut;
    std::vector<float>      ping;
    std::vector<float>      pong;
    std::vector<float>      bang;

    int                     channels;
    int                     frames;
    int                     empty;
};

class SoundDevice : public QIODevice
{
    Q_OBJECT

    QString m_adc;
    QString m_dac;
    int m_inputs = 0;
    int m_outputs = 0;
    QString m_error = "Not initialized";

    QAudioInput     *m_inp = nullptr;
    QAudioOutput    *m_outp = nullptr;

    Synchro         &synchro;
    SoundData       &sd;

    bool select(QString name, QList<QAudioDeviceInfo> &all, QAudioDeviceInfo &dinfo);

public:
    SoundDevice(Synchro  &syn, SoundData &data);
    virtual ~SoundDevice() override { }

    virtual qint64 readData(char *data, qint64 maxlen) override;
    virtual qint64 writeData(const char *data, qint64 len) override;

    bool start();
    void stop();

    bool selectDevices(QString adc, QString dac, int inputs, int outputs, int sampling);
    QString error() const;

private slots:
    void handleInStateChanged(QAudio::State newState);
    void handleOutStateChanged(QAudio::State newState);

signals:
    void info(QString inf);
};

#endif // SOUNDDEVICE_H

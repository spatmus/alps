#ifndef SOUNDDEVICE_H
#define SOUNDDEVICE_H

#include <QObject>
#include <QAudioInput>
#include <QAudioOutput>
#include "Synchro.hpp"
#include <QAudioBuffer>

struct SoundData {
    long                    szIn = 0;
    long                    szOut = 0;
    std::vector<short>      ping;
    std::vector<short>      pong;
    std::vector<short>      bang;

    int                     channels = 8;
    int                     inputs = 8;
    int                     frames = 0;
    int                     empty = 0;

    QString toString()
    {
        return QString::number(frames) + " frames; "
                + QString::number(szIn) + " size IN; "
                + QString::number(szOut) + " size OUT; "
                + QString::number(ping.size()) + " ping; "
                + QString::number(pong.size()) + " pong; "
                + QString::number(channels) + " channels";
    }
};

class SoundPlayer : public QIODevice
{
    Q_OBJECT

    QString m_adc;
    Synchro         &synchro;
    SoundData       &sd;

public:
    SoundPlayer(Synchro  &syn, SoundData &data);
    virtual ~SoundPlayer() override { }

    bool start();
    void stop();

    QAudioOutput    *m_outp = nullptr;
    virtual qint64 readData(char *data, qint64 maxlen) override;
    virtual qint64 writeData(const char *data, qint64 len) override
    {
        Q_UNUSED(data);
        Q_UNUSED(len);
        return 0;
    }
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
    SoundPlayer     player;

    Synchro         &synchro;
    SoundData       &sd;

    bool select(QString name, QList<QAudioDeviceInfo> &all, QAudioDeviceInfo &dinfo);

public:
    SoundDevice(Synchro  &syn, SoundData &data);
    virtual ~SoundDevice() override { }

    virtual qint64 readData(char *data, qint64 maxlen) override
    {
        Q_UNUSED(data);
        Q_UNUSED(maxlen);
        return 0;
    }

    virtual qint64 writeData(const char *data, qint64 len) override;

    bool debug = false;

    bool start();
    void stop();

    bool selectDevices(QString adc, QString dac, int inputs, int outputs, int sampling);
    QString error() const;

signals:
    void info(QString inf);
};

#endif // SOUNDDEVICE_H

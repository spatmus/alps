#ifndef MAINLOOP_H
#define MAINLOOP_H

#include <QThread>
#include "Synchro.hpp"
#include "Speakers.hpp"
#include "sounddevice.h"

#define MAX_OUTPUTS 16
#define MAX_INPUTS  16
#define QUALITY     0.0
#define SAMPLE_RATE_ 96000

class MainLoop : public QThread
{
    Q_OBJECT

    Synchro     &synchro;
    SoundData   &sd;
    Speakers    &speakers;

    bool debug = false;

    static std::chrono::steady_clock::time_point now();

    void xcorr(int refChannel, int sigChannel, float * res);
    int findMaxAbs(float *d, int sz);
    int compute(); // returns reference delay in samples
    void report();
    void sendOsc(const char *address, const char *fmt, ...);

    // the measurement result
    int delays[MAX_OUTPUTS][MAX_INPUTS];

public:
    explicit MainLoop(Synchro &syn, SoundData &data, Speakers &spe);
    void run() override;

    int inputs  = 2; // stereo
    int outputs = 2;
    int ref_out = 0;
    int ref_in  = 0;
    float qual  = QUALITY;
    QString oscIP;
    QString oscPort;


    qint32 sampling = SAMPLE_RATE_;
    bool autopan = false;

    void setDebug(bool value);

signals:
    void info(QString inf);

public slots:
};

#endif // MAINLOOP_H

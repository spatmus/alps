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

typedef std::vector<float> Series;
typedef std::vector<Series> SeriesA;
typedef std::vector<SeriesA> SeriesAA;
struct TaskDescr
{
    TaskDescr(int num, int inputs, SeriesAA &data, SoundData &sd) :
        _num(num), _inputs(inputs),
        d(data), _sd(sd)
    {
    }

    int _num;
    int _inputs;
    SeriesAA &d;
    SoundData &_sd;
};

void xcorrFunc(int inp, TaskDescr td);

class MainLoop : public QThread
{
    Q_OBJECT

    Synchro     &synchro;
    Speakers    &speakers;

    bool debug = false;

    static std::chrono::steady_clock::time_point now();

    int compute(); // returns reference delay in samples

    QString report();
    void sendOsc(const char *address, const char *fmt, ...);

    // the measurement result
    int delays[MAX_OUTPUTS][MAX_INPUTS];

public:
    explicit MainLoop(Synchro &syn, SoundData &data, Speakers &spe);
    void run() override;

    static int findMaxAbs(const float *d, int sz);
    static void xcorr(SoundData &_sd, int refChannel, int sigChannel, float * res);
    SoundData   &sd;

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
    void correlation(const float* res, int sz, int inp, int outp);

public slots:
};

#endif // MAINLOOP_H

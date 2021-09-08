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
    TaskDescr(SeriesAA &data, SoundData &sd, int refInp, int refOut) :
        d(data), _sd(sd), _refInp(refInp), _refOut(refOut)
    {
    }

    // This constructor doesn't need reference channels. They can be accessed from MainLoop
    TaskDescr(SeriesAA &data, SoundData &sd) :
        d(data), _sd(sd), _refInp(-1), _refOut(-1)
    {
    }

    void init();

    int _outputs;
    int _inputs;
    int _refInp;
    int _refOut;
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

    SeriesAA saa;
    TaskDescr td;
    void compute2();
    void compute();
    int applyRefDelay(); // returns reference delay in samples

    QString report();
    void sendOsc(const char *address, const char *fmt, ...);

    // the measurement result
    int delays[MAX_OUTPUTS][MAX_INPUTS];

public:
    explicit MainLoop(Synchro &syn, SoundData &data, Speakers &spe);

    void allocateAll();
    void run() override;

    static int findMaxAbs(const float *d, int sz);
    static void xcorr(SoundData &_sd, int refChannel, int sigChannel, float * res);
    SoundData   &sd;

    int ref_out = 0;
    int ref_in  = 0;
    float qual  = QUALITY;
    QString oscIP;
    QString oscPort;
    qint32 sampling = SAMPLE_RATE_;
    bool autopan = false;
    bool useThreads = true;

    void setDebug(bool value);

signals:
    void info(QString inf);
    void correlation(const float* res, int sz, int inp, int outp, int idx);

public slots:
};

#endif // MAINLOOP_H

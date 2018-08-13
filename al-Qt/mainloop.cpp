#include "mainloop.h"
#include <chrono>
#include "../alglib/src/fasttransforms.h"

using namespace std::chrono;

MainLoop::MainLoop(Synchro  &syn, SoundData &data) : QThread(), synchro(syn), sd(data)
{
}

void MainLoop::setDebug(bool value)
{
    debug = value;
}

high_resolution_clock::time_point MainLoop::now()
{
    return high_resolution_clock::now();
}

void MainLoop::run()
{
    synchro.setStopped(false);
//        lo_address t = lo_address_new(oscIP, oscPort);
    for (int i = 0; !currentThread()->isInterruptionRequested(); i++)
    {
        auto t0 = now();
        synchro.transferData(sd.frames);
        auto t1 = now();
        emit info("debug " + QString::number(i + 1));
        if (i)
        {
            if (compute() == 0)
            {
                // emit info("zero reference delay; measurement ignored.");
                continue;
            }
        }
        auto t2 = now();
        if (i)
        {
            report();
        }
        auto t3 = now();
        if (debug)
        {
            duration<double> dt1 = duration_cast<duration<double>>(t1 - t0);
            duration<double> dt2 = duration_cast<duration<double>>(t2 - t1);
            duration<double> dt3 = duration_cast<duration<double>>(t3 - t2);

            emit info("wait: " + QString::number(dt1.count())
                + " compute: " + QString::number(dt2.count())
                + " report: " + QString::number(dt3.count()));
        }
    }
    synchro.setStopped(true);
}

int MainLoop::compute()
{
    memset(delays, 0, sizeof(delays));
    int num = sd.channels; // output channels
    float *res = new float[sd.frames];
    for (int n = 0; n < num; n++)
    {
        for (int inp = 0; inp < inputs; inp++)
        {
            // don't compute correlation of reference input with non-reference speakers
            if (inp == ref_in && n != ref_out) continue;

            xcorr(n, inp, res);
            int idx = findMaxAbs(res, (int)sd.frames);
            QString ok = " good";
            if (fabs(res[idx]) >= qual)
            {
                delays[n][inp] = idx;
            }
            else
            {
                ok = " ignored";
            }
            if (debug) emit info("speaker " + QString::number(n)
                                 + " mic " + QString::number(inp)
                                 + " idx " + QString::number(idx)
                                 + " val " + QString::number(res[idx])
                                 + ok);
        }
    }

    // One input channel is used for latency correction.
    // It is electrically connected to one output.
    int md = delays[ref_out][ref_in];
    emit info("reference delay " + QString::number(md));
    for (int n = 0; n < num; n++)
    {
        for (int inp = 0; inp < inputs; inp++)
        {
            delays[n][inp] -= md;
        }
    }

    delete [] res;
    return md;
}

void MainLoop::report()
{

}

int MainLoop::findMaxAbs(float *d, int sz)
{
    int idx = 0;
    float v = fabs(*d);
    for (int i = 1; i < sz; i++)
    {
        float t = fabs(d[i]);
        if (t > v)
        {
            idx = i;
            v = t;
        }
    }
    return idx;
}

// crosscorrelation for data from sd.ping and sd.pong, with given channels
void MainLoop::xcorr(int refChannel, int sigChannel, float * res)
{
    int sz = (int)sd.frames;
    alglib::real_1d_array x, y, r;
    x.setlength(sz);
    y.setlength(sz);
    r.setlength(sz);
    int ch = sd.channels;
    for (int i = 0; i < sz; i++)
    {
        x[i] = sd.ping[i * ch + refChannel];
        y[i] = sd.bang[i * inputs + sigChannel];
    }
    alglib::corrr1dcircular(y, sz, x, sz, r);
    for (int i = 0; i < sz; i++)
    {
        res[i] = r[i];
    }
}

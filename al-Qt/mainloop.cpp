#include "mainloop.h"
#include <chrono>
#include "../alglib/src/fasttransforms.h"
#include <QUdpSocket>
#include <QtEndian>

using namespace std::chrono;

MainLoop::MainLoop(Synchro  &syn, SoundData &data, Speakers &spe) :
    QThread(), synchro(syn), speakers(spe),
    td(saa, data), sd(data)
{
    allocateAll();
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
    td.init();
    synchro.setStopped(false);
    for (int i = 0; !currentThread()->isInterruptionRequested(); i++)
    {
        auto t0 = now();
        synchro.transferData(sd.frames);
        auto t1 = now();
        if (!i) continue;
        useThreads ? compute2() : compute();
        auto t2 = now();
        if (applyRefDelay() == 0)
        {
            // emit info("zero reference delay; measurement ignored.");
            emit info("debug no compute");
            continue;
        }

        QString rep = "No result";
        QString r = report();
        if (!r.isEmpty()) rep = r;
        auto t3 = now();
        if (debug)
        {
            duration<double> dt1 = duration_cast<duration<double>>(t1 - t0);
            duration<double> dt2 = duration_cast<duration<double>>(t2 - t1);
            duration<double> dt3 = duration_cast<duration<double>>(t3 - t2);

            emit info("debug " + QString::number(i + 1)
                + " wait: " + QString::number(dt1.count())
                + " compute: " + QString::number(dt2.count())
                + " report: " + QString::number(dt3.count()));
        }
        else
        {
            emit info("debug " + QString::number(i + 1) + " " + rep);
        }
    }
    synchro.setStopped(true);
}

void MainLoop::allocateAll()
{
    saa.resize(sd.outputs);
    for (int n = 0; n < sd.outputs; n++)
    {
        saa[n].resize(sd.inputs);
        for (int inp = 0; inp < sd.inputs; inp++)
        {
            saa[n][inp].resize(sd.frames);
        }
    }
}

void xcorrFunc(int inp, TaskDescr td)
{
    for (int n = 0; n < td._outputs; n++)
    {
        MainLoop::xcorr(td._sd, n, inp, td.d[n][inp].data());
    }
}

void MainLoop::compute2()
{
    TaskDescr td(saa, sd);
    td.init();

    std::vector<std::thread> threads(sd.inputs);

    for (int inp = 0; inp < sd.inputs; inp++)
    {
        std::thread t(xcorrFunc, inp, td);
        threads[inp] = std::move(t);
    }

    for (auto& th : threads) th.join();

    for (int n = 0; n < sd.outputs; n++)
    {
        for (int inp = 0; inp < sd.inputs; inp++)
        {
            float *res = saa[n][inp].data();
            int idx = findMaxAbs(res, sd.frames);
            if (debug)
            {
                emit correlation(res, sd.frames, inp, n, idx);
            }
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
}

void MainLoop::compute()
{
    memset(delays, 0, sizeof(delays));
    int num = sd.outputs; // output channels
    std::vector<float> res(sd.frames);
    for (int n = 0; n < num; n++)
    {
        for (int inp = 0; inp < sd.inputs; inp++)
        {
            // don't compute correlation of reference input with non-reference speakers
            if (inp == ref_in && n != ref_out) continue;

            xcorr(sd, n, inp, res.data());
            int idx = findMaxAbs(res.data(), sd.frames);
            if (debug)
            {
                emit correlation(res.data(), res.size(), inp, n, idx);
            }
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
}

int MainLoop::applyRefDelay()// returns reference delay in samples
{
    // One input channel is used for latency correction.
    // It is electrically connected to one output.
    int md = delays[ref_out][ref_in];
    if (debug) emit info("reference delay " + QString::number(md));
    for (int n = 0; n < sd.outputs; n++)
    {
        for (int inp = 0; inp < sd.inputs; inp++)
        {
            // keep the reference delay uncompensated
            if (n == ref_out && inp == ref_in) continue;
            delays[n][inp] -= md;
        }
    }
    return md;
}

QString MainLoop::report()
{
    char lbl[40]; // for OSC messages
    QString rep, rep2;
    for (int inp = 0; inp < sd.inputs; inp++)
    {
//        cout << "input:" << inp << endl;

        // report all distances
        for (int n = 0; n < sd.outputs; n++)
        {
            double d = (double)delays[n][inp] / sampling * 330;

            // validate the measured distance for all microphones except reference one
            if ((inp != ref_in) && !speakers.distOk(d, n, autopan ? 0 : speakers.getCoordinates(n).z)) continue;

            sprintf(lbl, "/mic%d", inp + 1);
            sendOsc(lbl, "if", n + 1, d);
            QString r;
            r.sprintf("mic %i sp %i dist %.2lf;", inp + 1, n + 1, d);
            rep2 += r;
        }

        if (autopan) continue;

        double X = 0, Y = 0;
        int averNum = 0;
        // for all speaker pairs
        for (int i = 0; i < speakers.numPairs(); i++)
        {
            int n = speakers.getPair(i).one.id;
            int m = speakers.getPair(i).two.id;
            if (n >= sd.outputs || m >= sd.outputs)
            {
                // cout << "ERROR: The speaker pair (" << n << "," << m << ") cannot be used" << endl;
                continue;
            }

            double d1 = (double)delays[n][inp] / sampling * 330;
            double d2 = (double)delays[m][inp] / sampling * 330;
            double z1 = speakers.getCoordinates(n).z;
            double z2 = speakers.getCoordinates(m).z;

//            if (debug) cout << "speakers (" << n << "," << m << ") dist (" << d1 << "," << d2 << ") z (" << z1 << "," << z2 << ")" << endl;

            if (!speakers.distOk(d1, n, z1)) continue;
            if (!speakers.distOk(d2, m, z2)) continue;

            double x, y;
            if (speakers.distToXY(d1, d2, n, m, &x, &y))
            {
                // cout << "speakers (" << n << "," << m << ") dist (" << d1 << "," << d2 << ") x: " << x << " y: " << y << endl;
                X += x;
                Y += y;
                averNum++;
            }
            else
            {
                // if (debug) cout << "no result" << endl;
            }
        }
        if (averNum)
        {
            X /= averNum;
            Y /= averNum;
            QString r;
            r.sprintf("mic %i x=%4.2lf  y=%4.2lf ", inp, X, Y);
            rep += r;
            sprintf(lbl, "/position%d", inp + 1);
            sendOsc(lbl, "ff", X, Y);
        }
        else
        {
            // cout << "no estimates for mic " << inp << endl;
            sprintf(lbl, "/noestimate%d", inp + 1);
            sendOsc(lbl, "");
        }
    }
    return rep.isEmpty() ? rep2 : rep;
}

int MainLoop::findMaxAbs(const float *d, int sz)
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
void MainLoop::xcorr(SoundData &_sd, int refChannel, int sigChannel, float * res)
{
    int sz = (int)_sd.frames;
    alglib::real_1d_array x, y, r;
    x.setlength(sz);
    y.setlength(sz);
    r.setlength(sz);
    int ch = _sd.outputs;
    for (int i = 0; i < sz; i++)
    {
        x[i] = _sd.ping[i * ch + refChannel] / 32768.0;
        y[i] = _sd.bang[i * _sd.inputs + sigChannel] / 32768.0;
    }
    alglib::corrr1dcircular(y, sz, x, sz, r);
    for (int i = 0; i < sz; i++)
    {
        res[i] = r[i];
    }
}

// always writes a multiple of 4 bytes
static QByteArray tosc_vwrite(const char *address, const char *format, va_list ap)
{
    QByteArray buffer;
    buffer.resize(2048);
    memset(buffer.data(), 0, 2048);
    strcpy(buffer.data(), address);
    quint32 i = strlen(address);
    i = (i + 4) & ~0x3;
    buffer[i++] = ',';
    int s_len = (int) strlen(format);
    strcpy(buffer.data() + i, format);
    i = (i + 4 + s_len) & ~0x3;

    for (int j = 0; format[j] != '\0'; ++j)
    {
        switch (format[j])
        {
            case 'f':
            {
                const float f = (float) va_arg(ap, double);
                *((quint32*) (buffer.data() + i)) = qToBigEndian<quint32>(*(quint32*) &f);
                i += 4;
                break;
            }
            case 'd':
            {
                const double f = (double) va_arg(ap, double);
                *((quint64*) (buffer.data() + i)) = qToBigEndian<quint64>(*((quint64 *) &f));
                i += 8;
                break;
            }
            case 'i':
            {
                const quint32 k = (quint32) va_arg(ap, int);
                *((uint32_t *) (buffer.data() + i)) = qToBigEndian<quint32>(k);
                i += 4;
                break;
            }
            case 's':
            {
                const char *str = (const char *) va_arg(ap, void *);
                s_len = (int) strlen(str);
                strcpy(buffer.data() + i, str);
                i = (i + 4 + s_len) & ~0x3;
                break;
            }
            case 'T': // true
            case 'F': // false
            case 'N': // nil
            case 'I': // infinitum
                break;
         }
    }
    buffer.resize(i);
    return buffer;
}

void MainLoop::sendOsc(const char *address, const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    QByteArray datagram = tosc_vwrite(address, fmt, ap);
    va_end(ap);

    QUdpSocket udp;
    udp.writeDatagram(datagram, QHostAddress(oscIP), oscPort.toInt());
}

void TaskDescr::init()
{
    _outputs = _sd.outputs;
    _inputs = _sd.inputs;

}

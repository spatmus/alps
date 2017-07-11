//
//  main.cpp
//  Audio1
//
//  Created by cmtuser on 07/05/2017.
//  Copyright © 2017 cmtuser. All rights reserved.
//

#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "../alglib/src/fasttransforms.h"
#include "portaudio.h"
#include "sndfile.h"
#include "../lo/lo.h"

#define PA_SAMPLE_TYPE  paFloat32
typedef float SAMPLE;

// TODO
// 1. The values defined below should be read from a configuration file
// 2. Add a check that reference channel is connected properly (the first speaker output connected to the last input)
// 3. Add a signal quality check as a ratio between the highest and the lowest RMS of parts of input signals

#define DURATION    0.2
#define FADE_PART   0.2
#define MAX_OUTPUTS 16
#define MAX_INPUTS  16
#define ADC_INPUTS  2
#define SAMPLE_RATE 96000
#define REPEAT      50
#define EXTRA_PLAY  20
#define OSC_IP      "192.168.1.4"
#define OSC_PORT    "7770"

//const char *fname = "/Users/cmtuser/Desktop/xcodedocs/Audio1/Audio1/alhambra.wav";
const char *fname = "/Users/cmtuser/Desktop/xcodedocs/Audio1/Audio1/20kHz_noise2ch.wav";
// const char *fname = "/Users/cmtuser/Desktop/xcodedocs/Audio1/Audio1/4ch.wav";

// speaker coordinates x,y
float xy[][3] = {
    {0, 0, 0},
    {1.2, 0, 0},
    {7, 88},
    {0, 17},
    {2, 0},
    {2, 2},
    {0, 2},
    {3, 3}
};

using namespace std;

struct SoundData {
    long        szIn;
    long        szOut;
    long        ptr;
    SAMPLE      *ping;
    SAMPLE      *pong;
    SAMPLE      *bang;
    SF_INFO     sfInfo;
    int         empty;
};

SoundData sd = {0, 0, 0, NULL, NULL};

// the measurement result
int delays[MAX_OUTPUTS][MAX_INPUTS];


/* This routine will be called by the PortAudio engine when audio is needed.
 It may be called at interrupt level on some machines so don't do anything
 that could mess up the system like calling malloc() or free().
 */
static int paCallback( const void *inputBuffer, void *outputBuffer,
                          unsigned long framesPerBuffer,
                          const PaStreamCallbackTimeInfo* timeInfo,
                          PaStreamCallbackFlags statusFlags,
                          void *userData )
{
    SoundData &d = *((SoundData*)userData);
    if (d.ptr < d.sfInfo.frames)
    {
        long long frames = d.sfInfo.frames - d.ptr;
        if (frames > framesPerBuffer)
        {
            frames = framesPerBuffer;
        }
        long pOut = d.ptr * d.sfInfo.channels;
        long pIn = d.ptr * ADC_INPUTS;
        memcpy(outputBuffer, d.ping + pOut, sizeof(SAMPLE) * frames * d.sfInfo.channels);
        memcpy(d.pong + pIn, inputBuffer, sizeof(SAMPLE) * frames * ADC_INPUTS);
        d.ptr += frames;
    }
    else
    {
        if (d.empty-- == EXTRA_PLAY)
        {
            memcpy(d.bang, d.pong, sizeof(SAMPLE) * d.sfInfo.frames * ADC_INPUTS);
        }
        memset(outputBuffer, 0, sizeof(SAMPLE) * framesPerBuffer * d.sfInfo.channels);
    }
    return paContinue;
}

bool loadWave(const char *fname);
void fadeInOut();
void saveWave(const char *fname);
bool selectDevices(int *inDev, int *outDev);
bool openStream(int inDev, int outDev, PaStream **stream);
void compute();
void report(lo_address t);
void loadConfiguration(const char *cfg);
float quality(int input);

// values loaded from configuration file
float duration = DURATION;
float fade = FADE_PART;
int inputs = ADC_INPUTS;
int repeat = REPEAT;
int extraPlay = EXTRA_PLAY;
const char * oscIP = OSC_IP;
const char * oscPort = OSC_PORT;
const char * adcIn = "Built-in";
const char * adcOut = "Built-in";

int main(int argc, const char * argv[]) {
    if (argc > 1)
    {
        loadConfiguration(argv[1]);
    }
    
    int inDev = -1, outDev = -1;
    if (loadWave(fname))
    {
        float dur = (float)sd.sfInfo.frames / SAMPLE_RATE;
        if (dur > duration)
        {
            dur = duration;
        }
        sd.sfInfo.frames = duration * SAMPLE_RATE;
        sd.szOut = sd.sfInfo.frames * sd.sfInfo.channels;
        sd.szIn = sd.sfInfo.frames * inputs;
        fadeInOut();
        
        PaError err = Pa_Initialize();
        if( err == paNoError )
        {
            PaStream *stream = 0;
            if (selectDevices(&inDev, &outDev) && openStream(inDev, outDev, &stream))
            {
                lo_address t = lo_address_new(oscIP, oscPort);
                for (int i = 0; i < repeat; i++)
                {
                    cout << "run " << (i + 1) << endl;
                    compute();
                    if (i)
                    {
                        report(t);
                    }

                    int cnt = 0;
                    /* wait until the entire signal has been played */
                    while (sd.empty > 0)
                    {
                        ++cnt;
                        Pa_Sleep(10);
                    }
                    cout << "counter=" << cnt << endl;
                    sd.ptr = 0;
                    sd.empty = EXTRA_PLAY;
                }
                Pa_StopStream(stream);
                Pa_CloseStream(stream);
                saveWave("record.wav");
            }
        }
        else
        {
            printf(  "PortAudio error: %s\n", Pa_GetErrorText( err ) );
            return 1;
        }
        delete [] sd.ping;
        delete [] sd.pong;
        delete [] sd.bang;
        sd.ping = 0;
        sd.pong = 0;
        sd.bang = 0;
        sd.szIn = 0;
        sd.szOut = 0;
        
        err = Pa_Terminate();
        if( err != paNoError )
            printf(  "PortAudio error: %s\n", Pa_GetErrorText( err ) );
    }
    
    return 0;
}

void loadConfiguration(const char *cfg)
{
    FILE *f = fopen(cfg, "rt");
    if (f)
    {
        cout << "----- Using configuration " << cfg << endl;
        char buf[1000];
        while (fgets(buf, sizeof(buf), f))
        {
            char * p = strtok(buf, "=");
            if (!strcmp(p, "duration"))
            {
                duration = atof(strtok(0, "\r\n"));
                cout << "duration " << duration << endl;
            }
            else if (!strcmp(p, "fade"))
            {
                fade = atof(strtok(0, "\r\n"));
                cout << "fade " << fade << endl;
            }
            else if (!strcmp(p, "inputs"))
            {
                inputs = atoi(strtok(0, "\r\n"));
                cout << "inputs " << inputs << endl;
            }
            else if (!strcmp(p, "repeat"))
            {
                repeat = atoi(strtok(0, "\r\n"));
                cout << "repeat " << repeat << endl;
            }
            else if (!strcmp(p, "extraPlay"))
            {
                extraPlay = atoi(strtok(0, "\r\n"));
                cout << "extraPlay " << extraPlay << endl;
            }
            else if (!strcmp(p, "oscIP"))
            {
                oscIP = strdup(strtok(0, "\r\n"));
                cout << "oscIP " << oscIP << endl;
            }
            else if (!strcmp(p, "oscPort"))
            {
                oscPort = strdup(strtok(0, "\r\n"));
                cout << "oscPort " << oscPort << endl;
            }
            else if (!strcmp(p, "adcIn"))
            {
                adcIn = strdup(strtok(0, "\r\n"));
                cout << "adcIn " << adcIn << endl;
            }
            else if (!strcmp(p, "adcOut"))
            {
                adcOut = strdup(strtok(0, "\r\n"));
                cout << "adcOut " << adcOut << endl;
            }
        }
        cout << "-----" << endl;
        fclose(f);
    }
}

int selectFromList(int numDevices, bool inp)
{
    int cnt = 0;
    const int MAX_DEV = 20;
    int idxs[MAX_DEV];
    for(int i = 0; i < numDevices; i++) {
        const PaDeviceInfo *deviceInfo = Pa_GetDeviceInfo( i );
        int num = inp ? deviceInfo->maxInputChannels : deviceInfo->maxOutputChannels;
        if (num) {
            idxs[cnt] = i;
            cnt++;
            cout << cnt << " " << deviceInfo->name << endl;
            if (strstr(deviceInfo->name, inp ? adcIn : adcOut))
            {
                cout << "Used per configuration" << endl;
                return i;
            }
        }
    }
    switch (cnt) {
        case 0:
            return -1;
            
        case 1:
            cout << " --- selected: " << Pa_GetDeviceInfo( idxs[0] )->name << endl;
            return idxs[0];
            
        default:
            while (1) {
                int val;
                cout << (inp ? "Input" : "Output") << " device [1.." << cnt << "]: ";
                cin >> val;
                if (val > 0 && val <= cnt)
                {
                    cout << " --- selected: " << Pa_GetDeviceInfo( idxs[val - 1] )->name << endl;
                    return idxs[val - 1];
                }
            }
            break;
    }
    return 0;
}

bool selectDevices(int *inDev, int *outDev)
{
    int numDevices = Pa_GetDeviceCount();
    if( numDevices < 0 )
    {
        printf( "ERROR: Pa_CountDevices returned 0x%x\n", numDevices );
        return false;
    }
    
    int i = selectFromList(numDevices, true);
    if (i < 0) {
        return false;
    }
    *inDev = i;

    i = selectFromList(numDevices, false);
    if (i < 0) {
        return false;
    }
    *outDev = i;
    
    return true;
}

bool openStream(int inDev, int outDev, PaStream **stream)
{
    PaStreamParameters inPars;
    inPars.device = inDev;
    inPars.channelCount = inputs; // sd.sfInfo.channels;
    inPars.sampleFormat = PA_SAMPLE_TYPE;
    inPars.hostApiSpecificStreamInfo = NULL;
    inPars.suggestedLatency = 0.1;
    
    PaStreamParameters outPars = inPars;
    outPars.device = outDev;
    outPars.channelCount = sd.sfInfo.channels;
    
    sd.ptr = 0;
    sd.empty = extraPlay;
    
    /* Open an audio I/O stream. */
    PaError err = Pa_OpenStream( stream,
                                 &inPars,          /* no input channels */
                                 &outPars,          /* stereo output */
                                 SAMPLE_RATE,
                                 256,        /* frames per buffer, i.e. the number
                                                of sample frames that PortAudio will
                                                request from the callback. Many apps
                                                may want to use
                                                paFramesPerBufferUnspecified, which
                                                tells PortAudio to pick the best,
                                                possibly changing, buffer size.*/
                                 paNoFlag,
                                 paCallback, /* this is your callback function */
				 &sd ); /*This is a pointer that will be passed to
                                               your callback*/
    if( err != paNoError )
    {
        printf(  "PortAudio open error: %s\n", Pa_GetErrorText( err ) );
        return false;
    }
    err = Pa_StartStream(*stream);
    if( err != paNoError )
    {
        printf(  "PortAudio start error: %s\n", Pa_GetErrorText( err ) );
        return false;
    }
    
    return true;
}

// crosscorrelation for data from sd.ping and sd.pong, with given channels
void xcorr(int refChannel, int sigChannel, SAMPLE * res)
{
    int sz = (int)sd.sfInfo.frames;
    alglib::real_1d_array x, y, r;
    x.setlength(sz);
    y.setlength(sz);
    r.setlength(sz);
    int ch = sd.sfInfo.channels;
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

int findMaxAbs(SAMPLE *d, int sz)
{
    int idx = 0;
    SAMPLE v = fabs(*d);
    for (int i = 1; i < sz; i++)
    {
        SAMPLE t = fabs(d[i]);
        if (t > v)
        {
            idx = i;
            v = t;
        }
    }
    return idx;
}

void compute()
{
    memset(delays, 0, sizeof(delays));
    int num = sd.sfInfo.channels; // output channels
    SAMPLE *res = new SAMPLE[sd.sfInfo.frames];
    for (int n = 0; n < num; n++)
    {
        int inps = (n == num - 1) ? inputs - 1 : inputs;
        for (int inp = 0; inp < inps; inp++)
        {
            xcorr(n, inp, res);
            int idx = findMaxAbs(res, (int)sd.sfInfo.frames);
            delays[n][inp] = idx;
        }
    }
    
    // The last input channel is used for latency correction.
    // It is electrically connected to the first output.
    int md = delays[0][inputs - 1];
    for (int n = 0; n < num; n++)
    {
        for (int inp = 0; inp < inputs; inp++)
        {
            delays[n][inp] -= md;
        }
    }
    
    delete [] res;
}

// Estimates the location x, y from distances d1,d2 to speakers
// located at 0,0 and L,0
bool distToXY(float d1, float d2, float L, float *x, float *y)
{
    *x = (d1 * d1 + L * L - d2 * d2) / 2 / L;
    if (d1 >= *x)
    {
        *y = sqrt(d1 * d1 - *x * *x);
        return true;
    }
    
    return false;
}

// Estimates the location x, y from distances d1,d2 to speakers n and m
// located at xy[n] and xy[m], see above
bool distToXY(float d1, float d2, int n, int m, float *x, float *y)
{
    float x1 = xy[n][0];
    float y1 = xy[n][1];
    float x2 = xy[m][0];
    float y2 = xy[m][1];
    float dx = x2 - x1;
    float dy = y2 - y1;
    float L = sqrtf(dx * dx + dy * dy);
    float a = atan2f(dy, dx);
    float X, Y;
    if (distToXY(d1, d2, L, &X, &Y))
    {
        float si = sinf(a);
        float co = cosf(a);
        *x = x1 + X * co - Y * si;
        *y = y1 + X * si + Y * co;
        return true;
    }
    
    return false;
}

double pythagor(double d, double z)
{
    if (d < z)
    {
        cout << "pythagor error " << d << " < " << z << endl;
        return 0;
    }
    return sqrt(d * d - z * z);
}

void report(lo_address t)
{
    int num = sd.sfInfo.channels; // output channels
    // for all microphones
    for (int inp = 0; inp < inputs - 1; inp++)
    {
        float X = 0, Y = 0;
        int averNum = 0;
        cout << "input:" << inp << endl;
        // for all speakers
        for (int n = 0; n < num; n++)
        {
            float d1 = (float)delays[n][inp] / SAMPLE_RATE * 330;
            if (!d1) continue;
            d1 = pythagor(d1, xy[n][2]);
            cout << "speaker: " << n << " dist: " << d1 << endl;
            
            // for all pairs of speakers
            for (int m = n + 1; m < num; m++)
            {
                float d2 = (float)delays[m][inp] / SAMPLE_RATE * 330;
                if (!d2) continue;
                d2 = pythagor(d2, xy[m][2]);
                
                float x, y;
                if (distToXY(d1, d2, n, m, &x, &y))
                {
                    cout << "x: " << x << " y: " << y << endl;
                    X += x;
                    Y += y;
                    averNum++;
                }
                else
                {
                    cout << "no result" << endl;
                }
            }
        }
        if (averNum)
        {
            X /= averNum;
            Y /= averNum;
            cout << "position estimate for mic " << inp <<
                " x=" << X << " y=" << Y << endl;

            char lbl[40];
            sprintf(lbl, "/position%d", inp + 1);
            if (lo_send(t, lbl, "ff", X, Y) == -1) 
            {
                cout << "OSC error " << lo_address_errno(t) << lo_address_errstr(t) << endl;
            }
        }
        else
        {
            cout << "no estimate for mic " << inp << endl;
        }
    }
}

void saveWave(const char *fname)
{
    SNDFILE* f = sf_open(fname, SFM_WRITE, &sd.sfInfo);
    if (!f) {
        cout << "sfWriteFile(): couldn't open \"" << fname << "\"" << endl;
    } else {
        sf_count_t samples_written = sf_write_float(f, sd.bang, sd.szIn);
        if (samples_written != (int)sd.szIn) {
            cout << "saveWave(): written " << samples_written << " float samples, expected "
            << sd.szIn << " to \"" << fname << "\"" << endl;
        }
        sf_close(f);
    }
}

bool loadWave(const char *fname)
{
    delete [] sd.ping;
    delete [] sd.pong;
    delete [] sd.bang;
    memset(&sd, 0, sizeof(sd));
    SNDFILE* f = sf_open(fname, SFM_READ, &sd.sfInfo);
    if (!f) {
        cout << "sfReadFile(): couldn't read \"" << fname << "\"" << endl;
        return false;
    }
    
    sd.ping = new SAMPLE [sd.szOut = sd.sfInfo.frames * sd.sfInfo.channels];
    sd.pong = new SAMPLE [sd.szIn = sd.sfInfo.frames * inputs];
    sd.bang = new SAMPLE [sd.szIn = sd.sfInfo.frames * inputs];
    memset(sd.pong, 0, sd.szIn * sizeof(SAMPLE));
    memset(sd.bang, 0, sd.szIn * sizeof(SAMPLE));
    
    sf_count_t samples_read = sf_read_float(f, sd.ping, sd.szOut);
    if (samples_read != sd.szOut) {
        cout << "loadWave(): read " << samples_read << " float samples, expected "
        << sd.szOut << " from \"" << fname << "\"" << endl;
    }
    if (sd.sfInfo.samplerate != SAMPLE_RATE) {
        cout << "Sampling frequency " << sd.sfInfo.samplerate << " instead of " << SAMPLE_RATE << endl;
    }
    sf_close(f);
    return true;
}

void fadeInOut()
{
    long last = sd.szOut - 1;
    int fdCnt = fade * sd.szOut;
    for (long i = 0; i < fdCnt; i++)
    {
        float v = (float) i / fdCnt;
        sd.ping[i] *= v;
        sd.ping[last - i] *= v;
    }
}

//
//  main.cpp
//  Audio1
//
//  Created by cmtuser on 07/05/2017.
//  Copyright © 2017 cmtuser. All rights reserved.
//

#include <iostream>
#include "../alglib/src/statistics.h"
#include "portaudio.h"
#include "sndfile.h"

#define PA_SAMPLE_TYPE  paFloat32
typedef float SAMPLE;

#define MAX_OUTPUTS 16
#define ADC_INPUTS  2
#define SAMPLE_RATE 96000

using namespace std;

struct SoundData {
    long        szIn;
    long        szOut;
    long        ptr;
    SAMPLE      *ping;
    SAMPLE      *pong;
    SF_INFO     sfInfo;
};

SoundData sd = {0, 0, 0, NULL, NULL};

// the measurement result
int delays[MAX_OUTPUTS][ADC_INPUTS];


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
    long p = d.ptr * d.sfInfo.channels;
    memcpy(outputBuffer, d.ping + p, sizeof(SAMPLE) * framesPerBuffer * d.sfInfo.channels);
    memcpy(d.pong + p, inputBuffer, sizeof(SAMPLE) * framesPerBuffer * ADC_INPUTS);
    d.ptr += framesPerBuffer;
    return d.ptr > d.sfInfo.frames ? paComplete : paContinue;
}

bool loadWave(const char *fname);
void saveWave(const char *fname);
bool selectDevices(int *inDev, int *outDev);
void makeNoise(int inDev, int outDev);
void compute();
void report();

int main(int argc, const char * argv[]) {
//    const char *fname = "/Users/cmtuser/Desktop/xcodedocs/Audio1/Audio1/20kHz_noise2ch.wav";
    const char *fname = "/Users/cmtuser/Desktop/xcodedocs/Audio1/Audio1/alhambra.wav";
    if (argc > 1)
    {
        fname = argv[1];
    }
    
    int inDev = -1, outDev = -1;
    if (loadWave(fname))
    {
        float duration = 2.0;
        sd.sfInfo.frames = duration * SAMPLE_RATE;
        sd.szOut = sd.sfInfo.frames * sd.sfInfo.channels;
        sd.szIn = sd.sfInfo.frames * ADC_INPUTS;
        
        PaError err = Pa_Initialize();
        if( err == paNoError )
        {
            if (selectDevices(&inDev, &outDev))
            {
                time_t t0 = time(0);
                makeNoise(inDev, outDev);
                cout << "time " << time(0) - t0 << endl;
                compute();
                report();
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
        sd.ping = 0;
        sd.pong = 0;
        sd.szIn = 0;
        sd.szOut = 0;
        
        err = Pa_Terminate();
        if( err != paNoError )
            printf(  "PortAudio error: %s\n", Pa_GetErrorText( err ) );
    }
    
    return 0;
}

int selectFromList(int numDevices, bool inp)
{
    int cnt = 0;
    const int MAX_DEV = 20;
    int idxs[MAX_DEV];
    for(int i=0; i<numDevices; i++ ) {
        const PaDeviceInfo *deviceInfo = Pa_GetDeviceInfo( i );
        int num = inp ? deviceInfo->maxInputChannels : deviceInfo->maxOutputChannels;
        if (num) {
            idxs[cnt] = i;
            cnt++;
            cout << cnt << " " << deviceInfo->name << endl;
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

void makeNoise(int inDev, int outDev)
{
    PaStream *stream;
    PaStreamParameters inPars;
    inPars.device = inDev;
    inPars.channelCount = ADC_INPUTS; // sd.sfInfo.channels;
    inPars.sampleFormat = PA_SAMPLE_TYPE;
    inPars.hostApiSpecificStreamInfo = NULL;
    inPars.suggestedLatency = 0.1;
    
    PaStreamParameters outPars = inPars;
    outPars.device = outDev;
    outPars.channelCount = sd.sfInfo.channels;
    
    /* Open an audio I/O stream. */
    PaError err = Pa_OpenStream( &stream,
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
        printf(  "PortAudio error: %s\n", Pa_GetErrorText( err ) );
    }
    else
    {
        err = Pa_StartStream( stream );
        if( err == paNoError )
        {
            /* Sleep for several seconds. */
            Pa_Sleep(sd.sfInfo.frames * 1000 / SAMPLE_RATE + 200);
            err = Pa_StopStream( stream );
        }
        
        Pa_CloseStream( stream );
    }
}

// crosscorrelation for data from sd.ping and sd.pong, with given channels
void xcorr(int refChannel, int sigChannel, SAMPLE * res)
{
    int sz = (int)sd.sfInfo.frames;
    alglib::real_1d_array x, y;
    x.setlength(sz);
    y.setlength(sz);
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
    int num = sd.sfInfo.channels; // output channels
    SAMPLE *res = new SAMPLE[sd.sfInfo.frames];
    for (int n = 0; n < num; n++)
    {
        int inps = (n == num - 1) ? ADC_INPUTS - 1 : ADC_INPUTS;
        for (int inp = 0; inp < inps; inp++)
        {
            xcorr(n, inp, res);
            int idx = findMaxAbs(res, (int)sd.sfInfo.frames);
            cout << idx << endl;
            delays[n][inp] = idx;
        }
    }
    
    // The last input channel is used for latency correction.
    // It is electrically connected to the first output.
    int md = delays[0][ADC_INPUTS - 1];
    for (int n = 0; n < num; n++)
    {
        for (int inp = 0; inp < ADC_INPUTS; inp++)
        {
            delays[n][inp] -= md;
        }
    }
    
    delete [] res;
}

void report()
{
    
}

void saveWave(const char *fname)
{
    SNDFILE* f = sf_open(fname, SFM_WRITE, &sd.sfInfo);
    if (!f) {
        cout << "sfWriteFile(): couldn't open \"" << fname << "\"" << endl;
    } else {
        sf_count_t samples_written = sf_write_float(f, sd.pong, sd.szIn);
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
    memset(&sd, 0, sizeof(sd));
    SNDFILE* f = sf_open(fname, SFM_READ, &sd.sfInfo);
    if (!f) {
        cout << "sfReadFile(): couldn't read \"" << fname << "\"" << endl;
        return false;
    }
    
    sd.ping = new SAMPLE [sd.szOut = sd.sfInfo.frames * sd.sfInfo.channels];
    sd.pong = new SAMPLE [sd.szIn = sd.sfInfo.frames * ADC_INPUTS];
    memset(sd.pong, 0, sd.szIn * sizeof(SAMPLE));
    
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


//
//  main.cpp
//  Audio1
//
//  Created by cmtuser on 07/05/2017.
//  Copyright © 2017 cmtuser. All rights reserved.
//

#include <iostream>
#include "portaudio.h"
#include "sndfile.h"

#define SAMPLE_RATE 96000

/* This routine will be called by the PortAudio engine when audio is needed.
 It may called at interrupt level on some machines so don't do anything
 that could mess up the system like calling malloc() or free().
 */
static int patestCallback( const void *inputBuffer, void *outputBuffer,
                          unsigned long framesPerBuffer,
                          const PaStreamCallbackTimeInfo* timeInfo,
                          PaStreamCallbackFlags statusFlags,
                          void *userData )
{
    return 0;
}

struct SoundData {
    long sz;
    long ptr;
    float *dt;
    SF_INFO sfInfo;
};

SoundData sd = {0, 0, 0};

bool loadWave(const char *fname)
{
    delete [] sd.dt;
    memset(&sd, 0, sizeof(sd));
    SNDFILE* f = sf_open(fname,SFM_READ,&sd.sfInfo);
    if (!f) {
        std::cout << "sfReadFile(): couldn't read \"" << fname << "\"" << std::endl;
        return false;
    }
    
    sd.dt = new float [sd.sz = sd.sfInfo.frames * sd.sfInfo.channels];
    
    sf_count_t samples_read = sf_read_float(f, sd.dt, sd.sz);
    if (samples_read < (int)sd.sz) {
        std::cout << "sfReadFile(): read " << samples_read << " float samples, expected "
        << sd.sz << " from \"" << fname << "\"" << std::endl;
    }
    sf_close(f);
    return true;
}

bool selectDevices(int *inDev, int *outDev);
void makeNoise(int inDev, int outDev);
void compute();
void report();

int main(int argc, const char * argv[]) {
    const char *fname = "noise.wav";
    if (argc > 1)
    {
        fname = argv[1];
    }
    int inDev = -1, outDev = -1;
    if (loadWave(fname))
    {
        PaError err = Pa_Initialize();
        if( err == paNoError )
        {
            if (selectDevices(&inDev, &outDev))
            {
                makeNoise(inDev, outDev);
                compute();
                report();
            }
        }
        else
        {
            printf(  "PortAudio error: %s\n", Pa_GetErrorText( err ) );
            return 1;
        }
        delete [] sd.dt;
        sd.dt = 0;
        sd.sz = 0;
        
        err = Pa_Terminate();
        if( err != paNoError )
            printf(  "PortAudio error: %s\n", Pa_GetErrorText( err ) );
    }
    
    return 0;
}

int selectFromList(int numDevices, bool inp)
{
    int cnt = 0;
    for(int i=0; i<numDevices; i++ ) {
        const PaDeviceInfo *deviceInfo = Pa_GetDeviceInfo( i );
        int num = inp ? deviceInfo->maxInputChannels : deviceInfo->maxOutputChannels;
        if (num) {
            cnt++;
            std::cout << cnt << " " << deviceInfo->name << std::endl;
        }
    }
    switch (cnt) {
        case 0:
            return 0;
            
        case 1:
            std::cout << " - selected " << std::endl;
            return 1;
            
        default:
            while (1) {
                int val;
                std::cout << (inp ? "Input" : "Output") << " device [1.." << cnt << "]: ";
                std::cin >> val;
                if (val > 0 && val <= cnt)
                {
                    return val;
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
    if (i < 1) {
        return false;
    }
    *inDev = i;

    i = selectFromList(numDevices, false);
    if (i < 1) {
        return false;
    }
    *outDev = i;
    
    return true;
}

void makeNoise(int inDev, int outDev)
{
    PaStream *stream;
    /* Open an audio I/O stream. */
    PaError err = Pa_OpenDefaultStream( &stream,
                                       2,          /* no input channels */
                                       2,          /* stereo output */
                                       paFloat32,  /* 32 bit floating point output */
                                       SAMPLE_RATE,
                                       256,        /* frames per buffer, i.e. the number
                                                    of sample frames that PortAudio will
                                                    request from the callback. Many apps
                                                    may want to use
                                                    paFramesPerBufferUnspecified, which
                                                    tells PortAudio to pick the best,
                                                    possibly changing, buffer size.*/
                                       patestCallback, /* this is your callback function */
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
            Pa_Sleep(3*1000);
            err = Pa_StopStream( stream );
        }
        
        Pa_CloseStream( stream );
    }
}

void compute()
{
    
}

void report()
{
    
}

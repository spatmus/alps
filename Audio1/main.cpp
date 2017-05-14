//
//  main.cpp
//  Audio1
//
//  Created by cmtuser on 07/05/2017.
//  Copyright © 2017 cmtuser. All rights reserved.
//

#include <iostream>
#include "portaudio.h"

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

int data; // tot be replaced

int main(int argc, const char * argv[]) {
    // insert code here...
    std::cout << "Hello, World!\n";

    PaError err = Pa_Initialize();
    if( err != paNoError )
    {
        printf(  "PortAudio error: %s\n", Pa_GetErrorText( err ) );
        return 1;
    }
    
    int numDevices = Pa_GetDeviceCount();
    if( numDevices < 0 )
    {
        printf( "ERROR: Pa_CountDevices returned 0x%x\n", numDevices );
        err = numDevices;
        return 1;
    }
    
    const PaDeviceInfo *deviceInfo;
    for(int i=0; i<numDevices; i++ )
    {
        deviceInfo = Pa_GetDeviceInfo( i );
        std::cout << deviceInfo->name << std::endl;
    }
    
    PaStream *stream;
    /* Open an audio I/O stream. */
    err = Pa_OpenDefaultStream( &stream,
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
                               &data ); /*This is a pointer that will be passed to
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

    err = Pa_Terminate();
    if( err != paNoError )
        printf(  "PortAudio error: %s\n", Pa_GetErrorText( err ) );
    return 0;
}

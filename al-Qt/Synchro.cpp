//
//  Synchro.cpp
//  alOF
//
//  Created by user on 03.08.2018.
//

#include "Synchro.hpp"

void Synchro::setStopped(bool value)
{
    stopped = value;
    doneaudio.notify_one();
    allowcompute.notify_one();
}

int Synchro::getAudioPtr()
{
    //        unique_lock<mutex> lck(mtx);
    return audioPtr;
}

void Synchro::addAudioPtr(int add, int cnt)
{
    std::unique_lock<std::mutex> lck(mtx);
    audioPtr += add;
    if (audioPtr >= cnt)
    {
        doneaudio.notify_one();
        //            cout << now() << " == audio done" << endl;
    }
}

void Synchro::addAudioPtrOut(int add)
{
    audioPtrOut += add;
}

void Synchro::transferData(int cnt)
{
    std::unique_lock<std::mutex> lck(mtx);
    compute = false;
    // wait until audio is complete
    while ((getAudioPtr() < cnt) && !stopped)
    {
        doneaudio.wait(lck);
    }
    
    audioPtr = 0;
    audioPtrOut = 0;
    // wait for notification from the audio thread
    while (!compute && !stopped)
    {
        allowcompute.wait(lck);
    }
}

void Synchro::allowCompute()
{
    std::unique_lock<std::mutex> lck(mtx);
    compute = true;
    allowcompute.notify_one();
    //        cout << now() << " == allow compute" << endl;
}

//
//  Synchro.hpp
//  alOF
//
//  Created by user on 03.08.2018.
//

#ifndef Synchro_hpp
#define Synchro_hpp

#include <mutex>

// http://en.cppreference.com/w/cpp/thread/condition_variable

struct Synchro
{
    std::condition_variable allowcompute;
    std::condition_variable doneaudio;
    std::mutex mtx;
    
    int audioPtr = 0;
    bool compute = true;
    bool stopped = false;
    
    int getAudioPtr();
    void addAudioPtr(int add, int cnt);

    int audioPtrOut = 0;
    int getAudioPtrOut() { return audioPtrOut; }
    void addAudioPtrOut(int add);

    void transferData(int cnt);
    void allowCompute();

public:
    void setStopped(bool value);
};

#endif /* Synchro_hpp */

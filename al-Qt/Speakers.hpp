//
//  Speakers.hpp
//  alOF
//
//  Created by user on 03.08.2018.
//

#ifndef Speakers_hpp
#define Speakers_hpp

#include <vector>
#include <string>

// 2D projections of speakers are used to select usable pairs
struct speaker
{
    float x;
    float y;
    bool used;
    int id;
    
    speaker(float _x, float _y)
    {
        x = _x;
        y = _y;
        used = false;
        id = -1;
    }
    
    speaker operator - (speaker &s) { return speaker(x - s.x, y- s.y); }
    int operator == (speaker &s) { return (x == s.x) && (y == s.y); }
};

struct spair
{
    spair(speaker &o, speaker &t) : one(o), two(t)
    {
    }
    
    speaker &one;
    speaker &two;
    
    float ang(speaker &another)
    {
        speaker a = two - one;
        speaker b = another - one;
        return a.x * b.y - b.x * a.y;
    }
    
    // both speakers are at the same position so it's not a pair
    bool empty() { return one.x == two.x && one.y == two.y; };
    bool exchanged(spair &other) { return one == other.two && two == other.one; };
};

struct xyz
{
    double x;
    double y;
    double z;
	xyz(double _x, double _y, double _z) { x = _x; y = _y; z = _z; }
	xyz() : xyz(0,0,0) { }
};

class Speakers
{
    // speaker coordinates x,y
    std::vector<xyz> xy;
    bool debug = false;
    double maxdist = 100;
    std::vector<speaker> cc;
    std::vector<spair> speakers;

public:
    Speakers(int dim) { xy.resize(dim); }
    void setDebug(bool d) { debug = d; }
    void setMaxDist(double d) { maxdist = d; }

    void findSpeakerPairs();

    xyz &getCoordinates(int idx) { return xy[idx]; };
	bool set(int idx, xyz &sp);
    
    int numPairs() { return speakers.size(); }
    spair &getPair(int idx) { return speakers[idx]; }
    
    bool distToXY(double d1, double d2, int n, int m, double *x, double *y);
    bool distToXY(double d1, double d2, double L, double *x, double *y);
    double pythagor(double d, double z);
    bool distOk(double &d, int n, double z);

	// This is the last index of speakers position loaded from configuration
	// It should match the number of channels in wav file
    int lastSpeaker = -1; // the default is stereo

    std::string toString();
};

#endif /* Speakers_hpp */

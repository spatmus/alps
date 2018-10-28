//
//  Speakers.cpp
//  alOF
//
//  Created by user on 03.08.2018.
//

#include "Speakers.hpp"
#include <math.h>
#include <iostream>
#include <strstream>

bool Speakers::set(int idx, xyz &sp)
{
	if (idx < 0 || idx >= xy.size()) return false;
	xy[idx] = sp;
	if (lastSpeaker < idx) lastSpeaker = idx;
	return true;
}


// Estimates the location x, y from distances d1,d2 to speakers
// located at 0,0 and L,0
bool Speakers::distToXY(double d1, double d2, double L, double *x, double *y)
{
    if (!L) return false;
    *x = (d1 * d1 + L * L - d2 * d2) / 2 / L;
    if (d1 >= fabs(*x))
    {
        *y = sqrt(d1 * d1 - *x * *x);
        return true;
    }
    return false;
}

// Estimates the location x, y from distances d1,d2 to speakers n and m
// located at xy[n] and xy[m], see above
bool Speakers::distToXY(double d1, double d2, int n, int m, double *x, double *y)
{
    double x1 = xy[n].x;
    double y1 = xy[n].y;
    double x2 = xy[m].x;
    double y2 = xy[m].y;
    double dx = x2 - x1;
    double dy = y2 - y1;
//    std::cout << dx << " " << dy << " " << std::endl;
    if (!dx && !dy) return false;
    double L = sqrt(dx * dx + dy * dy);
    double a = atan2(dy, dx);
    double X, Y;
    if (distToXY(d1, d2, L, &X, &Y))
    {
        float si = sinf(a);
        float co = cosf(a);
        *x = x1 + X * co - Y * si;
        *y = y1 + X * si + Y * co;
//        std::cout << X << " " << Y << " a " << a << " si " << si << " co " << co << std::endl;
        return true;
    }
    
    return false;
}

double Speakers::pythagor(double d, double z)
{
    if (fabs(d) < fabs(z))
    {
        if (debug) std::cout << "pythagor error " << d << " < " << z << std::endl;
        return 0;
    }
    return sqrt(d * d - z * z);
}

bool Speakers::distOk(double &d, int n, double z)
{
    if (d <= 0) return false;
    if (d > maxdist)
    {
        if (debug) std::cout << "speaker " << n << " is too far " << d << std::endl;
        return false;
    }
    if (debug) std::cout << "speaker: " << n << " dist: " << d << std::endl;
    d = pythagor(d, z);
    return true;
}

void Speakers::findSpeakerPairs()
{
    int n = lastSpeaker + 1;
    
    for (int i = 0; i < n; i++)
    {
        cc.push_back(speaker(xy[i].x, xy[i].y));
        cc[i].id = i;
    }
    
    for (int i = 0; i < n; i++)
    {
        speaker &a = cc[i];
        
        for (int j = 0; j < n; j++) if (i != j)
        {
            speaker &b = cc[j];
            spair pr(a, b);
            if (pr.empty())
            {
                std::cout << "ERROR Two speakers at the same position: " << i << " " << j << std::endl;
                continue;
            }
            
            bool ok = true;
            for (int k = 0; k < n; k++) if (i != k && j != k )
            {
                speaker &c = cc[k];
                float f = pr.ang(c);
                if (f < 0)
                {
                    ok = false;
                    break;
                }
            }
            
            if (ok)
            {
                a.used = true;
                b.used = true;
                speakers.push_back(pr);
            }
        }
    }
    
    // if all speakers are aligned then reversed pairs exist
    bool aligned = false;
    int np = (int)speakers.size();
    for (int i = 0; (i < np) && !aligned; i++)
    {
        spair &a = speakers[i];
        for (int j = i + 1; j < np; j++)
        {
            spair &b = speakers[j];
            if (b.exchanged(a))
            {
                aligned = true;
                break;
            }
        }
    }
    
    if (aligned)
    {
        std::cout << "ALIGNED SPEAKER PAIRS" << std::endl;
        speakers.clear();
        
        for (int i = 0; i < n; i++)
        {
            speaker &a = cc[i];
            for (int j = i + 1; j < n; j++)
            {
                speaker &b = cc[j];
                a.used = true;
                b.used = true;
                speakers.push_back(spair(a, b));
            }
        }
    }
    else
    {
        std::cout << "PAIRS" << std::endl;
    }
}

std::string Speakers::toString()
{
    if (!cc.size())
    {
        return "Speakers not configured in autopan mode";
    }
    std::ostrstream os;

    int n = lastSpeaker + 1;
    int np = speakers.size();
    for (int i = 0; i < np; i++)
    {
        speaker &a = speakers[i].one;
        speaker &b = speakers[i].two;
        os << i << " [" << a.id << "] (" << a.x << "," << a.y << ") ["
        << b.id << "] (" << b.x << "," << b.y << ")" << std::endl;
    }

    for (int i = 0; i < n; i++)
    {
        speaker a = cc[i];
        if (!a.used)
        {
            os << "NOT USED [" << i << "] (" << a.x << "," << a.y << ")" << std::endl;
        }
    }

    os << std::ends;
    std::string str = os.str();
    std::cout << str;
    return str;
}

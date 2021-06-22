/*
  ==============================================================================

    window.h
    Created: 22 Dec 2018 3:17:36pm
    Author:  Travis West

  ==============================================================================
*/

#pragma once
#include <cmath>
#include <../3rdparty/simplesound/simple/constants/pi.h>

namespace mubone
{

class Window 
{
public:
    void apply(
            float * destination, 
            int start, 
            int duration, 
            int numSamples,
            float amplitude = 1.0) const
    {
        //jassert(start + numSamples <= duration);
        for (int n = 0; n < numSamples; ++n)
        {
            int index = n + start;
            destination[n] = amplitude * destination[n] * hann(index, duration);
        }
    }

    float hann(int index, int duration) const
    {
        float t = index / static_cast<float>(duration - 1);
        float hann = 0.5 * (1 - std::cos(Simple::twoPi * t));
        return hann;
    }
};

} // namespace mubone

/*
  ==============================================================================

    grain.h
    Created: 17 Dec 2018 1:47:46pm
    Author:  Travis West

  ==============================================================================
*/

#pragma once

#include <../3rdparty/simplesound/simple/boundaries.h>

#include "soundobject.h"
#include "window.h"

#include "../JuceLibraryCode/JuceHeader.h"

namespace mubone
{

class SoundGrain
{
public:
    struct Parameters
    {
        ControlSoundReference ref;
        int l_channel;
        int r_channel;
        float pan;
        float playbackrate;
        int delay;
        int duration;
        float amplitude;
        Window window;
    };

    SoundGrain() : p{}, written{} {}

    SoundGrain(const Parameters& init) : p{init}, written{0}
    {
        const int mindur = p.ref.size() < 3 ? p.ref.size() : 3;
        p.duration = Simple::clip(p.duration, p.ref.size(), mindur);
        p.amplitude = Simple::clip(p.amplitude, 1.0f, 0.0f);
        p.pan = Simple::clip(p.pan, 0.0f, 1.0f);
    } 

    bool busy() const noexcept {return written < p.duration;}
    bool idle() const noexcept {return !busy();}

    void getNextAudioBlock
            ( juce::AudioBuffer<float>& buffer
            , int startsamp
            , int numsamples
            , Sound& workingbuffer
            ) 
    {
        if (idle()) return;

        if (p.delay >= numsamples) 
        {
            p.delay -= numsamples; 
            return;
        }
        else if (p.delay)
        {
            numsamples -= p.delay;
            startsamp += p.delay;
            p.delay = 0;
        }

        if (written + numsamples > p.duration) numsamples = p.duration - written;

        workingbuffer.copyFrom((p.ref + written).getReadPointer(), numsamples);
        p.window.apply(workingbuffer.getWritePointer(), written, p.duration, numsamples, p.amplitude);
        if (buffer.getNumChannels() < 2)
            buffer.addFrom(0, startsamp, workingbuffer.getReadPointer(), numsamples);
        else
        {
            buffer.addFrom(0, startsamp, workingbuffer.getReadPointer(), numsamples, p.pan);
            buffer.addFrom(1, startsamp, workingbuffer.getReadPointer(), numsamples, 1.0f - p.pan);
        }

        written += numsamples;
    }

private:
    // the params are conceptually constant;
    // they should be set when a grain is constructed and never subsequently modified
    Parameters p;

    // this keeps track of the lifetime / progress of the grain
    int written;
};

} // namespace mubone

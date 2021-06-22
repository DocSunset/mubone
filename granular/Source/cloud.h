/*
  ==============================================================================

    cloud.h
    Created: 22 Dec 2018 11:48:30am
    Author:  Travis West

  ==============================================================================
*/

#pragma once
#include "../JuceLibraryCode/JuceHeader.h"
#include "parameters.hpp"
#include "audiosphere.h"
#include "grain.h"
#include "window.h"
#include <../3rdparty/simplesound/simple/synchronoustrigger.h>
#include <../3rdparty/simplesound/simple/interpolators.h>

#include <array>

namespace mubone::synthesis
{

class GrainCloud
{
public:
    GrainCloud(Audiosphere& a) 
    : 
        grains{}, audiosphere{a}
    {
        initialize(prev_gd);
        initialize(gd);
    }

    void prepareToPlay(int samplesPerBlockExpected, int sampleRate) 
    {
        blocksize = samplesPerBlockExpected;
        samplerate = sampleRate;
        smoothing_time_samps = 0.005 * samplerate;
        trigger.sampling_rate.set_hz(samplerate);
    }

    void releaseResources() 
    { 
    }

    void getNextAudioBlock(const std::size_t& time, const AudioSourceChannelInfo& iobuffer, Sound& workingbuffer)
    {
        if (planted()) launchNewGrains(time, iobuffer.numSamples);
        for (auto& grain : grains) 
        {
            if (grain.idle()) continue;
            grain.getNextAudioBlock(*(iobuffer.buffer),
                                    iobuffer.startSample,
                                    iobuffer.numSamples,
                                    workingbuffer);
        }
    }

    bool busy() const
    {
        for (const auto& grain : grains) { if (grain.busy()) return true; }
        return false;
    }

    bool idle() const { return !busy(); }

    int   getNumCandidates(float searchdistance) const;
    bool  allGrainsBusy() const;
    int   getIdleGrain() const;
    int   getChannel() const;

    void  plant() {plantedflag = true;}
    void  grab() {plantedflag = false;}
    bool  planted() const { return plantedflag; }
    bool  grabbed() const { return !plantedflag; }

    void  update(const GrainDescription & g)
    {
        prev_gd = gd;
        gd = g;
        updated = true;
    }

    Vector normal() const { return get<direction>(gd); }

    static constexpr int numgrains = 32;
private:

    std::array<SoundGrain, numgrains> grains;
    Audiosphere& audiosphere;

    int blocksize;
    int samplerate;
    std::size_t smoothing_time_samps;

    GrainDescription prev_gd{};
    GrainDescription gd{};

    auto make_lines(const std::size_t& time)
    {
        return map(zip(prev_gd, gd), [&time, this](const auto& pair)
            {
                auto [prev_elem, next_elem] = pair;
                return Simple::LinearInterpolator(
                        time, prev_elem, 
                        time + smoothing_time_samps, next_elem);
            });
    }

    template<class ListOfLines>
    static GrainDescription lines_at(const ListOfLines& lines, const std::size_t& time)
    {
        return map( lines, [&time] (const auto& elem) {return elem.at(time);} );
    }


    Simple::SynchronousTrigger trigger;
    bool plantedflag = false;
    bool updated = false;
    std::size_t updated_time = 0;

    void launchNewGrains(const std::size_t& time, int numSamples);
    void launchNewGrains(int numSamples);

};

} // namespace mubone

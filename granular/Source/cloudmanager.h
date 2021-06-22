/*
  ==============================================================================

    cloudmanager.h
    Created: 17 Dec 2018 1:47:33pm
    Author:  Travis West

  ==============================================================================
*/

#pragma once

#include "../JuceLibraryCode/JuceHeader.h"
#include "audiosphere.h"
#include "cloud.h"
#include "repeatermacro.h"
#include <../3rdparty/simplesound/simple/boundaries.h>

namespace mubone::synthesis
{

class CloudManager
{
public:
    CloudManager(Audiosphere& a)
    :   clouds{REPEATX8(GrainCloud(a))}, cursor{GrainCloud(a)}
    {
        initialize(graindescription);
        for (auto& cloud : clouds) cloud.grab();
        cursor.plant();
    }

    void prepareToPlay(int samplesPerBlockExpected, int sampleRate)
    {
        workingbuffer = Sound(samplesPerBlockExpected);
        for (auto& cloud : clouds) cloud.prepareToPlay(samplesPerBlockExpected, sampleRate);
        cursor.prepareToPlay(samplesPerBlockExpected, sampleRate);
    }

    void releaseResources()
    {
        workingbuffer = Sound{};
        for (auto& cloud : clouds) cloud.releaseResources();
        cursor.releaseResources();
    }

    void getNextAudioBlock(
            const std::size_t& time,
            const AudioSourceChannelInfo& iobuffer)
    {
        for (auto& cloud : clouds)
        {
            if (cloud.grabbed())
            {
                cloud.update(graindescription); // I should not need to do this
                if (cloud.idle()) continue;
            }
            cloud.getNextAudioBlock(time, iobuffer, workingbuffer);
        }
        if (cursor.planted()) cursor.update(graindescription);
        if (cursor.planted() || cursor.busy())
            cursor.getNextAudioBlock(time, iobuffer, workingbuffer);
    }

    void plant() { changePlantedClouds(1); }
    void grab() { changePlantedClouds(-1); }
    void set_granulate_state(bool flag)
    {
        if (flag) cursor.plant();
        else cursor.grab();
    }

    void changePlantedClouds(int changeQuantity)
    {
        setPlantedClouds(numPlanted() + changeQuantity);
    }

    void setPlantedClouds(int newplanted)
    {
        newplanted = Simple::clip(newplanted, numclouds);
        int prevplanted = numPlanted();
        int truechange = newplanted - prevplanted;
        while (truechange > 0)
        {
            for (auto& cloud : clouds) if (cloud.grabbed())
            {
                cloud.update(graindescription); // this should be the only time I need to update these
                cloud.plant();
                --truechange;
                break;
            }
        }
        jassert(truechange <= 0);
        while (truechange < 0)
        {
            grabClosestCloud();
            ++truechange;
        }
        jassert(truechange == 0);
    }

    int numPlanted() const 
    {
        int a = 0;
        for (int i = 0; i < numclouds; ++i)
        {
            if (clouds[i].planted()) ++a;
        }
        return a;
    }

    bool cloudPlanted(int i) const {if (0 <= i && i < numclouds) return clouds[i].planted(); else return false;}
    const Vector& normal() const {return get<direction>(graindescription).value;}
    static constexpr int numclouds = 8;

    GrainDescription graindescription;

private:
    void grabClosestCloud()
    {
        jassert(numPlanted() > 0);
        float bestdistance = 99999;
        GrainCloud * closestcloud;
        bool match = false;
        for (auto& cloud : clouds)
        {
            if (cloud.grabbed()) continue;
            float distance = (normal() - cloud.normal()).squaredNorm();
            if (distance < bestdistance)
            {
                bestdistance = distance;
                closestcloud = &cloud;
                match = true;
            }
        }
        if (match) 
        {
            closestcloud->grab();
        }
    }

    std::array<GrainCloud, numclouds> clouds;
    Sound workingbuffer;
    GrainCloud cursor;
};

} // namespace mubone

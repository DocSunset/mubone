/*
  ==============================================================================

    soundobject.h
    Created: 19 Dec 2018 10:19:15am
    Author:  Travis West

  ==============================================================================
*/

#pragma once

#include <list>
#include <vector>
#include <memory>
#include <cmath>
#include <../3rdparty/simplesound/simple/constants/pi.h>

#include "linalgtypes.h"

#include "../JuceLibraryCode/JuceHeader.h"


namespace mubone
{

class Sound
{
public:
    using List = std::list<std::shared_ptr<Sound>>;

    Sound() : sizeinsamps{0}, buffer(0, 0) {}
    Sound(int size) : sizeinsamps{size}, buffer(1, sizeinsamps) {}

    int size() const {return sizeinsamps;}

    const float * getReadPointer(int index = 0) const 
    {
        return buffer.getReadPointer(0,index);
    }

    float * getWritePointer(int index = 0)
    {
        return buffer.getWritePointer(0,index);
    }

    void copyFrom(const float * readptr, int numsamples)
    {
        buffer.copyFrom(0, 0, readptr, numsamples);
    }

    void applyGain(const float gain)
    {
        buffer.applyGain(gain);
    }

    float at(int index) const { return buffer.getSample(0,index); }

private:
    int sizeinsamps;
    juce::AudioBuffer<float> buffer;
};

struct ControlSoundReference
{
    ControlSoundReference() : normal{}, index{}, sound{} {}
    ControlSoundReference(const Vector& vec,
                          const int& i,
                          const std::shared_ptr<Sound>& ptr)
        :
        normal(vec), index{i}, sound{ptr}
    {}

    float arcDistanceTo(const Vector& compnormal)
    {
        if ((compnormal - cachednormal).squaredNorm() < 0.01)
        {
            return cacheddist;
        }
        jassert(normal.norm() - 1 < 0.001);
        jassert(compnormal.norm() - 1 < 0.001);
        //float crossmagn = normal.cross(compnormal).squaredNorm();
        //float dotprod   = normal.dot(compnormal);
        //return std::atan2(crossmagn, dotprod);
        //
        // use a cheaper linear metric instead for now
        cachednormal = compnormal;
        cacheddist = (normal - compnormal).squaredNorm() * Simple::pi / 4.0f;
        return cacheddist;
    }

    float getCachedDistance() const {return cacheddist;}

    const float * getReadPointer(int indexoffset = 0) const 
    {
        return sound->getReadPointer(index + indexoffset);
    }

    float * getWritePointer(int indexoffset = 0)
    {
        return sound->getWritePointer(index + indexoffset);
    }

    float at(int indexoffset = 0) const { return sound->at(index + indexoffset); }

    int size() const { return sound->size() - index; }

    const Sound& operator*() const {return *sound;}
          Sound& operator*()       {return *sound;}
    const std::shared_ptr<Sound> operator->() const {return sound;}
          std::shared_ptr<Sound> operator->()       {return sound;}

    ControlSoundReference operator+(int offset) const
    {
        return ControlSoundReference(normal, offsetIndex(offset), sound);
    }

    ControlSoundReference& operator++()
    {
        index = offsetIndex(1);
        return *this;
    }

    ControlSoundReference operator++(int)
    {
        auto ret = *this;
        index = offsetIndex(1);
        return ret;
    }

    ControlSoundReference& operator+=(int offset)
    {
        index = offsetIndex(offset);
        return *this;
    }

    ControlSoundReference operator-(int offset) const
    {
        return ControlSoundReference(normal, offsetIndex(-offset), sound);
    }

    ControlSoundReference& operator--()
    {
        index = offsetIndex(-1);
        return *this;
    }

    ControlSoundReference operator--(int)
    {
        auto ret = *this;
        index = offsetIndex(-1);
        return ret;
    }

    ControlSoundReference& operator-=(int offset)
    {
        index = offsetIndex(-offset);
        return *this;
    }

    const Vector& getNormal() const {return normal;}
    int getIndex() const {return index;}
    std::shared_ptr<Sound> pointer() {return sound;}

    operator bool() const {return static_cast<bool>(sound);}

private:
    int offsetIndex(int offset) const
    {
        int newindex = index + offset;
        if (newindex < 0) newindex = 0;
        else if (newindex >= sound->size()) newindex = sound->size() - 1;
        return newindex;
    }

    Vector normal;
    int index;
    std::shared_ptr<Sound> sound;
    float cacheddist;
    Vector cachednormal;

    friend class SoundObjectBuilder;
};

class ControlSoundBoundingBox
{
private:
    std::vector<ControlSoundReference> references;
    std::vector<int> indices;
    int candidates = 0;
    Vector cached_normal = Vector::Zero();
    float cached_radius = 0;
    static int random_index(int exclusive_max)
    {
        static juce::Random r;
        return r.nextInt(exclusive_max);
    }
};

} // namespace mubone

#pragma once

#include <list>
#include <vector>
#include <algorithm>
#include <utility>
#include <memory>

#include "../3rdparty/readerwriterqueue/readerwriterqueue.h"
#include "../3rdparty/readerwriterqueue/atomicops.h"
#include <../3rdparty/simplesound/simple/circularbuffer.h>

#include "linalgtypes.h"
#include "soundobject.h"



namespace mubone
{

// this struct holds pointers to sources of data from which to copy a sound and build
// control to sound references to it. It is cheap to copy.
struct SoundObjectBuilder
{
    int startsamp = 0;
    int startblock = 0;
    int numsamples = 0;
    int numblocks = 0;

    std::shared_ptr<Simple::CircularBuffer<float>> audiobuffer{};
    std::shared_ptr<Simple::CircularBuffer<std::pair<std::size_t, Vector>>> ctrlbuffer{};

    std::shared_ptr<Sound> buildSound() const;
    std::vector<ControlSoundReference>& buildReferences(
                                    const std::shared_ptr<Sound>& newsound, 
                                    std::vector<ControlSoundReference>& references) const;
};

class SoundObjectBuilderThread : public juce::Thread
{
public:
    SoundObjectBuilderThread()
    : 
        Thread("Sound Object Allocator"),
        jobrequests(queuesize),
        garbage(queuesize)
    {
        pause.store(false);
        clear_flag.store(false);
    }

    bool submitJob(const SoundObjectBuilder& request);

    void run() override
    { 
        while (true) { if (threadShouldExit()) return;
            cleanUpGarbage();
            if (clear_flag.load()) clear();
            if (!pause.load()) processJobs(); 
        } 
        yield();
    }

    bool getSound(Sound::List& output); 
    bool getReferences(std::vector<ControlSoundReference>& output, std::vector<int>& output_indices);

    std::atomic<bool> pause;
    std::atomic<bool> clear_flag;
private:
    void processJobs();
    void cleanUpGarbage();
    void clear()
    {
        SoundObjectBuilder dummy;
        while (jobrequests.try_dequeue(dummy)) continue;
        references.clear();
        reference_indices.clear();
        pause.store(false);
        clear_flag.store(false);
    }

    std::vector<ControlSoundReference> references;
    std::vector<int> reference_indices;

    // input queues
    moodycamel::ReaderWriterQueue<SoundObjectBuilder> jobrequests;
    moodycamel::ReaderWriterQueue<std::vector<ControlSoundReference>> garbage;
    moodycamel::ReaderWriterQueue<std::vector<int>> garbage_indices;

    // output queues
    moodycamel::ReaderWriterQueue<std::shared_ptr<Sound::List>> finishedsounds;
    moodycamel::ReaderWriterQueue<std::vector<ControlSoundReference>> finishedreferences;
    moodycamel::ReaderWriterQueue<std::vector<int>> finishedindices;

    static const int queuesize = 10;
};

class SoundObjectList
{
public:
    typedef std::vector<ControlSoundReference>::iterator iterator;

    SoundObjectList()
    {
        builderthread.startThread(7);
    }

    ~SoundObjectList()
    {
        builderthread.signalThreadShouldExit();
        builderthread.notify();
        /*bool smoothexit = */builderthread.waitForThreadToExit(1000);
        //jassert(smoothexit);
    }

    bool addSound(const SoundObjectBuilder& request)
    { 
        return builderthread.submitJob(request); 
    }

    bool update()
    {
        bool ret = false;
        ret = builderthread.getSound(sounds) || ret;
        ret = builderthread.getReferences(references, reference_indices) || ret;
        return ret;
    }

    void clear()
    {
        builderthread.pause.store(true);
        while(update()) continue; // clears builder output queues
        sounds.clear();
        references.clear();
        reference_indices.clear();
        builderthread.clear_flag.store(true); // builder will clear input queues and destroy all sound refs
    }

    Sound::List& getSounds()    { return sounds; }
    std::size_t numSounds()     { return sounds.size(); }
    std::size_t numReferences() { return references.size(); }

    const ControlSoundReference& at(int i) const { return references.at(i); }
          ControlSoundReference& at(int i)       { return references.at(i); }
    const ControlSoundReference& operator[](int i) const { return references[i]; }
          ControlSoundReference& operator[](int i)       { return references[i]; }

    iterator begin() {return references.begin();}
    iterator end() {return references.end();}

    void prepare_candidates(const Vector& v, const float& search_radius);
    ControlSoundReference random_candidate();

private:
    SoundObjectBuilderThread builderthread;
    Sound::List sounds;
    std::vector<ControlSoundReference> references;
    std::vector<int> reference_indices;
    Random r;
    int candidates = 0;
    Vector cached_normal = Vector::Zero();
    float cached_radius = 0;
    std::size_t cached_num_references = 0;
};

} // namespace mubone

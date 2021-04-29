#include "soundobjectlist.h"

namespace mubone
{

// SoundObjectBuilder ///////////////////////////////////////////////////
// these methods should never be called on the audio thread /////////////

std::shared_ptr<Sound> SoundObjectBuilder::buildSound() const
{
    if (!audiobuffer) return std::shared_ptr<Sound>{};
    auto newsound = std::make_shared<Sound>(numsamples); // allocates numsamples on heap
    audiobuffer->copyTo(newsound->getWritePointer(),
                        startsamp, 
                        numsamples);
    return newsound;
}

std::vector<ControlSoundReference>& SoundObjectBuilder::buildReferences(
                                    const std::shared_ptr<Sound>& newsound, 
                                    std::vector<ControlSoundReference>& references) const
{
    if (!ctrlbuffer) return references;
    std::size_t sampleindexoffset = ctrlbuffer->at(startblock).first;
    for (int i = 0; i < numblocks; ++i)
    {
        ControlSoundReference ref;
        auto [t, v] = ctrlbuffer->at(startblock + i);
        ref.normal = v;
        ref.index = static_cast<int>(t - sampleindexoffset);
        ref.sound = newsound;
        references.push_back(ref); // likely to allocate now and then
    }
    return references;
}

// SoundObjectBuilderThread /////////////////////////////////////////////

// These methods are only ever called on the builder thread /////////////

void SoundObjectBuilderThread::processJobs()
{
    SoundObjectBuilder request;
    while (jobrequests.try_dequeue(request))
    {
        auto sound = request.buildSound();
        bool success = finishedreferences.enqueue(request.buildReferences(sound, references)); // copy
        jassert(success);
        jassert(references.size() > 0);
        success = finishedindices.enqueue(std::vector<int>(references.size(), 0));
        jassert(success);

        auto soundlist = std::make_shared<Sound::List>();
        soundlist->push_back(sound);
        success = finishedsounds.enqueue(std::move(soundlist));
        jassert(success);
    }
    wait(250);
    return;

}

void SoundObjectBuilderThread::cleanUpGarbage()
{
    std::vector<ControlSoundReference> g;
    while (garbage.try_dequeue(g))
    {
        g.clear();
    }
    std::vector<int> i;
    while (garbage_indices.try_dequeue(i))
    {
        i.clear();
    }
}

// these methods should be safe to call on the audio thread /////////////

bool SoundObjectBuilderThread::submitJob(const SoundObjectBuilder& request)
{ 
    bool success = jobrequests.try_enqueue(request); // copy some ints and ptrs, cheap, never allocates
    jassert(success);
    this->notify();
    return success;
}

bool SoundObjectBuilderThread::getReferences(
        std::vector<ControlSoundReference>& output, 
        std::vector<int>& output_indices)
{
    bool success = false;
    std::vector<ControlSoundReference> temp;
    std::vector<int> temp_indices;
    while (finishedreferences.try_dequeue(temp)); // dequeue uses move, should be cheap
    while (finishedindices.try_dequeue(temp_indices)); // dequeue uses move, should be cheap
    if (temp.size())
    {
        std::swap(output, temp); // move move move!
        success = garbage.try_enqueue(std::move(temp)); // builder will free old vec
    }
    if (temp_indices.size())
    {
        jassert(temp_indices.size() > output_indices.size());
        std::swap(output_indices, temp_indices); // move move move!
        int j = 0;
        for (int i : temp_indices) // for i in old indicies
        {
            // manually copy old indices to output vector
            jassert(j < output_indices.size());
            output_indices[j] = i;
            ++j;
        }
        success = garbage_indices.try_enqueue(std::move(temp_indices)) && success;
    }
    return success;
}

bool SoundObjectBuilderThread::getSound(Sound::List& sounds)
{ 
    std::shared_ptr<Sound::List> newsound;
    while (finishedsounds.try_dequeue(newsound)) // copy a ptr (cheap)
    {
        sounds.splice(sounds.end(), *newsound);
    }
    if (newsound) return true;
    else return false;
}

// SoundObjectList //////////////////////////////////////////////////////
// lives on the audio thread ////////////////////////////////////////////

void SoundObjectList::prepare_candidates(
        const Vector& v, 
        const float& search_radius)
{
    if (!references.size() || !reference_indices.size()) 
    {
        jassert(false);
        return;
        // this should never happen, because prepare_candidates should never be
        // called before there are any sound references. 
    }
    if (references.size() != reference_indices.size()) jassert(false); 
        // these should always have the same size. If they don't though, we should be able to recover...

    auto num_refs = numReferences();
    if (  cached_num_references == num_refs
       && search_radius - cached_radius < 0.01
       && (v - cached_normal).squaredNorm() < 0.01)
        return;

    // update cache
    cached_radius = search_radius;
    cached_normal = v;
    cached_num_references = num_refs;

    // collect candidates
    candidates = 0;
    int i = 0;
    for (auto& ref : references)
    {
        if (ref.arcDistanceTo(v) < search_radius)
        {
            reference_indices[candidates] = i;
            ++candidates;
            if (candidates > reference_indices.size())
            {
                jassertfalse;
                return;
            }
                // this should never happen, but if it does at least it is
                // guaranteed that candidates is greater than zero, the program
                // won't crash, and a random_candidate will still be selected. 
        }
        ++i;
    }
}

ControlSoundReference SoundObjectList::random_candidate()
{
    if (!candidates) return ControlSoundReference{};
    int random_sound_index = r.nextInt(candidates);
    jassert(0 <= random_sound_index);
    jassert(random_sound_index < reference_indices.size());
    return references.at(reference_indices[random_sound_index]);
}
} // namespace mubone

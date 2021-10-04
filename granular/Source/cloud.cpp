#include "cloud.h"

#include "../3rdparty/simplesound/simple/random.h"

namespace mubone::synthesis
{

void GrainCloud::launchNewGrains(const std::size_t& time, int numSamples)
{
    // need at least one candidate sound reference
    if (audiosphere.numReferences() == 0) return; 
    // no voice stealing... for now
    if (allGrainsBusy()) return;
    
    if (updated) 
    {
        updated_time = time;
        updated = false;
    }
    if (time > updated_time + smoothing_time_samps)
        return launchNewGrains(numSamples);

    auto lines = make_lines(updated_time);
    auto start = lines_at(lines, time);
    auto end = lines_at(lines, time + numSamples);
    auto start_amp = get<amplitude>(start);
    auto end_amp = get<amplitude>(end);
    auto start_freq = get<frequency>(start);
    auto end_freq = get<frequency>(end);
    
    // no need to bother launching grains if they will be silent
    if (  start_amp == 0 && end_amp == 0) return;
    // no need to run the trigger if the frequency is zero
    if (  start_freq == 0 && end_freq == 0) return;

    bool sorted = false;
    for (int i = 0; i < numSamples; ++i)
    {
        GrainDescription g = lines_at(lines, time + i);
        
        float _activation_probability = (float)get<activation_probability>(g);
        float _frequency = (float)get<frequency>(g);
        trigger.frequency.set_hz(_frequency);
        if (trigger.tick() && Simple::Random<float>::unipolar() < _activation_probability)
        {
            int index = getIdleGrain();
            if (index < 0 || index > numgrains) 
            {
                jassert(false); // this should never happen
                return;
            }
            if (!sorted)
            {
                audiosphere.update();
                audiosphere.prepare_candidates(get<direction>(g), get<search_radius>(g));
                sorted = true;
            }

            auto sound = audiosphere.random_candidate();
            if (!sound) continue;

            int _channel         = getChannel();
            float _playback_rate = (float)get<playback_rate>(g);
            float _duration      = (float)get<duration>(g) * samplerate;
            float _amplitude     = (float)get<amplitude>(g);
            float _skew          = (float)get<skew>(g);
            
            SoundGrain::Parameters p;
            p.ref = sound;
            p.channel = _channel;
            p.playbackrate = _playback_rate;
            p.delay = i;
            p.duration = _duration;
            p.amplitude = _amplitude;
            p.window = Window{_skew};

            jassert(index >= 0);
            grains[index] = SoundGrain(p);

            if (allGrainsBusy()) return;
        }
    }
}

void GrainCloud::launchNewGrains(int numSamples)
{
    // TODO: combine this and the previous method
    auto _amplitude = get<amplitude>(gd);
    auto _frequency = get<frequency>(gd);
    if (_amplitude == 0 || _frequency == 0) return;
    int _channel         = getChannel();
    float _playback_rate = (float)get<playback_rate>(gd);
    float _duration      = (float)get<duration>(gd) * samplerate;
    float _activation_probability = (float)get<activation_probability>(gd);
    float _skew = (float)get<skew>(gd);

    trigger.frequency.set_hz(_frequency);
    bool sorted = false;
    for (int i = 0; i < numSamples; ++i)
    {
        if (trigger.tick() && Simple::Random<float>::unipolar() < _activation_probability)
        {
            int index = getIdleGrain();
            if (index < 0 || index > numgrains) 
            {
                jassert(false); // this should never happen
                return;
            }

            if (!sorted)
            {
                audiosphere.update();
                audiosphere.prepare_candidates(get<direction>(gd), get<search_radius>(gd));
                sorted = true;
            }

            auto sound = audiosphere.random_candidate();
            if (!sound) continue;

            SoundGrain::Parameters p;
            p.ref = sound;
            p.channel = _channel;
            p.playbackrate = _playback_rate;
            p.delay = i;
            p.duration = _duration;
            p.amplitude = _amplitude;
            p.window = Window{_skew};

            jassert(index >= 0);
            grains[index] = SoundGrain(p);

            if (allGrainsBusy()) return;
        }
    }
}

int   GrainCloud::getChannel() const {return 0;}

bool GrainCloud::allGrainsBusy() const
{
    return getIdleGrain() == -1;
}

int GrainCloud::getIdleGrain() const
{
    for (int i = 0; i < numgrains; ++i) {
        if (grains[i].idle()) return i;
    }
    return -1;
}
} // namespace mubone

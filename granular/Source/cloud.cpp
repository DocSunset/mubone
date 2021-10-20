#include "cloud.h"

#include "../3rdparty/simplesound/simple/random.h"
#include "../3rdparty/simplesound/simple/constants/pi.h"
#include "../3rdparty/simplesound/simple/boundaries.h"
#include "parameter_mapping.h"

namespace
{
    template<typename Signal>
    float spray(const Signal& signal, float s)
    {
        return s * Simple::Random<float>::in_range(signal.min(), signal.max())
               + (1 - s) * signal.value;
    }
}

namespace mubone::synthesis
{

auto GrainCloud::getChannel(const GrainDescription& gd, const ControlSoundReference& ref) const
{
    auto width = 2 * get<spatial_width>(gd).value;
    auto spray = get<spatial_spray>(gd).value;

    if (width <= 0.0f) return std::make_tuple(0, 1, 0.5f);

    auto directional_pan = ref.getNormal().x();
    auto random_pan = Simple::Random<float>::in_range(-1, 1);
    float pan = spray * random_pan + (1 - spray) * directional_pan;

    // https://www.desmos.com/calculator/gdcmisuiu3
    if (width > 1)
    {
        auto z = Simple::clip(width - 1.0);
        auto s = 1.0f - z;
        if (std::abs(pan) > s)
            pan = pan > 0 ? 1 : -1;
        else
            pan = pan > 0 ? pan + z : pan - z;
    }
    else
    {
        pan = pan * width;
    }
    pan = Simple::clip(0.5 * pan + 0.5);
    return std::make_tuple(0, 1, pan);
}

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
    auto start_amp_min = get<amplitude_min>(start);
    auto start_amp_max = get<amplitude_max>(start);
    auto end_amp_min = get<amplitude_min>(end);
    auto end_amp_max = get<amplitude_max>(end);
    
    // no need to bother launching grains if they will be silent
    if (  start_amp_min == 0 && end_amp_min == 0
       && start_amp_max == 0 && end_amp_max == 0
       ) return;

    bool sorted = false;
    for (int i = 0; i < numSamples; ++i)
    {
        GrainDescription g = lines_at(lines, time + i);
        
        float _activation_probability = (float)get<activation_probability>(g);
        trigger.min_period.set_hz(frequency_mapping(get<frequency_min>(g)));
        trigger.max_period.set_hz(frequency_mapping(get<frequency_max>(g)));
        if (trigger.tick() && Simple::Random<float>::unipolar() < _activation_probability)
        {
            float _amp_base = Simple::Random<float>::in_range(get<amplitude_min>(g), get<amplitude_max>(g));
            if (_amp_base == 0) continue;
            float _amplitude = std::pow(_amp_base, get<amplitude_curve>(g) * 10.0f);
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

            auto [_l_channel, _r_channel, _pan] = getChannel(g, sound);
            float _playback_rate = (float)get<playback_rate>(g);
            auto _duration      = get<duration_min>(g);
            float _duration_spray = get<duration_max>(gd);
            float _skew          = Simple::Random<float>::in_range(get<skew_min>(g), get<skew_max>(g));
            
            SoundGrain::Parameters p;
            p.ref = sound;
            p.l_channel = _l_channel;
            p.r_channel = _r_channel;
            p.pan = _pan;
            p.playbackrate = _playback_rate;
            p.delay = i;
            p.duration = duration_mapping(Simple::Random<float>::in_range(_duration, _duration_spray)) * samplerate;
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
    if (get<amplitude_min>(gd) == 0 && get<amplitude_max>(gd) == 0) return;
    float _playback_rate = (float)get<playback_rate>(gd);
    auto _duration_min = get<duration_min>(gd);
    auto _duration_max = get<duration_max>(gd);
    float _activation_probability = (float)get<activation_probability>(gd);
    trigger.min_period.set_hz(frequency_mapping(get<frequency_min>(gd)));
    trigger.max_period.set_hz(frequency_mapping(get<frequency_max>(gd)));

    bool sorted = false;
    for (int i = 0; i < numSamples; ++i)
    {
        if (trigger.tick() && Simple::Random<float>::unipolar() < _activation_probability)
        {
            float _amp_base = Simple::Random<float>::in_range(get<amplitude_min>(gd), get<amplitude_max>(gd));
            if (_amp_base == 0) continue;
            float _amplitude = std::pow(_amp_base, get<amplitude_curve>(gd) * 10.0f);
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

            auto [_l_channel, _r_channel, _pan] = getChannel(gd, sound);
            SoundGrain::Parameters p;
            p.ref = sound;
            p.l_channel = _l_channel;
            p.r_channel = _r_channel;
            p.pan = _pan;
            p.playbackrate = _playback_rate;
            p.delay = i;
            p.duration = duration_mapping(Simple::Random<float>::in_range(_duration_min, _duration_max)) * samplerate;
            p.amplitude = _amplitude;
            float _skew          = Simple::Random<float>::in_range(get<skew_min>(gd), get<skew_max>(gd));
            p.window = Window{_skew};

            jassert(index >= 0);
            grains[index] = SoundGrain(p);

            if (allGrainsBusy()) return;
        }
    }
}

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

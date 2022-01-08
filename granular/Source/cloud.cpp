#include "cloud.h"

#include "../3rdparty/simplesound/simple/random.h"
#include "../3rdparty/simplesound/simple/frequency.h"
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
    auto width = get<spatial_width>(gd).value;
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

    auto lines = make_lines(updated_time);
    auto start = lines_at(lines, time);
    auto end = lines_at(lines, time + numSamples);
    auto start_amp_min = get<amplitude_min>(start).value;
    auto start_amp_max = get<amplitude_max>(start).value;
    auto end_amp_min = get<amplitude_min>(end).value;
    auto end_amp_max = get<amplitude_max>(end).value;
    
    // no need to bother launching grains if they will be silent
    if (  start_amp_min <= -127 && end_amp_min <= -127
       && start_amp_max <= -127 && end_amp_max <= -127
       ) return;

    bool sorted = false;
    for (int i = 0; i < numSamples; ++i)
    {
        GrainDescription g = lines_at(lines, time + i);
        
        float _activation_probability = (float)get<activation_probability>(g);
        // ensure valid frequency to avoid locking up the trigger
        float fmin = Simple::clip(get<frequency_min>(g).value, 0.0001f, 96000.0f);
        float fmax = Simple::clip(get<frequency_max>(g).value, 0.0001f, 96000.0f);
        trigger.min_period.set_hz(fmin);
        trigger.max_period.set_hz(fmax);
        if (trigger.tick() && Simple::Random<float>::unipolar() < _activation_probability)
        {
            float _amp_db = Simple::Random<float>::in_range(get<amplitude_min>(g), get<amplitude_max>(g));
            if (_amp_db <= -127) continue;
            float _amplitude = std::pow(10, _amp_db / 20.f);
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
            float _duration      = get<duration_min>(g);
            float _duration_spray = get<duration_max>(gd);
            float _skew          = Simple::Random<float>::in_range(get<skew_min>(g), get<skew_max>(g));
            
            SoundGrain::Parameters p;
            p.ref = sound;
            p.l_channel = _l_channel; // if the channels are misconfigured it could wreak havoc
            p.r_channel = _r_channel;
            p.pan = _pan; // clipped by grain
            p.playbackrate = _playback_rate; // not yet implemented
            p.delay = i; // must be positive or zero; satisfied by construction
            float dminmidi = Simple::hz_to_midi(1000.0f / get<duration_min>(g));
            float dmaxmidi = Simple::hz_to_midi(1000.0f / get<duration_max>(g));
            p.duration = std::ceil(samplerate / Simple::midi_to_hz(Simple::Random<float>::in_range(dminmidi, dmaxmidi))); // clipped by grain
            p.amplitude = _amplitude; // clipped by grain
            p.window = Window{Simple::clip(_skew, 0.0f, 1.0f)};

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

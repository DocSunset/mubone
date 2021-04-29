#ifdef RUNTESTS
#include "catch.hpp"
#include "../Source/grain.h"
#include <memory>
#include <iostream>

using namespace mubone;

TEST_CASE("A sound grain windows samples from a sound")
{
    int outputchannel = 0;
    float playbackrate = 1;
    int maxdelay = 20;
    int duration = 15;
    float amplitude = 1;
    auto sound = std::make_shared<Sound>(duration);
    Sound workingbuffer(duration + maxdelay);
    ControlSoundReference ref(Vector(0, 0, 0), 0, sound);
    Window w;
    AudioBuffer<float> buffer(1, duration + maxdelay);

    for (int i = 0; i < duration; ++i) sound->getWritePointer()[i] = 1;
    buffer.clear();

    SECTION("All in one go")
    {
        int startsamp = 0;

        SECTION("Without delay")
        {
            int delay = 0;
            SoundGrain g(SoundGrain::Parameters{ref, outputchannel, playbackrate, delay, duration, amplitude, w});
            g.getNextAudioBlock(buffer, startsamp, duration, workingbuffer);
            REQUIRE(buffer.getSample(0,0)            == Approx(w.hann(0, duration)));
            REQUIRE(buffer.getSample(0,duration / 4) == Approx(w.hann(duration / 4, duration)));
            REQUIRE(buffer.getSample(0,duration / 2) == Approx(w.hann(duration / 2, duration)));
            REQUIRE(buffer.getSample(0,duration - 1) == Approx(w.hann(duration - 1, duration)));
        }

        SECTION("With some delay")
        {
            int delay = maxdelay / 2;
            SoundGrain g(SoundGrain::Parameters{ref, outputchannel, playbackrate, delay, duration, amplitude, w});
            g.getNextAudioBlock(buffer, startsamp, duration + maxdelay, workingbuffer);
            REQUIRE(buffer.getSample(0,0)            == Approx(w.hann(0, duration)));
            REQUIRE(buffer.getSample(0,delay-1)   == Approx(w.hann(0, duration)));
            REQUIRE(buffer.getSample(0,delay + duration / 4) == Approx(w.hann(duration / 4, duration)));
            REQUIRE(buffer.getSample(0,delay + duration / 2) == Approx(w.hann(duration / 2, duration)));
            REQUIRE(buffer.getSample(0,delay + duration - 1) == Approx(w.hann(duration - 1, duration)));
        }

        SECTION("With max delay")
        {
            int delay = maxdelay;
            SoundGrain g(SoundGrain::Parameters{ref, outputchannel, playbackrate, delay, duration, amplitude, w});
            g.getNextAudioBlock(buffer, startsamp, duration + maxdelay, workingbuffer);
            REQUIRE(buffer.getSample(0,0)            == Approx(w.hann(0, duration)));
            REQUIRE(buffer.getSample(0,delay-1)   == Approx(w.hann(0, duration)));
            REQUIRE(buffer.getSample(0,delay + duration / 4) == Approx(w.hann(duration / 4, duration)));
            REQUIRE(buffer.getSample(0,delay + duration / 2) == Approx(w.hann(duration / 2, duration)));
            REQUIRE(buffer.getSample(0,delay + duration - 1) == Approx(w.hann(duration - 1, duration)));
        }
    }

    SECTION("All in smaller blocks")
    {
        int numblocks = 3;
        int blocksize = duration / numblocks;
        int startsamp = 0;

        SECTION("Without delay")
        {
            int delay = 0;
            SoundGrain g(SoundGrain::Parameters{ref, outputchannel, playbackrate, delay, duration, amplitude, w});

            while (g.busy()) 
            {
                g.getNextAudioBlock(buffer, startsamp, blocksize, workingbuffer);
                startsamp += blocksize;
            }

            REQUIRE(buffer.getSample(0,0)            == Approx(w.hann(0, duration)));
            REQUIRE(buffer.getSample(0,duration / 4) == Approx(w.hann(duration / 4, duration)));
            REQUIRE(buffer.getSample(0,duration / 2) == Approx(w.hann(duration / 2, duration)));
            REQUIRE(buffer.getSample(0,duration - 1) == Approx(w.hann(duration - 1, duration)));
        }

        SECTION("With some delay")
        {
            int delay = maxdelay / 2;
            SoundGrain g(SoundGrain::Parameters{ref, outputchannel, playbackrate, delay, duration, amplitude, w});

            while (g.busy()) 
            {
                g.getNextAudioBlock(buffer, startsamp, blocksize, workingbuffer);
                startsamp += blocksize;
            }

            REQUIRE(buffer.getSample(0,0)            == Approx(w.hann(0, duration)));
            REQUIRE(buffer.getSample(0,delay-1)   == Approx(w.hann(0, duration)));
            REQUIRE(buffer.getSample(0,delay + duration / 4) == Approx(w.hann(duration / 4, duration)));
            REQUIRE(buffer.getSample(0,delay + duration / 2) == Approx(w.hann(duration / 2, duration)));
            REQUIRE(buffer.getSample(0,delay + duration - 1) == Approx(w.hann(duration - 1, duration)));
        }

        SECTION("With max delay")
        {
            int delay = maxdelay;
            SoundGrain g(SoundGrain::Parameters{ref, outputchannel, playbackrate, delay, duration, amplitude, w});

            while (g.busy()) 
            {
                g.getNextAudioBlock(buffer, startsamp, blocksize, workingbuffer);
                startsamp += blocksize;
            }

            REQUIRE(buffer.getSample(0,0)            == Approx(w.hann(0, duration)));
            REQUIRE(buffer.getSample(0,delay-1)   == Approx(w.hann(0, duration)));
            REQUIRE(buffer.getSample(0,delay + duration / 4) == Approx(w.hann(duration / 4, duration)));
            REQUIRE(buffer.getSample(0,delay + duration / 2) == Approx(w.hann(duration / 2, duration)));
            REQUIRE(buffer.getSample(0,delay + duration - 1) == Approx(w.hann(duration - 1, duration)));
        }
    }

}
#endif

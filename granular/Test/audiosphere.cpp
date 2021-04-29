#ifdef RUNTESTS
#include "catch.hpp"
#include "../Source/audiosphere.h"
#include <chrono>
#include <thread>

using namespace mubone;

void makeSineWithRMS(float * dest, int numsamples, float rmsamp)
{
    constexpr float omega = 6.28 * 880 / 44100;
    float amp = rmsamp * std::sqrt(2.0);
    for (int samp = 0; samp < numsamples; ++samp)
    {
        dest[samp] = std::sin(omega * samp) * amp;
    }
}

TEST_CASE("Audiosphere getRMS gets the expected RMS amplitude")
{
    const int numsamples = 44100;
    float buffer[numsamples] = {};

    SECTION("RMS of silence = 0")
    {
        double rms = 0;
        for (int samp = 0; samp < numsamples; ++samp)
            buffer[samp] = rms;
        REQUIRE(0.0 == Approx(Audiosphere::getRMS(buffer, numsamples)));
    }

    SECTION("RMS of a constant = the constant")
    {
        double rms = 1;
        for (int samp = 0; samp < numsamples; ++samp)
            buffer[samp] = rms;
        REQUIRE(rms == Approx(Audiosphere::getRMS(buffer, numsamples)));
    }

    SECTION("RMS of A * sin(omega * t) = A / sqrt(2)")
    {
        double A = 1.0;
        double expect = A / std::sqrt(2.0);
        makeSineWithRMS(buffer, numsamples, expect);
        auto out = Approx(Audiosphere::getRMS(buffer, numsamples));
        REQUIRE(expect == out.epsilon(0.0001));
    }
}

//TEST_CASE("audiosphere records sounds into separate sound objects")
//{
//    const int blocksize = 32;
//    const int samplerate = 44100;
//    const int numblocks = 4;
//    const int blockswithsound = 2;
//    const float thresh = 0.1;
//
//    float * input = new float[blocksize * numblocks];
//    for (int samp = 0; samp < blocksize * numblocks; ++samp) input[samp] = 0;
//    makeSineWithRMS(input + blocksize,
//                    blocksize * blockswithsound,
//                    0.707);
//
//    ControlSampleFusion f{};
//    Audiosphere as{};
//    as.noisethreshold = thresh;
//    as.prepareToPlay(blocksize, samplerate);
//
//    REQUIRE(as.readyToPlay());
//
//    SECTION("leading silence is ignored")
//    {
//        as.record(input, blocksize, f);
//        CHECK_FALSE(as.thresholdPassed());
//        CHECK_FALSE(as.recording());
//
//        SECTION("when the sound starts, audiosphere begins recording")
//        {
//            as.record(input+blocksize, blocksize, f);
//            CHECK(as.thresholdPassed());
//            CHECK(as.recording());
//
//            SECTION("as the sound continues, audiosphere continues recording")
//            {
//                as.record(input+2*blocksize, blocksize, f);
//                CHECK(as.thresholdPassed());
//                CHECK(as.recording());
//
//                SECTION("when the sound stops, it gets copied to an object")
//                {
//                    as.record(input+3*blocksize, blocksize, f);
//                    CHECK_FALSE(as.thresholdPassed());
//                    CHECK_FALSE(as.recording());
//
//                    using std::this_thread::sleep_for;
//                    using namespace std::chrono_literals;
//                    sleep_for(100ms);
//
//                    as.update();
//                    REQUIRE(as.numSounds() == 1);
//                }
//            }
//        }
//    }
//
//    as.releaseResources();
//}
#endif

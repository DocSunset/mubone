#ifdef RUNTESTS
#include "catch.hpp"
#include "../Source/soundobjectlist.h"
#include <thread>
#include <cmath>

using namespace mubone;

TEST_CASE("A sound object builder without buffers builds empty containers")
{
    SoundObjectBuilder bob;
    bob.numsamples = 999;
    bob.numblocks = 99;

    SECTION("No audiobuffer, no sounds")
    {
        auto sound = bob.buildSound();
        REQUIRE(!sound);
    }

    SECTION("No ctrlbuffer, no refs")
    {
        auto sound = bob.buildSound();
        std::vector<ControlSoundReference> refs;
        REQUIRE(refs.size() == 0);
        bob.buildReferences(sound, refs);
        REQUIRE(refs.size() == 0);
    }
}

TEST_CASE("Sound object builder builds sounds by copying from its audiobuffer")
{
    auto audiobuffer = std::make_shared<Simple::CircularBuffer<float>>(4);
    audiobuffer->tick(1).tick(2).tick(3).tick(4);
    SoundObjectBuilder bob;
    bob.startsamp = 1;
    bob.numsamples = 2;
    bob.audiobuffer = audiobuffer;

    auto sound = bob.buildSound();

    REQUIRE(sound->at(0) == 2);
    REQUIRE(sound->at(1) == 3);
    REQUIRE(sound->size() == 2);
}

TEST_CASE("Sound object builder builds control refs by copying and offsetting from its ctrlbuffer")
{
    auto audiobuffer = std::make_shared<Simple::CircularBuffer<float>>(1);
    auto ctrl = std::make_shared<Simple::CircularBuffer<std::pair<std::size_t, Vector>>>(4);
    SoundObjectBuilder bob;
    ctrl->tick(std::make_pair(std::size_t{10}, Vector{}));
    ctrl->tick(std::make_pair(std::size_t{20}, Vector{}));
    ctrl->tick(std::make_pair(std::size_t{30}, Vector{}));
    ctrl->tick(std::make_pair(std::size_t{40}, Vector{}));

    bob.audiobuffer = audiobuffer;
    bob.ctrlbuffer = ctrl;
    bob.startblock = 1;
    bob.numblocks = 2;
    bob.startsamp = 0;
    bob.numsamples = 1;

    std::vector<ControlSoundReference> refs;
    auto sound = bob.buildSound();
    bob.buildReferences(sound, refs);

    REQUIRE(refs.size() == 2);
    REQUIRE(refs[0].getIndex() == 0);
    REQUIRE(refs[1].getIndex() == 10);
    REQUIRE(refs[0].pointer() == sound);
}

TEST_CASE("New sounds can be added to the list by giving it a valid builder")
{
    SoundObjectList list;
    SoundObjectBuilder bob;
    auto audiobuffer = std::make_shared<Simple::CircularBuffer<float>>(3);
    auto ctrlbuffer = std::make_shared<Simple::CircularBuffer<std::pair<std::size_t, Vector>>>(3);

    audiobuffer->tick(1).tick(2).tick(3);

    ctrlbuffer->tick(std::make_pair(std::size_t{0}, Vector(1, 0, 0)));
    ctrlbuffer->tick(std::make_pair(std::size_t{20}, Vector(0, 1, 0)));
    ctrlbuffer->tick(std::make_pair(std::size_t{40}, Vector(0, 0, 1)));

    bob.audiobuffer = audiobuffer;
    bob.ctrlbuffer = ctrlbuffer;
    bob.startsamp = 0;
    bob.numsamples = 3;
    bob.startblock = 0;
    bob.numblocks = 3;

    REQUIRE(list.numSounds() == 0);
    REQUIRE(list.numReferences() == 0);

    REQUIRE(list.addSound(bob));

    std::this_thread::sleep_for(std::chrono::milliseconds(2000));

    list.update();
    REQUIRE(list.numSounds() == 1);
    REQUIRE(list.numReferences() == 3);

    SECTION("Added sounds can be retrieved by proximity to a given vector")
    {
        Vector v;
        v = ctrlbuffer->at(0).second;
        list.prepare_candidates(v, 0.5);
        auto ctrlref = list.random_candidate();
        REQUIRE(ctrlref);
        REQUIRE(ctrlref.getNormal().isApprox(v));
        REQUIRE(ctrlref.at(0) == Approx(audiobuffer->at(0)));
    }

}
#endif

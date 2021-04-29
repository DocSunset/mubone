#ifdef RUNTESTS
#include "catch.hpp"
#include "../Source/soundobject.h"
#include <cmath>

using namespace mubone;

TEST_CASE("A control sound reference can calculate its distance to a normal vector")
{
    SECTION("(1,0,0) and (0,1,0)")
    {
        ControlSoundReference ref(Vector(1, 0, 0), 0, std::make_shared<Sound>(3));
        REQUIRE(ref.arcDistanceTo(Vector(0, 1, 0)) == Approx(std::asin(1)));
    }
    SECTION("(1,0,0) and (-1,0,0)")
    {
        ControlSoundReference ref(Vector( 1, 0, 0), 0, std::make_shared<Sound>(3));
        REQUIRE(ref.arcDistanceTo(Vector(-1, 0, 0)) == Approx(std::acos(-1)));
    }
}

TEST_CASE("A control sound reference can be transparently treated like a sound, but accesses into the sound it references with an offset")
{
    auto sound = std::make_shared<Sound>(3);

    float * write = sound->getWritePointer();
    write[0] = 1;
    write[1] = 2;
    write[2] = 3;

    int offset = 1;
    ControlSoundReference ref(Vector(0, 0, 0), offset, sound);

    SECTION("getReadPointer")
    {
        REQUIRE(*(ref.getReadPointer()) == *(sound->getReadPointer(offset)));
    }

    SECTION("getWritePointer")
    {
        REQUIRE(*(ref.getWritePointer()) == *(sound->getWritePointer(offset)));
    }

    SECTION("at")
    {
        REQUIRE(ref.at(-1) == sound->at(offset - 1));
        REQUIRE(ref.at( 0) == sound->at(offset + 0));
        REQUIRE(ref.at( 1) == sound->at(offset + 1));
    }
}

TEST_CASE("A new control sound reference with a different offset can be produced with addition / subtraction")
{
    auto sound = std::make_shared<Sound>(3);

    float * write = sound->getWritePointer();
    write[0] = 1;
    write[1] = 2;
    write[2] = 3;

    ControlSoundReference ref(Vector(0, 0, 0), 1, sound);

    SECTION("offset within bounds")
    {
        SECTION("positive")
        {
            SECTION("operator+(int)")
            {
                auto newref = ref + 1;
                REQUIRE(newref.at(0) == sound->at(2));
            }
            SECTION("operator++()")
            {
                ++ref;
                REQUIRE(ref.at(0) == sound->at(2));
            }
            SECTION("operator++(int)")
            {
                auto oldref = ref++;
                REQUIRE(oldref.at(0) == sound->at(1));
                REQUIRE(ref.at(0) == sound->at(2));
            }
            SECTION("operator+=(int)")
            {
                ref += 1;
                REQUIRE(ref.at(0) == sound->at(2));
            }
        }
        SECTION("negative")
        {
            SECTION("operator-(int)")
            {
                auto newref = ref - 1;
                REQUIRE(newref.at(0) == sound->at(0));
            }
            SECTION("operator--()")
            {
                --ref;
                REQUIRE(ref.at(0) == sound->at(0));
            }
            SECTION("operator--(int)")
            {
                auto oldref = ref--;
                REQUIRE(oldref.at(0) == sound->at(1));
                REQUIRE(ref.at(0) == sound->at(0));
            }
            SECTION("operator-=(int)")
            {
                ref -= 1;
                REQUIRE(ref.at(0) == sound->at(0));
            }
        }
    }

    SECTION("offset out of bounds (clamped to boundaries)") 
    {
        SECTION("positive")
        {
            SECTION("operator+(int)")
            {
                auto newref = ref + 2;
                REQUIRE(newref.at(0) == sound->at(2));
            }
            SECTION("operator++()")
            {
                ++ref;
                ++ref;
                REQUIRE(ref.at(0) == sound->at(2));
            }
            SECTION("operator++(int)")
            {
                auto oldref = ref++;
                ref++;
                REQUIRE(oldref.at(0) == sound->at(1));
                REQUIRE(ref.at(0) == sound->at(2));
            }
            SECTION("operator+=(int)")
            {
                ref += 2;
                REQUIRE(ref.at(0) == sound->at(2));
            }
        }
        SECTION("negative")
        {
            SECTION("operator-(int)")
            {
                auto newref = ref - 2;
                REQUIRE(newref.at(0) == sound->at(0));
            }
            SECTION("operator--()")
            {
                --ref;
                --ref;
                REQUIRE(ref.at(0) == sound->at(0));
            }
            SECTION("operator--(int)")
            {
                auto oldref = ref--;
                ref--;
                REQUIRE(oldref.at(0) == sound->at(1));
                REQUIRE(ref.at(0) == sound->at(0));
            }
            SECTION("operator-=(int)")
            {
                ref -= 2;
                REQUIRE(ref.at(0) == sound->at(0));
            }
        }
    }
}
#endif

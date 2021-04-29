#ifdef RUNTESTS
#include "catch.hpp"
#include "../Source/window.h"

using namespace mubone;

TEST_CASE("Window applies a hann function to the input buffer")
{
    constexpr int duration = 101;
    float b[duration];
    for (int n = 0; n < duration; ++n) b[n] = 1;
    Window w;

    w.apply(b, 0, duration, duration);

    REQUIRE(b[0]   == Approx(0));
    REQUIRE(b[25]  == Approx(0.5));
    REQUIRE(b[50]  == Approx(1));
    REQUIRE(b[100] == Approx(0));
}
#endif

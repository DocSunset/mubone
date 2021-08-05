#ifndef RANGE_ADJUSTMENT_H
#define RANGE_ADJUSTMENT_H

#include <limits>
#include <cmath>

struct Range
{
    float min;
    float mid;
    float max;

    void reset()
    {
        min = std::numeric_limits<float>::max();
        static_assert(std::numeric_limits<float>::has_quiet_NaN, "error: execting NaNs");
        mid = std::numeric_limits<float>::quiet_NaN();
        max = std::numeric_limits<float>::min();
    }

    void set_midpoint(float x)
    {
        if (std::isnan(mid)) mid = x;
        // calculate an exponential rolling average over approx 5000 samples
        mid -= mid / 1000.0f;
        mid += x / 1000.0f;
    }

    void set_midpoint_from_bounds()
    {
        mid = 0.5 * (max - min);
    }

    void calibrate(float x)
    {
        if      (x < min) min = x;
        else if (x > max) max = x;
    }
};

float linear_map(float x, Range domain, Range codomain)
{
    float normalized = (x - domain.min) / (domain.max - domain.min);
    return normalized * (codomain.max - codomain.min) + codomain.min;
}

float centered_map(float x, Range domain, Range codomain)
{
    if (x < domain.mid)
        return linear_map(x, {domain.min, domain.mid}, {codomain.min, codomain.mid});
    else
        return linear_map(x, {domain.mid, domain.max}, {codomain.mid, codomain.max});
}

Range ranges[] =
    { {-1, 0, 1}
    , {0, 0.5, 1}
    , {0, 512, 1024}
    , {0, 512, 1024}
    , {0, -0.5, -1}
    , {0, 0.5, 1}
    , {0, 0.5, 1}
    , {0, 0.5, 1}
    };

enum RANGE : unsigned char
    { BIPOLAR_NORMALIZED
    , UNIPOLAR_NORMALIZED
    , JOYSTICK_X
    , JOYSTICK_Y
    , MISC_ADC0 // trigger
    , MISC_ADC1 // throttle
    , MISC_ADC2 // joint/x
    , MISC_ADC3 // joint/y
    , NUM_RANGES
    };

#endif

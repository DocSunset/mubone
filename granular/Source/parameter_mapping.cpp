#include "parameter_mapping.h"
#include <cmath>

float frequency_mapping(float f)
{
    constexpr float log_max_freq = 13.84352853461109; // log2(44100/3)
    constexpr float log_min_freq = -3.0; // 0.125 hz, 8 sec period
    float exponent = log_min_freq + (log_max_freq - log_min_freq) * f;
    return std::pow(2, exponent);
}

float duration_mapping(float d)
{
    constexpr float log_max_dur = 6; // 64 sec period
    constexpr float log_min_dur = -13.84352853461109; // log2(3/44100), 3 samps period at 44100 hz sr
    float exponent = log_min_dur + (log_max_dur - log_min_dur) * d;
    return std::pow(2, exponent);
}

#pragma once
#include "linalgtypes.h"

namespace mubone
{

template<const char * addr, typename value_t, const int mn, const int mx>
struct Signal
{
    using value_type = value_t;
    value_type value;
    static constexpr const char * address() {return addr;}
    static constexpr int min() {return mn;}
    static constexpr int max() {return mx;}

    // conversions to and from value_type
    // nb: nothing is explicit, so many operations e.g. math with a value_type
    // will implicitly convert Signal to value_type
    Signal() : value{} {}
    Signal(const value_type& newval) : value{newval} {}
    Signal(value_type&& newval) : value{newval} {}
    void operator=(const value_type& newval) {value = newval;}
    void operator=(value_type&& newval) {value = std::move(newval);}
    operator value_type() const {return value;}

    // math with another equivalent Signal
    value_type operator+(const value_type& other) const {return value + other;}
    value_type operator-(const value_type& other) const {return value - other;}
    value_type operator*(const value_type& other) const {return value * other;}
    value_type operator/(const value_type& other) const {return value / other;}
    Signal& operator+=(const value_type& other) {value += other; return *this;}
    Signal& operator-=(const value_type& other) {value -= other; return *this;}
    Signal& operator*=(const value_type& other) {value *= other; return *this;}
    Signal& operator/=(const value_type& other) {value /= other; return *this;}
};

template<const char * addr, const int mn, const int mx>
Vector operator*(
        const Signal<addr, Vector, mn, mx>& sig, 
        const float& scalar)
{
    return sig.value * scalar;
}

template<const char * addr, const int mn, const int mx>
Vector operator/(
        const Signal<addr, Vector, mn, mx>& sig, 
        const float& scalar)
{
    return sig.value / scalar;
}

template<class sig_t>
constexpr bool has_range()
{
    if constexpr (sig_t::min() == 0 && sig_t::max() == 0) return false;
    else return true;
}


} // namespace mubone

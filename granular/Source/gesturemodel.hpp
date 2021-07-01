#pragma once

#include <cmath>
#include <exception>
#include <typeinfo>
#include <stdexcept>

#include <../3rdparty/simplesound/simple/delay.h>
#include <../3rdparty/simplesound/simple/filters.h>
#include <../3rdparty/simplesound/simple/constants/pi.h>

#include "linalgtypes.h"
#include "parameters.hpp"
#include "sensorreceiver.hpp"
#include "../JuceLibraryCode/JuceHeader.h"

namespace mubone::gesture
{

class Model
: public sensor::Receiver::Listener
{
public:
    Model() 
        : 
        zero{Quaternion::Identity()},
        lop_jab(Vector::Zero(), 0.003), 
        no_DC_jab(Vector::Zero(), 0.005),
        prev_swing_velocity{Vector::Zero()},
        prev_slide_position{1.0f},
        prev_slide_velocity{0.0f},
        lop_slide_pos{1.0f, 0.1},
        lop_slide_vel{0.0f, 0.1}
    {
        osc.connect("127.0.0.1", 5007);
        osc.send("/mugranular/start");
    }

    ~Model()
    {
        osc.disconnect();
    }

    gesture::Sample sample() const {return g;}

    std::atomic<bool> set_zero;

    void oscBundleReceived(const sensor::Sample& s) override
    {
        map_sensor_to_gesture(s);
        send_osc_bundle();
    }

private:
    void map_sensor_to_gesture(const sensor::Sample& s)
    { 
        if (set_zero.load())
        {
            Quaternion squat = get<sensor::quat>(s).value;
            Vector ybasis = squat.toRotationMatrix().col(1);
            float azimuth = std::atan2(ybasis.y(), ybasis.x());
            zero = AngleAxis(azimuth, Vector::UnitZ());
            set_zero.store(false);
        }
        get<quat>(g)        = zero * get<sensor::quat>(s).value;
        get<mat>(g)         = get<quat>(g).value.toRotationMatrix();
        get<normal>(g) = get<mat>(g).value.col(1);
        get<cubeal>(g) = get_cubeal(get<normal>(g));
        get<inclination>(g) =  std::atan2(-get<mat>(g).value(1,2), get<mat>(g).value(2,2));
        get<roll>(g)        = -std::atan2(get<mat>(g).value(2,0), get<mat>(g).value(2,2));
        get<azimuth>(g) = get_azimuth(get<mat>(g).value.col(0), get<mat>(g).value.col(1));
        get<shake>(g) = get_shake(get<sensor::gyro>(s), get<sensor::motion>(s));
        get<gesture::buttons>(g) = get<sensor::buttons>(s);
        get<yoke_direction >(g) = Vector(1, 0, 0);//get<sensor::yoke>(s); TODO: redesign the yoke entirely...
        get<yoke_throttle>(g) = 0;//get<sensor::throttle>(s); TODO: redesign the yoke entirely...
        get<yoke_angle>(g) = 0.0f; // TODO
        get<yoke_deflection>(g) = 0.0f; // TODO
        set_swing(get<mat>(g).value * get<sensor::gyro>(s).value); // this ?should? be rotated into the global frame?
        set_jab(get<sensor::motion>(s));
        set_slide(get<sensor::slide>(s) > 0.5 ? (float)get<sensor::slide>(s) : 0);
        if (set_zero.load())
        {
            zero = AngleAxis(-get<azimuth>(g), Vector::UnitZ());
            set_zero.store(false);
        }
    }

    void send_osc_bundle()
    {
        juce::OSCBundle bundle{juce::Time::getCurrentTime()};
        for_each(g, [&bundle](const auto& elem)
        {
            using sig_t = typename std::remove_reference_t<decltype(elem)>::value_type;
            OSCMessage message(elem.address());
            if constexpr (std::is_floating_point_v<sig_t>) message.addFloat32(elem.value);
            else if constexpr (std::is_integral_v<sig_t>) message.addInt32(elem.value);
            else 
            {
                constexpr int size = std::is_same_v<sig_t, Quaternion> ? 4
                                   : std::is_same_v<sig_t, Matrix> ? 9
                                   : std::is_same_v<sig_t, Vector> ? 3
                                   : 0;
                if constexpr (size > 0)
                {
                    for (int i = 0; i < size; ++i)
                    {
                        if constexpr (std::is_same_v<sig_t, Quaternion>) 
                            message.addFloat32(elem.value.coeffs().data()[i]);
                        else message.addFloat32(elem.value.data()[i]);
                    }
                }
            }
            bundle.addElement(message);
        });
        osc.send(bundle);
    }

    static Vector get_cubeal(const Vector& normal)
    {
        float x = fabs(normal.x());
        float y = fabs(normal.y());
        float z = fabs(normal.z());
        float max_element = x > y ? x : y;
        max_element = max_element > z ? max_element : z;
        return normal / max_element;
    }
    
    static float get_azimuth(const Vector& xbasis, const Vector& ybasis)
    {
        float az1 = atan2(xbasis.y(), xbasis.x());
        float r21 = xbasis.y()*xbasis.y() + xbasis.x()*xbasis.x();
        float az2 = atan2(ybasis.y(), ybasis.x());
        float r22 = ybasis.y()*ybasis.y() + ybasis.x()*ybasis.x();
        az2 -= Simple::pi/2.0;
        if (az2 < -Simple::pi) az2 += Simple::twoPi;
        return r21 > r22? az1 : az2;
    }

    static float get_shake(const Vector& gyro, const Vector& motion)
    {
        return motion.norm() * gyro.norm();
    }

    void set_swing(const Vector& velocity)
    {
        get<swing_velocity>(g)  = velocity;
        get<swing_acceleration>(g)     = velocity - prev_swing_velocity;

        swing_accl_magn.tick(get<swing_acceleration>(g).value.norm());
        bool peak  = swing_accl_magn[-2] < swing_accl_magn[-1];
             peak &= swing_accl_magn[-1] < swing_accl_magn[0];
        get<swing_peak>(g) = peak;

        prev_swing_velocity = get<swing_velocity>(g);
    }
    
    void set_jab(const Vector& global_accel)
    {
        get<jab_velocity>(g) = lop_jab.tick(no_DC_jab.tick(global_accel));
        get<jab_acceleration>(g)    = no_DC_jab.value();

        jab_accl_magn.tick(get<jab_acceleration>(g).value.norm());
        bool peak  = jab_accl_magn[-2] < jab_accl_magn[-1];
             peak &= jab_accl_magn[-1] < jab_accl_magn[0];
        get<jab_peak>(g) = peak;
    }

    void set_slide(const float& position)
    {
        if (position < 0.5) return;
        get<slide_position>(g) = position;
        get<slide_velocity>(g) = prev_slide_position - lop_slide_pos.tick(position);
        get<slide_acceleration>(g) = prev_slide_velocity - lop_slide_pos.tick(get<slide_velocity>(g));
        slide_accl_magn.tick(get<slide_acceleration>(g));
        bool peak  = slide_accl_magn[-2] < slide_accl_magn[-1];
             peak &= slide_accl_magn[-1] < slide_accl_magn[0];
        get<slide_peak>(g) = peak;

        prev_slide_position = get<slide_position>(g);
        prev_slide_velocity = get<slide_velocity>(g);
    }

    gesture::Sample g;

    Quaternion zero;
    Simple::Lowpass<Vector> lop_jab;
    Simple::DCBlocker<Vector> no_DC_jab;
    Vector prev_swing_velocity;
    float prev_slide_position;
    float prev_slide_velocity;
    Simple::Lowpass<float> lop_slide_pos;
    Simple::Lowpass<float> lop_slide_vel;
    Simple::Delay<float, 3> swing_accl_magn;
    Simple::Delay<float, 3> jab_accl_magn;
    Simple::Delay<float, 3> slide_accl_magn;

    juce::OSCSender osc;
};
    
} // namespace mubone

#pragma once

#include "list.hpp"
#include "linalgtypes.h"
#include "signal.hpp"

#define ADD_SIGNAL(name, string, type, min, max)\
    constexpr char name##_address[] = string;\
    using name = Signal<name##_address, type, min, max>
namespace mubone
{

namespace sensor
{
    ADD_SIGNAL(accl, "/accl", Vector, 0, 0);
    ADD_SIGNAL(gyro, "/gyro", Vector, 0, 0);
    ADD_SIGNAL(magn, "/magn", Vector, 0, 0);
    ADD_SIGNAL(quat, "/quat", Quaternion , 0, 0);
    ADD_SIGNAL(yoke, "/yoke_quat", Quaternion, 0, 0);
    //ADD_SIGNAL(throttle, "/throttle", float, 0, 0);
    ADD_SIGNAL(motion, "/motion", Vector , 0, 0);
    ADD_SIGNAL(buttons, "/buttons", int , 0, 0);
    ADD_SIGNAL(slide, "/slide", float, 0, 9);

    using Sample = List<accl, gyro, magn, quat, yoke, /*throttle,*/ motion, buttons, slide>;

    void initialize(Sample& s)
    {
        get<accl>(s) = Vector(0, 0, 1);
        get<gyro>(s) = Vector::Zero();
        get<magn>(s) = Vector(0, 1, 0);
        get<quat>(s) = Quaternion::Identity();
        get<yoke>(s) = Quaternion::Identity();
        //get<throttle>(s) = 0;
        get<motion>(s) = Vector::Zero();
        get<buttons>(s) = 0;
        get<slide>(s) = 1;
    }

} // namespace Sensor

namespace gesture
{
    ADD_SIGNAL(quat, "/orientation/quaternion", Quaternion, 0, 1);
    ADD_SIGNAL(mat, "/orientation/matrix", Matrix, 0, 1);
    ADD_SIGNAL(normal, "/normal", Vector, 0, 1);
    ADD_SIGNAL(cubeal, "/cubeal", Vector, 0, 1);
    ADD_SIGNAL(inclination, "/inclination", float, -180, 180);
    ADD_SIGNAL(roll, "/roll", float, -180, 180);
    ADD_SIGNAL(azimuth, "/azimuth", float, -180, 180);
    ADD_SIGNAL(shake, "/shake", float, 0, 1);
    ADD_SIGNAL(buttons, "/buttons", int, 0, 7);
    ADD_SIGNAL(yoke_direction, "/yoke/direction", Vector, 0, 1);
    ADD_SIGNAL(yoke_angle, "/yoke/angle", float, 0, 1);
    ADD_SIGNAL(yoke_deflection, "/yoke/deflection", float, 0, 1);
    ADD_SIGNAL(yoke_throttle, "/yoke/throttle", float, -180, 180);
    ADD_SIGNAL(swing_velocity, "/swing/velocity", Vector, 0, 1);
    ADD_SIGNAL(swing_acceleration, "/swing/acceleration", Vector, 0, 1);
    ADD_SIGNAL(swing_peak, "/swing/peak", bool, 0, 1);
    ADD_SIGNAL(jab_velocity, "/jab/velocity", Vector, 0, 1);
    ADD_SIGNAL(jab_acceleration, "/jab/acceleration", Vector, 0, 1);
    ADD_SIGNAL(jab_peak, "/jab/peak", bool, 0, 1);
    ADD_SIGNAL(slide_position, "/slide/position", float, 0, 9);
    ADD_SIGNAL(slide_velocity, "/slide/velocity", float, 0, 1);
    ADD_SIGNAL(slide_acceleration, "/slide/acceleration", float, 0, 1);
    ADD_SIGNAL(slide_peak, "/slide/peak", bool, 0, 1);

    using Sample = List<quat, mat, normal, cubeal, inclination, roll, azimuth,
          shake, buttons, yoke_direction, yoke_angle, yoke_deflection,
          yoke_throttle, swing_velocity, swing_acceleration, swing_peak,
          jab_velocity, jab_acceleration, jab_peak, slide_position,
          slide_velocity, slide_acceleration, slide_peak>;

    void initialize(Sample& s)
    {
        get<quat>(s) = Quaternion::Identity();
        get<mat>(s) = Matrix::Identity();
        get<normal>(s) = Vector(0, 1, 0);
        get<cubeal>(s) = Vector(0, 1, 0); 
        get<inclination>(s) = 0;
        get<roll>(s) = 0;
        get<azimuth>(s) = 0;
        get<shake>(s) = 0;
        get<buttons>(s) = 0;
        get<yoke_direction>(s) = Vector(0, 0, 0);
        get<yoke_angle>(s) = 0;
        get<yoke_deflection>(s) = 0;
        get<yoke_throttle>(s) = 0;
        get<swing_velocity>(s) = Vector(0, 0, 0);
        get<swing_acceleration>(s) = Vector(0, 0, 0);
        get<swing_peak>(s) = false;
        get<jab_velocity>(s) =  Vector(0, 0, 0);
        get<jab_acceleration>(s) =  Vector(0, 0, 0);
        get<jab_peak>(s) = false;
        get<slide_position>(s) = 1;
        get<slide_velocity>(s) = 0;
        get<slide_acceleration>(s) = 0;
        get<slide_peak>(s) = false;
    }

} // namespace gesture

namespace synthesis
{
    ADD_SIGNAL(direction, "/direction", Vector, 0, 1);
    ADD_SIGNAL(search_radius, "/search_angle", float, 1, 180);
    ADD_SIGNAL(frequency, "/frequency", float, 0, 10000);
    ADD_SIGNAL(duration, "/duration", float, 0, 60);
    ADD_SIGNAL(playback_rate, "/playback_rate", float, -4, 4);
    ADD_SIGNAL(amplitude, "/amplitude", float, 0, 1);

    ADD_SIGNAL(sound_recording, "/sound_recording", bool, 0, 1);
    ADD_SIGNAL(gesture_recording, "/gesture_recording", bool, 0, 1);
    ADD_SIGNAL(clouds_planted, "/clouds/planted", int, 0, 8);
    ADD_SIGNAL(granulating, "/granulating", bool, 0, 1);
    ADD_SIGNAL(reset_trigger, "/reset", bool, 0, 1);

    using GrainDescription = List<direction, search_radius, frequency, duration,
          playback_rate, amplitude>;

    using State = List<sound_recording, gesture_recording, clouds_planted,
          granulating, reset_trigger>;

    void initialize(GrainDescription& g)
    {
        get<direction>(g)            = Vector(1,  0,  0);
        get<search_radius>(g)        = 0.5;
        get<frequency>(g)            = 20;
        get<duration>(g)             = 0.05;
        get<playback_rate>(g)        = 1;
        get<amplitude>(g)            = 0.03125;
    }

    void initialize(State& s)
    {
        get<sound_recording>(s)   = false;
        get<gesture_recording>(s) = false;
        get<clouds_planted>(s)       = false;
        get<granulating>(s)       = false;
        get<reset_trigger>(s)     = false;
    }
} // namespace synthesis

#undef ADD_SIGNAL

} // namespace mubone

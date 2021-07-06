# mubone firmware

The mubone has two microcontrollers: the main MCU that reads the sensors, and
an auxiliary ESP8266 that forwards data to the local network and coordinates
connecting to the network.

The main firmware is contained in the `mcu` sketch:

```cpp
// @#'mcu/mcu.ino'
@{includes}

@{global definitions}
void setup()
{
    @{comms setup}

    @{sensors setup}

    @{initialize mimu filter}
}

void loop()
{
    @{read sensors}
    @{receive OSC messages}
    @{transmit OSC messages}
}
// @/
```

The ESP8266 firmware is contained in the `esp` sketch:

```cpp
// @#'esp/esp.ino'
@{esp includes}

@{esp definitions}
void setup()
{
    @{connect to the network}
}

void loop()
{
    @{receive serial OSC}
    @{send UDP OSC}
    @{receive UDP OSC}
    @{send serial OSC}
}

// @/
```

# mcu.ino

## EEPROM

Some data in memory needs to be stored persistently across power cycles,
including notably the error correction constants derived from calibration and
used to compensate for the shortcomings of the magnetometer and other
sensors. The EEPROM library is well suited to this task. 

```cpp
// @+'includes'
#include <EEPROM.h>
// @/
```

`EEPROM` class provides a pair of generic functions for storing data of any C++
type to persistent memory (sometimes flash memory, despite the name of the
library). Although it's also possible to manually serialize data to and from
persistent storage, it's most convenient to make use of these generic
functions, which requires us to define a structure for holding all of the data
we want to store across power cycles.

```cpp
// @+'global definitions'
struct PersistentState
{
    @{persistent state}
};

// @/
```

Before storing state it's necessary to copy it into the `PersistentState`
structure, and after loading state it still needs to be copied to where it's
needed. The full load and store methods therefore have the following outline:

```cpp
// @='persistent state subroutines'
void load_persistent_state()
{
    PersistentState persistent_state;
    const uint8_t *ptr = (const uint8_t*) &persistent_state;
    int count = sizeof(PersistentState);
    EEPtr e = 0x00;

    EEPROM.begin();
    for (; count; --count, ++e) (*e).update(*ptr++);
    EEPROM.end();

    @{distribute persistent state}
}

void store_persistent_state()
{
    PersistentState persistent_state;
    uint8_t *ptr = (uint8_t*) &persistent_state;
    int count = sizeof(PersistentState)
    EEPtr e = 0x00;

    @{collect persistent state}

    EEPROM.begin()
    for (; count; --count, ++e) *ptr++ = *e;
    EEPROM.end();
}
// @/
```

Note that the above routines assume that the `PersistentState` class is
trivially copyable or at least acts like it is. The implementer is beholden
to ensure that this is actually the case.

The full definition of the subroutines for distributing and storing state
respectively are elaborated in bits and pieces below; wherever state is
described that needs saving, the means for collecting and distributing that
state are given in an adjacent section of the text.

EEPROM has a limited life-span that is reduced by writing data to it. Currently
we aren't attempting any kind of wear-leveling, but eventually we may wish to
implement something. The basic strategy would be to include a counter in the
storage struct that gives an address to store the next write to. Whenever
writing to EEPROM, the address is incremented before storing it. When loading,
the entire EEPROM storage is scanned until the largest stored address is
located; this must be the most recent write, so it is loaded. This strategy is
effective, but not without its challenges. For instance, if the size of the
storage struct changes it would invalidate previously written records,
necessitating that they be reset.

More detail may be found
[here](https://embeddedgurus.com/stack-overflow/2017/07/eeprom-wear-leveling/).

## OSC

All of the sensor readings are packed into an OSC bundle which is periodically
sent to the network via the attached ESP8266. The bundle is declared globally
and reused every loop so that memory only has to be allocated for it once, the
first time the loop runs. 

A bundle is also declared for sending debug information and error messages.
This one is cleared at the end of the loop function after transmitting OSC
messages, so memory has to be allocated and released any time an error message
is added to the bundle. It is assumed that there will not be very many error
messages, and that the performance impact associated with their memory
management will thus be negligible. 

The error message bundle has to be global so that subroutines can add error
messages to the bundle. The sensor bundle on the other hand is declared
statically before reading sensors.

```cpp
// @+'includes'
#include "OSCBundle.h"
// @/

// @+'global definitions'
OSCBundle error_messages;
// @/

// @+'read sensors'
static OSCBundle bundle;

// @/
```

As seen below, each message that needs to be sent is statically declared in the
loop function and initialized by calling `bundle.add(address)` with the
appropriate address. This ensures that messages are only allocated once.
Data is added to the messages using the built in `msg.set()` method, or the
following helper function:

```cpp
// @+'global definitions'
void osc_set_floats(float * f, int size, OSCMessage& msg)
{
    for (int i = 0; i < size; ++i) msg.set(i, f[i]);
}

// @/
```

## SLIP serial

OSC messages are exchanged with the wireless controller via a SLIP-encoded
serial stream. Streams are set up for both USB and hardware serial so that the
device can be used in either wired or wireless configuration. Setup is very
simple.

Note: at the time of writing, hardware serial is disabled since the esp firmware
is still a work in progress.

```cpp
// @+'includes'
#include "SLIPEncodedUSBSerial.h"
//#include "SLIPEncodedSerial.h"
// @/

// @+'global definitions'
SLIPEncodedUSBSerial usbserial{Serial};
//SLIPEncodedSerial hwserial{Serial1};

// @/

// @+'comms setup'
usbserial.begin(2000000);
//hwserial.begin(2000000);
// @/

## I2C

The Arduino Wire library is used to interact with the I2C bus. All of the I2C
sensors use the default global Wire instance, so not much is needed to get
things up and running.

```cpp
// @+'comms setup'
Wire.begin();
// @/
```

## buttons

Buttons are the simplest sensors to measure. The internal pull-up resistor of
the MCU pin the button is attached to keeps the pin at a logical high voltage
until the button is pressed, connecting the pin directly to ground and
producing a logical low value.

```cpp
// @+'global definitions'
constexpr int num_buttons = 8;
constexpr int button_pins[num_buttons] = {5, 6, 7, 8, 2, 3, 4, 11};

// @/

// @+'sensors setup'
for (const auto& pin : button_pins) pinMode(pin, INPUT_PULLUP);

// @/

// @+'read sensors'
static OSCMessage& buttons = bundle.add("/buttons");
for (int i = 0; i < num_buttons; ++i) 
{
    const int pin = button_pins[i];
    const int reading = digitalRead(pin);
    buttons.set(i, reading);
}

// @/
```

## joystick

The joystick button is read alongside all the other buttons. Its analog signals
are handled independently, but in a similarly simple manner using the Arduino
library to read from the MCU's ADC.

```cpp
// @+'global definitions'
constexpr int joystick_pin_x = A1;
constexpr int joystick_pin_y = A0;

// @/

// @+'sensors setup'
pinMode(joystick_pin_x, INPUT);
pinMode(joystick_pin_y, INPUT);

// @/

// @+'read sensors'
static OSCMessage& joystick = bundle.add("/joystick");
joystick.set(0, analogRead(joystick_pin_x));
joystick.set(1, analogRead(joystick_pin_y));

// @/
```

## ADS1219 sensors

The slide position sensor, rotator, u-joint, and trigger use ADS1219 ADCs. Most
of the tricky parts of working with the ADC are handled by a separate library.
The firmware has to keep track of which channel of the ADC is being read, and
has to manually cycle through the channels. The handling is very similar for
the slide position sensor (sps) and the sensors that share the other ADS1219.

```cpp
// @+'includes'
#include "ADS1219.h"
// @/

// @+'global definitions'
using namespace ADS1219;

constexpr Address sps_address(ADDR0::DGND, ADDR1::DGND);
ADS1219_ADC sps_adc(sps_address, Wire);
constexpr MUX sps_channels[] = {MUX::AIN0_AIN1, MUX::AIN2_AIN3};

constexpr Address misc_adc_address(ADDR0::DVDD, ADDR1::DGND);
ADS1219_ADC misc_adc(misc_adc_address, Wire);
constexpr MUX misc_channels[] = {MUX::AIN0, MUX::AIN1, MUX::AIN2, MUX::AIN3};

// @/

// @+'sensors setup'
sps_adc.set_config( MUX::AIN0_AIN1
              , RATE::SPS1000
              , MODE::CONTINUOUS
              , VREF::EXTERN
              );
sps_adc.start_conversion();

misc_adc.set_config( MUX::AIN0
              , RATE::SPS1000
              , MODE::CONTINUOUS
              , VREF::EXTERN
              );
misc_adc.start_conversion();

// @/

// @+'read sensors'
static OSCMessage& sps = bundle.add("/slide_position");
{
    static int pot = 0;
    if (sps_adc.data_ready())
    {
        float reading = 0;
        bool ret = sps_adc.read_normalized(reading);
        sps.set(pot, reading);
        pot ^= 1; // switch to next potentiometer
        ret = sps_adc.modify_config(sps_channels[pot]);
    }
}

static OSCMessage * misc[4] = { &bundle.add("/trigger")
                              , &bundle.add("/joint/x")
                              , &bundle.add("/joint/z")
                              , &bundle.add("/joint/y")
                              };
{
    static int pot = 0;
    if (misc_adc.data_ready())
    {
        float reading = 0;
        bool ret = misc_adc.read_normalized(reading);
        misc[pot]->set(0, reading);
        pot = (pot + 1) & 0b11; // switch to next potentiometer
        ret = misc_adc.modify_config(misc_channels[pot]);
    }
}

// @/
```

## MIMU

The MIMU sensors are particularly involved since they require calibration and
sensor fusion. Most of the details of these algorithms are abstracted away in
the MIMU libraries.

```cpp
// @+'includes'
#include "MIMU_MPU9250.h"
#include "MIMUCalibrator.h"
#include "MIMUFusion.h"
// @/

// @+'global definitions'
MIMU_MPU9250 mimu;
MIMUCalibrator mimu_calibrator;
MIMUFusionFilter mimu_filter;

MIMUReading reading = MIMUReading::Zero();
Quaternion mimu_zero = Quaternion::Identity();

// @/

// @+'sensors setup'
mimu.setup();
mimu_calibrator.setup();
mimu_filter.setup();
// @/
```

Reading the sensors, correcting the errors in their output based on the
calibration coefficients, and running the fusion filter are performed in a
subroutine:

```cpp
// @+'global definitions'
void mimu_tick()
{
    if (mimu.readInto(reading))
    {
        mimu_calibrator.calibrate(reading);
    }
    mimu_filter.fuse(reading.accl, reading.gyro, reading.magn);
}

// @/
```

After ticking the MIMU, the orientation is zeroed and converted to two useful
representations (quaternion and matrix). Along with the calibrated sensor
readings, these orientation representations are added to the OSC bundle.

```cpp
// @+'read sensors'
mimu_tick();
auto zeroed = mimu_zero * mimu_filter.q;
auto zeroed_matrix = zeroed.toRotationMatrix();
auto accl_zerog = mimu_filter.getZeroGravityAccl();

static OSCMessage& accl = bundle.add("/accl");
osc_set_floats(reading.accl.data(),    3, accl);
static OSCMessage& gyro = bundle.add("/gyro");
osc_set_floats(reading.gyro.data(),    3, gyro);
static OSCMessage& magn = bundle.add("/magn");
osc_set_floats(reading.magn.data(),    3, magn);
static OSCMessage& quat = bundle.add("/quat");
osc_set_floats(zeroed.coeffs().data(), 4, quat);
static OSCMessage& mtrx = bundle.add("/mtrx");
osc_set_floats(zeroed_matrix.data(),   9, mtrx);
static OSCMessage& zerog = bundle.add("/zerog");
osc_set_floats(accl_zerog.data(),      3, zerog);

// @/
```

Based on the matrix, the normal vector can be extracted; this unit vector
reflects the direction the slide is pointed in the global coordinate space. 

```cpp
// @+'read sensors'
static OSCMessage& normal = bundle.add("/normal");
auto normal_vector = zeroed_matrix.col(1);
osc_set_floats(normal_vector.data(), 3, normal);

// @/
```

The normal vector always lies on the surface of a unit sphere. Sometimes it is
useful to project the vector further, so that it lies on a unit cube. The
following routine performs this mapping:

 ```cpp
// @+'read sensors'
static OSCMessage& cubeal = bundle.add("/cubeal");
{
    float x = fabs(normal_vector.x());
    float y = fabs(normal_vector.y());
    float z = fabs(normal_vector.z());
    float max = x > y ? x : y;
    max = max > z ? max : z;
    Vector cubeal_vector = normal_vector / max;
    osc_set_floats(cubeal_vector.data(), 3, cubeal);
}
// @/
```

TODO?: inclination, roll, azimuth, shake, swing, jab

### initializing the orientation sensor fusion filter

In some situations, such as after turning on the device, the orientation sensor
fusion filter will have an internal state that differs significantly from the
actual orientation of the sensor. In these situations, the filter can be
initialized by setting the orientation close to the actual orientation and then
running the filter for a moment with a very high proportional feedback
coefficient. The following function performs this routine:

```cpp
// @+'global definitions'
void mimu_initialize()
{
    while (!mimu.readInto(reading)); // wait until data is ready

    // initialize the orientation
    mimu_calibrator.calibrate(reading);
    mimu_filter.initializeFrom(reading.accl, reading.magn);
    
    float originalkp = mimu_filter.fc.k_P; // save current proportional feedback
    mimu_filter.fc.k_P = 21; // crank it up past 11

    // tick for 1.5s
    float start = millis();
    for (float dt = 0; dt < 1500; dt = millis() - start) mimu_tick();

    mimu_filter.fc.k_P = originalkp; // reset k_P
}

// @/

This is called at the end of the `setup` routine.

// @='initialize mimu filter'
mimu_initialize();
// @/
```

### setting the orientation sensor alignment

Tiny variances in the assembly of the circuit boards, mechanical parts, and
mounting of the mubone augmentations to the slide can lead to the alignment of
the orientation sensor changing with respect to the brass. The alignment
routine allows the orientation of the MIMU with respect to the brass to be
estimated; this requires two seperated measurements with the instrument in two
specific position.  In the first position, the player points the slide directly
at the ground, as close to perfectly vertical as possible.  In the second
position, the player points the mubone forward. In this second position, the
exact elevation of the slide is relatively unimportant, but the instrument
should not be rotated about the slide at all. The mubone should be held as
close to a neutral playing position as possible.

The routine waits indefinitely until the player is in position; once there, the
player can signal their readiness by pressing any button.  The following helper
checks for the player's signal:

```cpp
// @+'global definitions'
bool any_button_pressed()
{
    for (int i = 0; i < num_buttons; ++i) 
    {
        if (digitalRead(button_pins[i]) == 0) return true;
    }
    return false;
}

// @/
```

Before measuring the first position, the current alignment is reset so that the
unaligned sensor readings can still have their non-alignment errors compensated
for by the calibrator while being otherwise "raw" (i.e. not aligned using the
current presumably incorrect alignment constants).

```cpp
// @+'global definitions'
void mimu_align()
{
    mimu_calibrator.resetAlignment();
// @/
```

The routine then waits indefinitely until the player's signal is received, at
which time the MIMU is then read continuously for three seconds to average out
any noise, and the vector measured by the accelerometer is taken as a
measurement of the mubone's y-axis from the sensor's frame of reference.

```cpp
// @+'global definitions'
    while(!any_button_pressed()) {/* wait */}
    delay(1000); // wait a second for the player to stabilize after
                 // pressing the button
    reading = mimu.readForMillis(3000);
    mimu_calibrator.calibrate(reading);
    Vector ybasis(reading.accl);
// @/
```

After a second indefinite wait for the player's signal, the second measurement
it taken in the same fashion as the first.  The second reading is interpreted
as a measurement in the plane defined by the mubone's y- and z-axes from the
sensor's frame of reference. Along with the first reading, the two measurements
provide enough information to determine the alignment of the sensor, which is
stored with the other calibration contants.

```cpp
// @+'global definitions'
    while(!any_button_pressed()) {/* wait */}
    delay(1000); // wait a second for the player to stabilize after
                 // pressing the button
    reading = mimu.readForMillis(3000);
    mimu_calibrator.calibrate(reading);
    Vector zbasis(reading.accl);

    mimu_calibrator.setAlignment(ybasis, zbasis);
// @/
```

After the alignment is set, the fusion filter is re-initialized based on the
new sensor-local frame of reference.

```cpp
// @+'global definitions'
    mimu_initialize();
} 

// @/
```

### calibrating the orientation sensor

In case the user needs to calibrate the MIMU sensor there is significantly less
work to do. The following subroutine performs a minimal loop reading from the
orientation sensor and sending the un-calibrated raw data to the network for
analysis by calibration software. Pressing any of the buttons on the device
(the same signal as is used in the alignment procedure) cancels calibration
mode and breaks out of the loop, returning execution to the normal flow.

```cpp
// @+'global definitions'
void mimu_calibrate()
{
    OSCBundle bundle;
    OSCMessage& msg = bundle.add("/raw");
    while (true)
    {
        if (any_button_pressed()) return;
        mimu.readInto(reading);
        reading.updateBuffer();
        osc_set_floats(reading.data, 10, msg);
        @{transmit OSC messages}
    }
}

// @/
```

### persistent state: calibration, alignment, and fusion constants

The alignment data only needs to be updated if the alignment of the MIMU with
respect to the brass is significantly altered, e.g. if the mubone attachement
is dropped or the mounting plate is adjusted in some way. The calibration
constants for the MIMU sensor should change even less often. Both of these
data structures should be stored persistently to save the player from having to
re-upload the data every time they play.

Similarly, the fusion filter's coefficients may be adjusted by the player to
suit their preferences and fine tune the performance of the filter. These
should also be stored.

```cpp
// @+'persistent state'
MIMUCalibrationConstants mimucc;
MIMUFilterCoefficients mimufc;
// @/

// @+'collect persistent state'
persistent_state.mimucc = mimu_calibrator.getCalibration();
persistent_state.mimufc = mimu_filter.fc;
// @/

// @+'distribute persistent state'
mimu_calibrator.setCalibration(persistent_state.mimucc);
mimu_filter.fc = persistent_state.mimufc;
// @/
```

## setting the sensors' zero points

In most use cases, the orientation of the mubone in relative to the global
frame of reference is actually not meaningful; rather, the orientation relative
to a local frame of reference (e.g. the stage) is needed. This local frame is
typically a rotation of the global frame. Zeroing the orientation sensor allows
the orientation output to be rotated into the local frame of reference by
setting the orientation in which the mubone is aligned with the local y-axis
(e.g. when facing the audience). This is conceptually the same as setting the
direction that is considered "forward". The direction of up remains based on
gravity. The remaining coordinate axis (left-right) is determined as a
consequence.

```cpp
// @='zero orientation'
Vector ybasis = mimu_filter.rotation.col(1);
float azimuth = std::atan2(ybasis.y(), ybasis.x());
mimu_zero = AngleAxis(azimuth, Vector::UnitZ());
// @/
```

A somewhat related concept, the slide position sensor's output is accumulated
over time to provide an estimate of the slide position based on the rotation of
the string potentiometers. Zeroing the slide position sensor resets the current
position estimate to zero (usually with the slide fully retracted).

Both of these tasks are performed by the following subroutine:

```cpp
// @+'global definitions'
void zero_sensors()
{
    @{zero orientation}
}

// @/
```

Unlike alignment and calibration, the zero point needs to be set at least
once very time the instrument is played, and may conceivably be set multiple
times over the course of a performance. For this reason there's no use in
saving it persistently.

## sending OSC output

This is very simple using the OSC library. Start packet, send packet, end
packet.  The error messages bundle is only sent if there are error messages to
send.

```cpp
// @+'global definitions'
template<class SLIP_T>
void send_osc(SLIP_T& serial, OSCBundle& bundle, OSCBundle& error_messages)
{
    serial.beginPacket();
    bundle.send(serial);
    serial.endPacket();
    
    if (error_messages.size() > 0)
    {
        serial.beginPacket();
        error_messages.send(serial);
        serial.endPacket();
        error_messages.empty();
    }
}
// @/

// @='transmit OSC messages'
send_osc(usbserial, bundle, error_messages);
//send_osc(hwserial, bundle, error_messages);
// @/
```

## receiving OSC inputs

Somewhat more involved that sending, receiving OSC requires some book keeping
to deal with both messages and bundles, and to avoid blocking the main loop.

First of all: the MCU may in principle receive a single message or a bundle of
messages in any given SLIP packet. To deal with either case, both a bundle and
a message must be declared.

Initially, no packet is in progress and both the message `msg_in` and the
bundle `bundle_in` are empty. In this case, when bytes start coming in the very
first byte can be checked to determine if a bundle is incoming, since it will
be a `#` character. For any other first byte, we can assume that the incoming
packet is a message.

We use an enumeration to keep track of the current state. This is appended to
the includes block to ensure the definition is available when the templates are
instantiated.

```cpp
// @+'includes'
enum OSCInState {WAITING, MESSAGE, BUNDLE};
// @/

// @+'global definitions'
template<class SLIP_T>
void receive_osc(SLIP_T& serial)
{
    static OSCBundle bundle_in;
    static OSCMessage msg_in;
    static OSCInState state = WAITING;
    if (state == WAITING)
    {
        if (serial.available())
        {
            if (serial.peek() == '#') state = BUNDLE;
            else state = MESSAGE;
        }
    }
    
    if      (state == MESSAGE) state = receive_osc_inner(serial, msg_in, state);
    else if (state == BUNDLE)  state = receive_osc_inner(serial, bundle_in, state);
}

// @/

// @+'receive OSC messages'
receive_osc(usbserial);
//receive_osc(hwserial);
// @/
```

Because serial lines are not inherently packet-oriented, it's possible that
parts of a packet may arrive over the course of several loops. Rather than
block the loop waiting for the end of the packet, the strategy is to read as
much as there is available and then move on. 

The code is the same for receiving messages as it is for receiving bundles, so
we use a template function to avoid having to repeat ourselves. This function
will call out to `osc_dispatch` when the end of a packet is received before
returning `waiting` state.  Otherwise it returns the initial state so that the
container will keep being filled on the next loop.

Due to the implementation of SLIPSerial in the CNMAT OSC library, it's
necessary to check for an `endofPacket` condition before reading.

```cpp
// @+'global definitions'
template<class SLIP_T, class OSCContainer>
OSCInState receive_osc_inner(SLIP_T& serial, OSCContainer& osc, OSCInState initial_state)
{
    if (serial.endofPacket())
    {
        if (!osc.hasError()) osc_dispatch(osc);
        osc.empty();
        return WAITING;
    }
    else for (int size = serial.available(); size > 0; --size)
    {
        osc.fill(serial.read());
    }
    return initial_state;
}
// @/
```

For full flexibility, a custom dispatch routine is used. The bundle dispatcher
simply iterates through the messages in the bundle and forwards them to the
message dispatcher. The Arduino OSC library we use doesn't support nested
bundles, so there's no need to check for them.

```cpp
// @+'global definitions'
void osc_dispatch(OSCBundle& bundle)
{
    for (int i = 0; i < bundle.size(); ++i)
    {
        osc_dispatch( *(bundle.getOSCMessage(i)) );
    }
}

// @/
```

Many incoming messages simply require the arguments of the OSC message to be
parsed as floats and loaded into global variables. The following sub-routine
facilitates this pattern. After checking that the OSC message contains the
specified number of arguments in the first for loop, the second for loop copies
the arguments of the OSC message to the given `float * value` buffer. A bool
is returned to allow further action to be taken if the values are successfully
copied.

```cpp
// @+'global definitions'
bool set_floats(float * value, OSCMessage& msg, int n = 1)
{
    if (n > msg.size()) return false;
    for (int i = 0; i < n; ++i)
    {
             if (msg.isFloat(i))  continue;
        else if (msg.isDouble(i)) continue;
        else if (msg.isInt(i))    continue;
        else return false;
    }
    for (int i = 0; i < n; ++i)
    {
             if (msg.isFloat(i))  value[i] = msg.getFloat(i);
        else if (msg.isDouble(i)) value[i] = msg.getDouble(i);
        else if (msg.isInt(i))    value[i] = msg.getInt(i);
    }
    return true;
}

// @/
```

Finally, the message dispatcher simply checks if the given OSC message matches
any of the expected addresses. Comments are provided inline for quick reference
of the intended effect of each of the OSC methods supported by the firmware.

The persistent storage subroutines are inserted at this point in the file,
which ensures that they will be able to view any global variables declared
earlier, such as `mimu_calibrator` and the like.

```cpp
// @+'global definitions'
@{persistent state subroutines}

void osc_dispatch(OSCMessage& msg)
{
    // set the sensors' zero values (mimu and sps)
    if (msg.fullMatch("/zero")) zero_sensors();

    // quickly initialize the orientation sensor
    else if (msg.fullMatch("/initialize")) mimu_initialize();

    // set the alignment of the mimu wrt to the brass
    else if (msg.fullMatch("/align")) mimu_align();

    // switch to calibration mode; raw mimu readouts are sent to the network
    else if (msg.fullMatch("/calibrate")) mimu_calibrate();

    // set the calibration matrices and vectors
    else if (msg.fullMatch("/calibration/accl/matrix")) set_floats(mimu_calibrator.cc.acclcalibration.data(), msg, 9);
    else if (msg.fullMatch("/calibration/gyro/matrix")) set_floats(mimu_calibrator.cc.gyrocalibration.data(), msg, 9);
    else if (msg.fullMatch("/calibration/magn/matrix")) set_floats(mimu_calibrator.cc.magncalibration.data(), msg, 9);
    else if (msg.fullMatch("/calibration/accl/vector")) set_floats(mimu_calibrator.cc.abias.data(), msg, 3);
    else if (msg.fullMatch("/calibration/gyro/vector")) set_floats(mimu_calibrator.cc.gbias.data(), msg, 3);
    else if (msg.fullMatch("/calibration/magn/vector")) set_floats(mimu_calibrator.cc.mbias.data(), msg, 3);
    else if (msg.fullMatch("/calibration/commit")) mimu_calibrator.updateTransforms();

    // set the fusion filter coefficients
    //     proportional feedback
    else if (msg.fullMatch("/filter_coefficients/k_P")) set_floats(&mimu_filter.fc.k_P, msg);
    //     integral feedback
    else if (msg.fullMatch("/filter_coefficients/k_I")) set_floats(&mimu_filter.fc.k_I, msg);
    //     accelerometer influence
    else if (msg.fullMatch("/filter_coefficients/k_a")) set_floats(&mimu_filter.fc.k_a, msg);
    //     magnetometer influence
    else if (msg.fullMatch("/filter_coefficients/k_m")) set_floats(&mimu_filter.fc.k_m, msg);

    // save and reload persistent state
    else if (msg.fullMatch("/persistent_state/load"))  load_persistent_state();
    else if (msg.fullMatch("/persistent_state/store")) store_persistent_state();
}

// @/
```

# esp.ino

## connecting to the network

We use the WiFiManager library to facilitate connecting to the local network.
This library causes the ESP to host a mubone network in station mode where the
captive log in page is used to provide the password for one of the networks
the ESP detects while scanning.

```cpp
// @+'esp includes'
// wifi manager includes
#include "ESP8266WiFi.h"
#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include <WiFiManager.h> 
// @/

// @+'connect to the network'
WiFiManager wifiman;
wifiman.autoConnect("mubone");
// @/
```

Once the connection is established, the local IP address can be inspected. This
is used to determine an appropriate output IP address, which by default will
broadcast to all devices on the network.

```cpp
// @+'esp definitions'
IPAddress device_address;
IPAddress output_address;
// @/

// @+'connect to the network'
device_address = WiFi.localIP();
output_address = device_address;
output_address[3] = 255; // broadcast;
// @/
```


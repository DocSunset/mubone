#include <EEPROM.h>
#include "OSCBundle.h"
#include "SLIPEncodedUSBSerial.h"
//#include "SLIPEncodedSerial.h"
#include "ADS1219.h"
#include "MIMU_MPU9250.h"
#include "MIMUCalibrator.h"
#include "MIMUFusion.h"
enum OSCInState {WAITING, MESSAGE, BUNDLE};

struct PersistentState
{
    MIMUCalibrationConstants mimucc;
    MIMUFilterCoefficients mimufc;
};

OSCBundle error_messages;
void osc_set_floats(float * f, int size, OSCMessage& msg)
{
    for (int i = 0; i < size; ++i) msg.set(i, f[i]);
}

SLIPEncodedUSBSerial usbserial{Serial};
//SLIPEncodedSerial hwserial{Serial1};

constexpr int num_buttons = 8;
constexpr int button_pins[num_buttons] = {5, 6, 7, 8, 2, 3, 4, 11};

constexpr int joystick_pin_x = A1;
constexpr int joystick_pin_y = A0;

using namespace ADS1219;

constexpr Address sps_address(ADDR0::DGND, ADDR1::DGND);
ADS1219_ADC sps_adc(sps_address, Wire);
constexpr MUX sps_channels[] = {MUX::AIN0_AIN1, MUX::AIN2_AIN3};

constexpr Address misc_adc_address(ADDR0::DVDD, ADDR1::DGND);
ADS1219_ADC misc_adc(misc_adc_address, Wire);
constexpr MUX misc_channels[] = {MUX::AIN0, MUX::AIN1, MUX::AIN2, MUX::AIN3};

MIMU_MPU9250 mimu;
MIMUCalibrator mimu_calibrator;
MIMUFusionFilter mimu_filter;

MIMUReading reading = MIMUReading::Zero();
Quaternion mimu_zero = Quaternion::Identity();

void mimu_tick()
{
    if (mimu.readInto(reading))
    {
        mimu_calibrator.calibrate(reading);
    }
    mimu_filter.fuse(reading.accl, reading.gyro, reading.magn);
}

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

bool any_button_pressed()
{
    for (int i = 0; i < num_buttons; ++i) 
    {
        if (digitalRead(button_pins[i]) == 0) return true;
    }
    return false;
}

void mimu_align()
{
    mimu_calibrator.resetAlignment();
    while(!any_button_pressed()) {/* wait */}
    delay(1000); // wait a second for the player to stabilize after
                 // pressing the button
    reading = mimu.readForMillis(3000);
    mimu_calibrator.calibrate(reading);
    Vector ybasis(reading.accl);
    while(!any_button_pressed()) {/* wait */}
    delay(1000); // wait a second for the player to stabilize after
                 // pressing the button
    reading = mimu.readForMillis(3000);
    mimu_calibrator.calibrate(reading);
    Vector zbasis(reading.accl);

    mimu_calibrator.setAlignment(ybasis, zbasis);
    mimu_initialize();
} 

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
        send_osc(usbserial, bundle, error_messages);
        //send_osc(hwserial, bundle, error_messages);
    }
}

void zero_sensors()
{
    Vector ybasis = mimu_filter.rotation.col(1);
    float azimuth = std::atan2(ybasis.y(), ybasis.x());
    mimu_zero = AngleAxis(azimuth, Vector::UnitZ());
}

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
void osc_dispatch(OSCBundle& bundle)
{
    for (int i = 0; i < bundle.size(); ++i)
    {
        osc_dispatch( *(bundle.getOSCMessage(i)) );
    }
}

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

void load_persistent_state()
{
    PersistentState persistent_state;
    const uint8_t *ptr = (const uint8_t*) &persistent_state;
    int count = sizeof(PersistentState);
    EEPtr e = 0x00;

    EEPROM.begin();
    for (; count; --count, ++e) (*e).update(*ptr++);
    EEPROM.end();

    mimu_calibrator.setCalibration(persistent_state.mimucc);
    mimu_filter.fc = persistent_state.mimufc;
}

void store_persistent_state()
{
    PersistentState persistent_state;
    uint8_t *ptr = (uint8_t*) &persistent_state;
    int count = sizeof(PersistentState)
    EEPtr e = 0x00;

    persistent_state.mimucc = mimu_calibrator.getCalibration();
    persistent_state.mimufc = mimu_filter.fc;

    EEPROM.begin()
    for (; count; --count, ++e) *ptr++ = *e;
    EEPROM.end();
}

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

void setup()
{
    usbserial.begin(2000000);
    //hwserial.begin(2000000);
    Wire.begin();

    for (const auto& pin : button_pins) pinMode(pin, INPUT_PULLUP);

    pinMode(joystick_pin_x, INPUT);
    pinMode(joystick_pin_y, INPUT);

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

    mimu.setup();
    mimu_calibrator.setup();
    mimu_filter.setup();

    mimu_initialize();
}

void loop()
{
    static OSCBundle bundle;

    static OSCMessage& buttons = bundle.add("/buttons");
    for (int i = 0; i < num_buttons; ++i) 
    {
        const int pin = button_pins[i];
        const int reading = digitalRead(pin);
        buttons.set(i, reading);
    }

    static OSCMessage& joystick = bundle.add("/joystick");
    joystick.set(0, analogRead(joystick_pin_x));
    joystick.set(1, analogRead(joystick_pin_y));

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

    static OSCMessage& normal = bundle.add("/normal");
    auto normal_vector = zeroed_matrix.col(1);
    osc_set_floats(normal_vector.data(), 3, normal);

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
    receive_osc(usbserial);
    //receive_osc(hwserial);
    send_osc(usbserial, bundle, error_messages);
    //send_osc(hwserial, bundle, error_messages);
}

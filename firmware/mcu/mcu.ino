#include "OSCBundle.h"
#include "SLIPEncodedUSBSerial.h"
#include "ADS1219.h"
#include "MIMU_MPU9250.h"
#include "MIMUCalibrator.h"
#include "MIMUFusion.h"
enum OSCInState {WAITING, MESSAGE, BUNDLE};

OSCBundle error_messages;
SLIPEncodedUSBSerial slipserial{Serial};

constexpr int num_buttons = 8;
constexpr int button_pins[num_buttons] = {5, 6, 7, 8, 2, 3, 4, 13};

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
Quaternion orientation = Quaternion::Identity();

void mimu_tick()
{
    if (mimu.readInto(reading))
    {
        mimu_calibrator.calibrate(reading);
    }
    orientation = mimu_filter.fuse(reading.accl, reading.gyro, reading.magn);
}

void mimu_initialize()
{
    while (!mimu.readInto(reading)); // wait until data is ready

    // initialize the orientation
    mimu_calibrator.calibrate(reading);
    orientation = mimu_filter.initializeFrom(reading.accl, reading.magn);
    
    float originalkp = mimu_filter.fc.k_P; // save current proportional feedback
    mimu_filter.fc.k_P = 21; // crank it up past 11

    // tick for 1.5s
    float start = millis();
    for (float dt = 0; dt < 1500; dt = millis() - start) mimu_tick();

    mimu_filter.fc.k_P = originalkp; // reset k_P
}
void mimu_align()
{
    mimu_calibrator.resetAlignment();
    delay(2000);
    reading = mimu.readForMillis(3000);
    mimu_calibrator.calibrate(reading);
    Vector ybasis(reading.accl);
    delay(2000);
    reading = mimu.readForMillis(3000);
    mimu_calibrator.calibrate(reading);
    Vector zbasis(reading.accl);

    mimu_calibrator.setAlignment(ybasis, zbasis);
} 

void mimu_calibrate()
{
    OSCBundle bundle;
    while (true)
    {
        for (int i = 0; i < num_buttons; ++i) 
        {
            if (digitalRead(button_pins[i]) == 0) break;
        }
        static OSCMessage& msg = bundle.add("/raw");
        mimu.readInto(reading);
        reading.updateBuffer();
        for (int i = 0; i < 9; ++i)
        {
            msg.set(i, reading.data[i]);
        }
        slipserial.beginPacket();
        bundle.send(slipserial);
        slipserial.endPacket();
        
        if (error_messages.size() > 0)
        {
            slipserial.beginPacket();
            error_messages.send(slipserial);
            slipserial.endPacket();
        }
        while (slipserial.available()) slipserial.read(); // ignore incoming messages
    }
}

void zero_sensors()
{
}
template<class OSCContainer>
OSCInState osc_receive(OSCContainer& osc, OSCInState initial_state)
{
    int size = slipserial.available();
    while(size--)
    {
        osc.fill(slipserial.read());
        if (slipserial.endofPacket())
        {
            if (!osc.hasError()) osc_dispatch(osc);
            osc.empty();
            return WAITING;
        }
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

void osc_dispatch(OSCMessage& msg)
{
    if (msg.fullMatch("/zero")) zero_sensors();
    else if (msg.fullMatch("/initialize")) mimu_initialize();
    else if (msg.fullMatch("/align")) mimu_align();
    else if (msg.fullMatch("/calibrate")) mimu_calibrate();
    else if (msg.fullMatch("/calibration/accl/matrix")) set_floats(mimu_calibrator.cc.acclcalibration.data(), msg, 9);
    else if (msg.fullMatch("/calibration/gyro/matrix")) set_floats(mimu_calibrator.cc.gyrocalibration.data(), msg, 9);
    else if (msg.fullMatch("/calibration/magn/matrix")) set_floats(mimu_calibrator.cc.magncalibration.data(), msg, 9);
    else if (msg.fullMatch("/calibration/accl/vector")) set_floats(mimu_calibrator.cc.abias.data(), msg, 3);
    else if (msg.fullMatch("/calibration/gyro/vector")) set_floats(mimu_calibrator.cc.gbias.data(), msg, 3);
    else if (msg.fullMatch("/calibration/magn/vector")) set_floats(mimu_calibrator.cc.mbias.data(), msg, 3);
    else if (msg.fullMatch("/filter_coefficients/k_P")) set_floats(&mimu_filter.fc.k_P, msg);
    else if (msg.fullMatch("/filter_coefficients/k_I")) set_floats(&mimu_filter.fc.k_I, msg);
    else if (msg.fullMatch("/filter_coefficients/k_a")) set_floats(&mimu_filter.fc.k_a, msg);
    else if (msg.fullMatch("/filter_coefficients/k_m")) set_floats(&mimu_filter.fc.k_m, msg);

}
void setup()
{
    slipserial.begin(9600); // TODO: set this to the max
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
    OSCBundle bundle;
    
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
                                  , &bundle.add("/joint1")
                                  , &bundle.add("/joint2")
                                  , &bundle.add("/rotator")
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
    
static OSCMessage& accl = bundle.add("/accl");
    static OSCMessage& gyro = bundle.add("/gyro");
    static OSCMessage& magn = bundle.add("/magn");
    static OSCMessage& quat = bundle.add("/quat");
    
    mimu_tick();
    
    accl.set(0, reading.accl.x());
    accl.set(1, reading.accl.y());
    accl.set(2, reading.accl.z());
    
    gyro.set(0, reading.gyro.x());
    gyro.set(1, reading.gyro.y());
    gyro.set(2, reading.gyro.z());
    
    magn.set(0, reading.magn.x());
    magn.set(1, reading.magn.y());
    magn.set(2, reading.magn.z());
    
    quat.set(0, orientation.w());
    quat.set(1, orientation.x());
    quat.set(2, orientation.y());
    quat.set(3, orientation.z());
    
    static OSCBundle bundle_in;
    static OSCMessage msg_in;
    
static OSCInState state = WAITING;
    if (state == WAITING)
    {
        if (slipserial.available())
        {
            if (slipserial.peek() == '#') state = BUNDLE;
            else state = MESSAGE;
        }
    }
    
    if      (state == MESSAGE) state = osc_receive(msg_in, state);
    else if (state == BUNDLE)  state = osc_receive(bundle_in, state);
    slipserial.beginPacket();
    bundle.send(slipserial);
    slipserial.endPacket();
    
    if (error_messages.size() > 0)
    {
        slipserial.beginPacket();
        error_messages.send(slipserial);
        slipserial.endPacket();
    }
}
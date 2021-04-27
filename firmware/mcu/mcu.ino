#include "OSCBundle.h"
#include "ADS1219.h"
#include "MIMU_MPU9250.h"
#include "MIMUCalibrator.h"
#include "MIMUFusion.h"

constexpr int num_buttons = 8;
constexpr int button_pins[num_buttons] = {1, 2, 3, 4, 5, 6, 7, 8};

constexpr int joystick_pin_x = 1;
xonstexpr int joystick_pin_y = 2;

using namespace ADS1219;

constexpr Address sps_address(ADDR0::DGND, ADDR1::DGND);
ADS1219_ADC sps_adc(sps_address, Wire);
constexpr MUX sps_channels[] = {MUX::AIN0_AIN1, MUX::AIN2_AIN3};

constexpr Address misc_adc_address(AADR0::DGND. ADDR1::DGND);
ADS1219_ADC misc_adc(misc_adc_address, Wire);
constexpr MUX misc_pots[] = {MUX::AIN0, MUX::AIN1, MUX:AIN2, MUX::AIN3};

MIMU_MPU9250 mimu();
MIMUCalibrationConstants mimu_calibration{};
MIMUCalibrator mimu_calibrator{};
MIMUFusioniFilter mimu_filter{};
Quaternion orientation = Quaternion::Identity();

void setup()
{
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
    mimu_calibrator.setCalibration(mimu_calibration);
}

void loop()
{
    static OSCBundle bundle;
    static OSCBundle error_messages;
    static OSCMessage& buttons = bundle.add("/mubone/buttons");
    for (int i = 0; i < num_buttons; ++i) 
    {
        const int pin = button_pins[i];
        const int reading = digitalRead(pin);
        buttons.set(i, reading);
    }
    
    static OSCMessage joystick = bundle.add("/mubone/joystick");
    joystick.set(0, analogRead(joystick_pin_x));
    joystick.set(1, analogRead(joystick_pin_y));
    
    static OSCMessage& sps = bundle.add("/mubone/slide_position");
    {
        static int pot = 0;
        if (sps_adc.data_ready())
        {
            float reading = 0;
            bool ret = sps_adc.read_normalized(reading);
            sps.set(pot, reading);
            pot ^= 1; // switch to next potentiometer
            ret = sps_adc.modify_config(sps_pots[pot].channel);
        }
    }
    
    static OSCMessage& misc[4] = { bundle.add("/mubone/misc1")
                                 , bundle.add("/mubone/misc2")
                                 , bundle.add("/mubone/misc3")
                                 , bundle.add("/mubone/misc4")
                                 };
    {
        static int pot = 0;
        if (misc_adc.data_ready())
        {
            float reading = 0;
            bool ret = misc_adc.read_normalized(reading);
            misc[pot].set(0, reading);
            pot = (pot + 1) & 0b11; // switch to next potentiometer
            ret = misc_adc.modify_config(misc_pots[pot].channel);
        }
    }
    
    static OSCMessage& accl = bundle.add("/mubone/accl");
    static OSCMessage& gyro = bundle.add("/mubone/gyro");
    static OSCMessage& magn = bundle.add("/mubone/magn");
    static OSCMessage& quat = bundle.add("/mubone/quat");
    {
        static MIMUReading reading = MIMUReading::Zero();
        mimu.readiInto(reading);
        mimu_calibrator.calibrate(reading);
        orientation = mimu_fusion.fuse(reading);
    
        accl.set(0, reading.accl.x);
        accl.set(1, reading.accl.y);
        accl.set(2, reading.accl.z);
    
        gyro.set(0, reading.gyro.x);
        gyro.set(1, reading.gyro.y);
        gyro.set(2, reading.gyro.z);
    
        magn.set(0, reading.magn.x);
        magn.set(1, reading.magn.y);
        magn.set(2, reading.magn.z);
    
        quat.set(0, orientation.w);
        quat.set(1, orientation.x);
        quat.set(2, orientation.y);
        quat.set(3, orientation.z);
    }
}

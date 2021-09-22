#include <Arduino.h>
#include <Adafruit_PWMServoDriver.h>
#include <DmxSimple.h>
#include "./Arduino-Renard.h"
#include "Skeleton.h"
#include "TwoAxisSkeleton.h"
#include "ControlModes.h"

// Main Skeleton Show Sketch
// DMX Channels:
//  1 - MiniSkeleton 1 control mode - see CONTROL_MODE_* constants
//  2 - MiniSkeleton 1 Input 1  0 - 255
//  3 - MiniSkeleton 1 Input 2  0 - 255
//  4 - MiniSkeleton 1 Input 3  0 - 255
//  5 - Skeleton 2 control mode - see CONTROL_MODE_* constants
//  6 - Skeleton 2 Input 1 
//  7 - Skeleton 2 Input 2
//  8 - Skeleton 2 Input 3
//  9 - Skeleton 3 control mode
// 10 - Skeleton 3 Input 1
// 11 - Skeleton 3 Input 2
// 12 - Skeleton 3 Input 3
// 13 - Skeleton 4 control mode
// 14 - Skeleton 5 Input 1
// 15 - Skeleton 6 Input 2
// 16 - Skeleton 7 Input 3
// 17 - Par Light 1 CH 1
// 18 - Par Light 1 CH 2
// 19 - Par Light 1 CH 3
// 20 - Par Light 1 CH 4
// 21 - Par Light 1 CH 5
// 22 - Par Light 1 CH 6
// 23 - Par Light 1 CH 7


// All channels received from renard are also proxied directly to DMX
// Par light channel descriptions:
// CH1 - master dimmer (off -> on)
// CH2 - Red Dimming
// CH3 - Green Dimming
// CH4 - Blue Dimming
// CH5 - Flicker mode:
//      0 - 7   | No Flicker
//      8 - 255 | Strobe, from slow to fast
// CH6 - Control Mode
//       0 -  10   | Manual Control
//      11 -  60   | Color Selection
//      61 - 110   | Gradual Fade
//     111 - 160   | Variable Pulse
//     161 - 210   | Jump Change
//     211 - 255   | Sound activated mode
// CH7 - Color selection / discoloration speed, from slow to fast

#define SERVO_FREQ 50 

#define SK1_PWM_TILT_SERVO_INDEX 0
#define SK1_PWM_PAN_SERVO_INDEX 1
#define SK1_PWM_JAW_SERVO_INDEX 2
#define SK2_PWM_STG_RIGHT_SERVO_INDEX 3
#define SK2_PWM_STG_LEFT_SERVO_INDEX 4
#define SK2_PWM_JAW_SERVO_INDEX 5

#define CHANNELS 8
#define BAUD_RATE 57600

Adafruit_PWMServoDriver pwm = Adafruit_PWMServoDriver(0x40);
Renard renard;
byte values[CHANNELS];

// Mini skeleton 1 - full pan/tilt and jaw
Skeleton s = Skeleton(
    pwm,
    115, 499,
    SK1_PWM_PAN_SERVO_INDEX, SK1_PWM_TILT_SERVO_INDEX, SK1_PWM_JAW_SERVO_INDEX,
    0, 40,   // jaw min / max pct
    0, 100,  // pan min / max pct
    0, 100   // tilt min / max pct
);

// Full sized skeleton 2 - 2-axis + jaw
TwoAxisSkeleton s2 = TwoAxisSkeleton(
    pwm,
    SK2_PWM_STG_LEFT_SERVO_INDEX, 100, 300, 500,
    SK2_PWM_STG_RIGHT_SERVO_INDEX, 500, 300, 100,
    SK2_PWM_JAW_SERVO_INDEX, 100, 500
);

void setup() {
    pwm.begin();
    pwm.setPWMFreq(SERVO_FREQ);

    renard.begin((uint8_t*)&values, CHANNELS, &Serial, BAUD_RATE);
    
    s.setPan(128);
    s.setTilt(128);
    s.setJaw(0);

    DmxSimple.maxChannel(CHANNELS);
}

byte sk1ControlMode, sk1Input1, sk1Input2, sk1Input3;
byte sk2ControlMode, sk2Input1, sk2Input2, sk2Input3;

void loop() {
    if (renard.receive()) {
        // Update all of the skeletons
        sk1ControlMode      = values[0];
        sk1Input1           = values[1];
        sk1Input2           = values[2];
        sk1Input3           = values[3];
        sk2ControlMode      = values[4];
        sk2Input1           = values[5];
        sk2Input2           = values[6];
        sk2Input3           = values[7];

        s.updateControlMode(sk1ControlMode, sk1Input1, sk1Input2, sk1Input3);
        s2.updateControlMode(sk2ControlMode, sk2Input1, sk2Input2, sk2Input3);

        // relay all channels to DMX
        for (uint16_t c = 0 ; c < CHANNELS ; c++) {
            DmxSimple.write(c+1, values[c]);
        }
    }
    s.loop();
}

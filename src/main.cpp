#include "Arduino-Renard.h"
#include "ControlModes.h"
#include "Skeleton.h"
#include "TriaxialSkeleton.h"
#include "TwoAxisSkeleton.h"
#include <Adafruit_PWMServoDriver.h>
#include <Arduino.h>
#include <DmxSimple.h>

#define MULT 1
#define SERVO_FREQ 50 * MULT

#define SK1_PWM_TILT_SERVO_INDEX 0
#define SK1_PWM_PAN_SERVO_INDEX 1
#define SK1_PWM_JAW_SERVO_INDEX 2
#define SK2_PWM_STG_RIGHT_SERVO_INDEX 4
#define SK2_PWM_STG_LEFT_SERVO_INDEX 7
#define SK2_PWM_JAW_SERVO_INDEX 5
#define SK3_PWM_PAN_SERVO_INDEX 12
#define SK3_PWM_TILT_SERVO_INDEX 13
#define SK3_PWM_YAW_SERVO_INDEX 14
#define SK3_PWM_JAW_SERVO_INDEX 15
#define SK4_PWM_PAN_SERVO_INDEX 8
#define SK4_PWM_TILT_SERVO_INDEX 9
#define SK4_PWM_YAW_SERVO_INDEX 10
#define SK4_PWM_JAW_SERVO_INDEX 11

#define SK1_DMX_OFFSET 0
#define SK2_DMX_OFFSET 4
#define SK3_DMX_OFFSET 8
#define SK4_DMX_OFFSET 13

#define CHANNELS 64
#define BAUD_RATE 57600

Adafruit_PWMServoDriver pwm = Adafruit_PWMServoDriver(0x40);
Renard renard;
byte values[CHANNELS];

// Mini skeleton 1 - full pan/tilt and jaw
Skeleton s =
    Skeleton(pwm, 115, 499, SK1_PWM_PAN_SERVO_INDEX, SK1_PWM_TILT_SERVO_INDEX,
             SK1_PWM_JAW_SERVO_INDEX, 0, 40, // jaw min / max pct
             0, 100,                         // pan min / max pct
             10, 80                          // tilt min / max pct
    );

// Full sized skeleton 2 - 2-axis + jaw
TwoAxisSkeleton s2 =
    TwoAxisSkeleton(pwm, SK2_PWM_STG_LEFT_SERVO_INDEX, 100, 300, 500,
                    SK2_PWM_STG_RIGHT_SERVO_INDEX, 500, 300, 100,
                    SK2_PWM_JAW_SERVO_INDEX, 400, 150);

// skeleton 3, full triaxial head
TriaxialSkeleton s3 =
    TriaxialSkeleton(pwm, MULT * 100, MULT * 500, SK3_PWM_PAN_SERVO_INDEX,
                     SK3_PWM_TILT_SERVO_INDEX, SK3_PWM_YAW_SERVO_INDEX,
                     SK3_PWM_JAW_SERVO_INDEX, //
                     50, 100,                 // yaw pct min / max
                     20, 80,                  // jaw pct min / max
                     0, 100,                  // pan pct min / max
                     40, 100                  // tilt pct min / max
    );

// skeleton 4, full triaxial head
TriaxialSkeleton s4 =
    TriaxialSkeleton(pwm, MULT * 100, MULT * 500, SK4_PWM_PAN_SERVO_INDEX,
                     SK4_PWM_TILT_SERVO_INDEX, SK4_PWM_YAW_SERVO_INDEX,
                     SK4_PWM_JAW_SERVO_INDEX, //
                     0, 100,                  // yaw pct min / max
                     0, 50,                   // jaw pct min / max
                     20, 75,                  // pan pct min / max
                     20, 70                   // tilt pct min / max
    );

byte sk1ControlMode, sk1Input1, sk1Input2, sk1Input3;
byte sk2ControlMode, sk2Input1, sk2Input2, sk2Input3;
byte sk3ControlMode, sk3Input1, sk3Input2, sk3Input3, sk3Input4;
byte sk4ControlMode, sk4Input1, sk4Input2, sk4Input3, sk4Input4;

void dmxCallback(int universe, byte values[CHANNELS]) {
  // Update all of the skeletons
  sk1ControlMode = CONTROL_MODE_DMX; // values[SK1_DMX_OFFSET];
  sk1Input1 = values[SK1_DMX_OFFSET + 1];
  sk1Input2 = values[SK1_DMX_OFFSET + 2];
  sk1Input3 = values[SK1_DMX_OFFSET + 3];
  sk2ControlMode = CONTROL_MODE_DMX; // values[SK2_DMX_OFFSET];
  sk2Input1 = values[SK2_DMX_OFFSET + 1];
  sk2Input2 = values[SK2_DMX_OFFSET + 2];
  sk2Input3 = values[SK2_DMX_OFFSET + 3];
  sk3ControlMode = CONTROL_MODE_DMX; // values[SK3_DMX_OFFSET];
  sk3Input1 = values[SK3_DMX_OFFSET + 1];
  sk3Input2 = values[SK3_DMX_OFFSET + 2];
  sk3Input3 = values[SK3_DMX_OFFSET + 3];
  sk3Input4 = values[SK3_DMX_OFFSET + 4];
  sk4ControlMode = CONTROL_MODE_DMX; // values[SK4_DMX_OFFSET];
  sk4Input1 = values[SK4_DMX_OFFSET + 1];
  sk4Input2 = values[SK4_DMX_OFFSET + 2];
  sk4Input3 = values[SK4_DMX_OFFSET + 3];
  sk4Input4 = values[SK4_DMX_OFFSET + 4];

  s.updateControlMode(sk1ControlMode, sk1Input1, sk1Input2, sk1Input3);
  s2.updateControlMode(sk2ControlMode, sk2Input1, sk2Input2, sk2Input3);
  s3.updateControlMode(sk3ControlMode, sk3Input1, sk3Input2, sk3Input3,
                       sk3Input4);
  s4.updateControlMode(sk4ControlMode, sk4Input1, sk4Input2, sk4Input3,
                       sk4Input4);

  // relay all channels to DMX
  for (uint16_t c = 0; c < CHANNELS; c++) {
    DmxSimple.write(c + 1, values[c]);
  }
}

void setup() {
  Serial.begin(BAUD_RATE);
  pwm.begin();
  pwm.setPWMFreq(SERVO_FREQ);

  DmxSimple.maxChannel(CHANNELS);
  for (uint16_t c = 0; c < CHANNELS; c++) {
    values[c] = 0;
    DmxSimple.write(c + 1, values[c]);
  }

  s.setPan(128);
  s.setTilt(128);
  s.setJaw(0);

  s3.setPan(128);
  s3.setTilt(128);
  s3.setYaw(128);
  s3.setJaw(0);

  s4.setPan(128);
  s4.setTilt(128);
  s4.setYaw(128);
  s4.setJaw(0);

  renard.begin((uint8_t *)&values, CHANNELS, &Serial, BAUD_RATE);
}

void loop() {
  if (renard.receive()) {
    dmxCallback(1, values);
  }
}

#include <DMXSerial.h>
#include "./dmx_controller.h"

#define DMX_DIRECT_SET_EYE_RED         1
#define DMX_DIRECT_SET_EYE_GREEN       2
#define DMX_DIRECT_SET_EYE_BLUE        3
#define DMX_DIRECT_MOTOR_START         4
#define DMX_DIRECT_MOTOR_STOP          DMX_DIRECT_MOTOR_START + SERVO_COUNT

void SkullDMXController::init(
    unsigned int startChannel,
    Servo *s[SERVO_COUNT],
    unsigned int eyeR,
    unsigned int eyeG,
    unsigned int eyeB
) {
  this->startChannel = startChannel;
  this->eyeR = eyeR;
  this->eyeG = eyeG;
  this->eyeB = eyeB;
  this->servos = s;
}

void SkullDMXController::loop() {
    // Read the values and react accordingly
    if (DMXSerial.dataUpdated()) {
        // read the eye colors
        byte _eyeR = DMXSerial.read(startChannel + DMX_DIRECT_SET_EYE_RED);
        byte _eyeG = DMXSerial.read(startChannel + DMX_DIRECT_SET_EYE_GREEN);
        byte _eyeB = DMXSerial.read(startChannel + DMX_DIRECT_SET_EYE_BLUE);
        analogWrite(eyeR, 255 - _eyeR);
        analogWrite(eyeG, 255 - _eyeG);
        analogWrite(eyeB, 255 - _eyeB);

        // read in the motor values
        for(int cp = startChannel + DMX_DIRECT_MOTOR_START ; cp <= startChannel + DMX_DIRECT_MOTOR_STOP ; cp++) {
            byte val = DMXSerial.read(cp);
            Serial.print("SETVAL: ");
            Serial.print(cp - startChannel - DMX_DIRECT_MOTOR_START);
            Serial.print(" TO ");
            Serial.println(val);
            servos[cp - startChannel - DMX_DIRECT_MOTOR_START]->setPosition(val);
        }
        
        DMXSerial.resetUpdated();
    }
    for (int i = 0 ; i < SERVO_COUNT ; i++) {
      servos[i]->loop();
    }
}



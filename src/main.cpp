#include <Arduino.h>
#include <Wire.h>
#include <DMXSerial.h>
#include <Adafruit_PWMServoDriver.h>
#include "./servos.h"
#include "./config_controller.h"
#include "./dmx_controller.h"

// TODO: support inverting motors (so that 0 - 255 is like 255 - 0)
//       provide support for configuring that in configuration mode

#define BAUD_RATE 57600
#define PWM_FREQ 50 // 1600 is the max, 50 seems right for servos

// Define the pins for the eyes
#define EYE_GREEN_PIN 6
#define EYE_RED_PIN 5
#define EYE_BLUE_PIN 8

// Define the pins for each of the address dip switches
#define DIP_0 4
#define DIP_1 7
#define DIP_2 9
#define DIP_3 21 
#define DIP_4 20
#define DIP_5 19
#define DIP_6 18
#define DIP_7 10

// Define the pins for the expansion slots
#define EXP_1 15
#define EXP_2 14
#define EXP_3 16

// Initialize the controllers and servos
Adafruit_PWMServoDriver pwm1 = Adafruit_PWMServoDriver(0x40);
Servo servos[SERVO_COUNT];
SkullDMXController dmxController;
ConfigController cfgController;

// A function to read the current value of the dip switches
byte readDips() {
  byte p1 = digitalRead(DIP_0) == 0 ? 0b10000000 : 0b00000000;
  byte p2 = digitalRead(DIP_1) == 0 ? 0b01000000 : 0b00000000;
  byte p3 = digitalRead(DIP_2) == 0 ? 0b00100000 : 0b00000000;
  byte p4 = digitalRead(DIP_3) == 0 ? 0b00010000 : 0b00000000;
  byte p5 = digitalRead(DIP_4) == 0 ? 0b00001000 : 0b00000000;
  byte p6 = digitalRead(DIP_5) == 0 ? 0b00000100 : 0b00000000;
  byte p7 = digitalRead(DIP_6) == 0 ? 0b00000010 : 0b00000000;
  byte p8 = digitalRead(DIP_7) == 0 ? 0b00000001 : 0b00000000;
  return p1 + p2 + p3 + p4 + p5 + p6 + p7 + p8;
}

// used to control the state (configuration mode vs not)
byte reprogram = 0;
void setup() {
  Serial.begin(BAUD_RATE);
  pwm1.begin();
  pwm1.setPWMFreq(PWM_FREQ);  // This is the maximum PWM frequency
  DMXSerial.init(DMXReceiver);

  pinMode(EYE_RED_PIN, OUTPUT);
  pinMode(EYE_GREEN_PIN, OUTPUT);
  pinMode(EYE_BLUE_PIN, OUTPUT);
  pinMode(17, OUTPUT);

  pinMode(DIP_0, INPUT_PULLUP);
  pinMode(DIP_1, INPUT_PULLUP);
  pinMode(DIP_2, INPUT_PULLUP);
  pinMode(DIP_3, INPUT_PULLUP);
  pinMode(DIP_4, INPUT_PULLUP);
  pinMode(DIP_5, INPUT_PULLUP);
  pinMode(DIP_6, INPUT_PULLUP);
  pinMode(DIP_7, INPUT_PULLUP);

  Servo *servoPtrs[SERVO_COUNT];
  for(int i = 0 ; i < SERVO_COUNT ; i++) {
    servoPtrs[i] = &servos[i];
    servos[i].init(i);
  }

  byte ep[3] = { EYE_RED_PIN, EYE_GREEN_PIN, EYE_BLUE_PIN };
  cfgController.init(servoPtrs, ep);

  if (readDips() == 255) {
    reprogram = 1;
  } else if (cfgController.waitForConfigMode()) {
    reprogram = 1;
  } else {
    unsigned int startChannel = readDips() - 1;
    dmxController.init(
      startChannel,
      servoPtrs,
      EYE_RED_PIN, EYE_GREEN_PIN, EYE_BLUE_PIN
    );
  }
  cfgController.initFromEEPROM();
}

void loop() {
  if (reprogram) {
    cfgController.loop();
  } else {
    dmxController.loop();
  }
}

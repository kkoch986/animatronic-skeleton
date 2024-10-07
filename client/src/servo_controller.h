#pragma once

#include <PCA9685.h>
#include <Preferences.h>

#include "config.h"

// I2C Configuration for the PCA9685
// 85 = B1010101
#define PCA9685_I2C_ADDR 85
#define PCA9685_I2C_SDA 2
#define PCA9685_I2C_SCL 14
#define PWM_UPDATE_INTERVAL_MS 10

enum StateRestoreStatus {
  SRS_OK,
  SRS_INVALID_STATE_HEADER,
};

const byte STATE_DUMP_EEPROM_HEADER[] = {0xAA, 0x01};

class ServoController {
private:
  PCA9685 pwmController{PCA9685_I2C_ADDR};
  volatile bool enabled[SERVO_COUNT];
  volatile uint16_t targetValue[SERVO_COUNT];
  volatile uint16_t currentValue[SERVO_COUNT];
  volatile uint16_t centerValues[SERVO_COUNT];
  volatile bool arrived[SERVO_COUNT];
  volatile int16_t lowerLimit[SERVO_COUNT];
  volatile int16_t upperLimit[SERVO_COUNT];
  volatile char labels[SERVO_COUNT][SERVO_MAX_LABEL_SIZE];
  uint32_t lastMillis = 0;

  uint16_t pwmForVal(byte index, byte val);
  void defaultLabel(byte index, volatile char *buff);

public:
  bool setup();
  void loop();

  uint8_t motorCount();
  bool isEnabled(byte index);

  void center();
  void move(byte index, byte val);
  void set(byte index, byte val);
  void setEyeColor(byte r, byte g, byte b);
  void setEyeRed(byte r);
  void setEyeGreen(byte g);
  void setEyeBlue(byte b);

  void setLowerLimit(byte index, byte val);
  void setUpperLimit(byte index, byte val);
  void setEnabled(byte index, bool enabled);
  void setLabel(byte index, char *label, byte labelSize);

  // buff must be a buffer with at least stateDumpLength() bytes
  // copies a structured byte array into buff
  // representing the state of all of the motors. the format is as follows:
  // [TOTAL_MOTOR_COUNT][LABEL_MAX_SIZE]
  // the following pattern is then repeated for each motor sequentially:
  // [MOTOR_X_ENABLED][MOTOR_X_MIN][MOTOR_X_MAX][MOTOR_X_IDLE][MOTOR_X_SMOOTH]
  // [MOTOR_X_CURRENT][MOTOR_X_TARGET]
  // [MOTOR_X_LABEL_SIZE][MOTOR_X_LABEL (LABEL_MAX_SIZE bytes long)]
  void dumpState(byte *buff);

  // actually commit a dumped state to non-volatile storage
  void commitState();

  // restore the state of the motors from a structured byte array
  // the expected format is the same as the one used in dumpState
  // with the 2 byte version header prepended
  StateRestoreStatus restoreFromStateDump();

  // return the length of a full state dump
  // the format is defined so that all state dumps will be a
  // deterministic size based on the number of servos and the max label size
  uint16_t stateDumpLength();
};

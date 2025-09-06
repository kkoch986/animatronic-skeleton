#include "servo_controller.h"
#ifdef ENABLE_DEBUG
#include <Arduino.h>
#endif

Preferences prefs;

void ServoController::center() {
  for (int i = 0; i < SERVO_COUNT; i++) {
    targetValue[i] = centerValues[i];
  }
}

bool ServoController::setup() {
  byte err;
  Wire.begin(PCA9685_I2C_SDA, PCA9685_I2C_SCL);
  pwmController.resetDevices();
  pwmController.init();
#ifdef PCA9685_ENABLE_DEBUG_OUTPUT
  pwmController.printModuleInfo();
#endif
  pwmController.setPWMFreqServo();
  err = pwmController.getLastI2CError();

  StateRestoreStatus srs = restoreFromStateDump();
  if (srs != SRS_OK) {
    for (int i = 0; i < SERVO_COUNT; i++) {
      lowerLimit[i] = 0;
      upperLimit[i] = 255;
      defaultLabel(i, labels[i]);

      // set arrived = true so nothing moves at first
      arrived[i] = true;
      targetValue[i] = pwmForVal(i, 0);
      currentValue[i] = pwmForVal(i, 127);
    }
  }

  setEyeColor(255, 0, 0);
  delay(1000);
  setEyeColor(0, 255, 0);
  delay(1000);
  setEyeColor(0, 0, 255);
  delay(1000);
  setEyeColor(255, 255, 255);
  return err == 0;
}

void ServoController::setEyeColor(byte r, byte g, byte b) {
  setEyeRed(r);
  setEyeGreen(g);
  setEyeBlue(b);
}

void ServoController::setEyeRed(byte r) {
  pwmController.setChannelPWM(13, map(r, 0, 255, 0, 4096));
}
void ServoController::setEyeGreen(byte g) {
  pwmController.setChannelPWM(14, map(g, 0, 255, 0, 4096));
}
void ServoController::setEyeBlue(byte b) {
  pwmController.setChannelPWM(15, map(b, 0, 255, 0, 4096));
}

uint8_t ServoController::motorCount() { return SERVO_COUNT; }

bool ServoController::isEnabled(byte index) {
  if (index >= SERVO_COUNT) {
    return false;
  }
  return enabled[index];
}

byte ServoController::getCurrentPosition(byte index) {
  if (index >= SERVO_COUNT) {
    return 0;
  }
  uint16_t minPWM = pwmForVal(index, 0);
  uint16_t maxPWM = pwmForVal(index, 255);
  return map(currentValue[index], minPWM, maxPWM, 0, 255);
}

byte ServoController::getTargetPosition(byte index) {
  if (index >= SERVO_COUNT) {
    return 0;
  }
  uint16_t minPWM = pwmForVal(index, 0);
  uint16_t maxPWM = pwmForVal(index, 255);
  return map(targetValue[index], minPWM, maxPWM, 0, 255);
}

byte ServoController::getCenterPosition(byte index) {
  if (index >= SERVO_COUNT) {
    return 0;
  }
  uint16_t minPWM = pwmForVal(index, 0);
  uint16_t maxPWM = pwmForVal(index, 255);
  return map(centerValues[index], minPWM, maxPWM, 0, 255);
}

byte ServoController::getLowerLimit(byte index) {
  if (index >= SERVO_COUNT) {
    return 0;
  }
  return lowerLimit[index];
}

byte ServoController::getUpperLimit(byte index) {
  if (index >= SERVO_COUNT) {
    return 255;
  }
  return upperLimit[index];
}

bool ServoController::hasArrived(byte index) {
  if (index >= SERVO_COUNT) {
    return true;
  }
  return arrived[index];
}

uint16_t ServoController::pwmForVal(byte index, byte val) {
  return map(val, 0, 255, map(lowerLimit[index], 0, 255, 102, 512),
             map(upperLimit[index], 0, 255, 102, 512));
}

void ServoController::loop() {
  uint32_t currentMillis = millis();
  bool updatePWM = currentMillis - lastMillis >= PWM_UPDATE_INTERVAL_MS;
  if (updatePWM) {
    for (int i = 0; i < SERVO_COUNT; i++) {
      if (!enabled[i]) {
        continue;
      }
      if (!arrived[i]) {
        pwmController.setChannelPWM(i, targetValue[i]);
        arrived[i] = true;
      }
    }
    lastMillis = currentMillis;
  }
}

void ServoController::setEnabled(byte index, bool e) {
  if (index >= SERVO_COUNT) {
    return;
  }
  enabled[index] = e;
}

// LATER: theres some kind of bug in the overflow here that causes it
// to clear out the next label.
void ServoController::setLabel(byte index, char *label, byte labelSize) {
  if (index >= SERVO_COUNT) {
    return;
  }
  if (labelSize > SERVO_MAX_LABEL_SIZE) {
    labelSize = SERVO_MAX_LABEL_SIZE;
  }
  for (byte i = 0; i < labelSize; i++) {
    labels[index][i] = label[i];
  }
  labels[index][labelSize] = '\0';
}

void ServoController::move(byte index, byte val) {
  if (index >= SERVO_COUNT) {
    return;
  }
  targetValue[index] = pwmForVal(index, val);
  arrived[index] = false;
}

void ServoController::set(byte index, byte val) {
  if (index >= SERVO_COUNT) {
    return;
  }
  uint16_t t = pwmForVal(index, val);
  targetValue[index] = t;
  currentValue[index] = t;
  arrived[index] = false;
}

void ServoController::setLowerLimit(byte index, byte val) {
  if (index >= SERVO_COUNT) {
    return;
  }
  lowerLimit[index] = val;
  if (targetValue[index] < pwmForVal(index, val)) {
    targetValue[index] = pwmForVal(index, val);
    arrived[index] = false;
  }
}

void ServoController::setUpperLimit(byte index, byte val) {
  if (index >= SERVO_COUNT) {
    return;
  }
  upperLimit[index] = val;
  if (targetValue[index] > pwmForVal(index, val)) {
    targetValue[index] = pwmForVal(index, val);
    arrived[index] = false;
  }
}

uint16_t ServoController::stateDumpLength() {
  byte headerSize = 2;
  return headerSize + (SERVO_COUNT * SERVO_DUMP_BYTES_PER_SERVO);
}

// index should not be a number with more than SERVO_MAX_LABEL_SIZE - 2 digits
// and buff must be at least SERVO_MAX_LABEL_SIZE bytes long
// LATER: assert the above ^
void ServoController::defaultLabel(byte index, volatile char *buff) {
  sprintf((char *)buff, "S_%d", index);
}

// ret must be a buffer with at least stateDumpLength() bytes
void ServoController::dumpState(byte *ret) {
  ret[0] = SERVO_COUNT;
  ret[1] = SERVO_MAX_LABEL_SIZE;

  for (byte i = 0; i < SERVO_COUNT; i++) {
    uint16_t minPWM = pwmForVal(i, 0);
    uint16_t maxPWM = pwmForVal(i, 255);
    uint16_t start = 2 + (i * SERVO_DUMP_BYTES_PER_SERVO);
    ret[start] = enabled[i];
    ret[start + 1] = lowerLimit[i];
    ret[start + 2] = upperLimit[i];
    ret[start + 3] = 0; // idle
    ret[start + 4] = 0; // smooth
    ret[start + 5] = map(currentValue[i], minPWM, maxPWM, 0, 255);
    ret[start + 6] = map(targetValue[i], minPWM, maxPWM, 0, 255);
    ret[start + 7] = arrived[i];
    memcpy(&ret[start + 9], (char *)labels[i], SERVO_MAX_LABEL_SIZE);
    ret[start + 9 + SERVO_MAX_LABEL_SIZE] = '\0';
    ret[start + 8] = strlen((char *)&ret[start + 9]);
  }
}

char *ServoController::getLabel(byte index) {
  if (index >= SERVO_COUNT) {
    return nullptr;
  }
  return (char *)labels[index];
}

void ServoController::commitState() {
  uint16_t len = stateDumpLength() + 2;
  byte buff[len];
  buff[0] = STATE_DUMP_EEPROM_HEADER[0];
  buff[1] = STATE_DUMP_EEPROM_HEADER[1];
  dumpState(&buff[2]);

  prefs.begin("servo_state", false);
  prefs.putBytes("state", buff, len);
  prefs.end();
}

StateRestoreStatus ServoController::restoreFromStateDump() {
  uint16_t len = stateDumpLength() + 2;
  byte buff[len];

  prefs.begin("servo_state", true);
  prefs.getBytes("state", buff, len);
  prefs.end();

  // confirm the first 2 bytes
  if (buff[0] != STATE_DUMP_EEPROM_HEADER[0] ||
      buff[1] != STATE_DUMP_EEPROM_HEADER[1]) {
    return SRS_INVALID_STATE_HEADER;
  }

  byte *b = buff + 2;
  byte servoCount = b[0];
  byte labelSize = b[1];
  int destinationLabelSize = labelSize;
  if (destinationLabelSize > SERVO_MAX_LABEL_SIZE) {
    destinationLabelSize = SERVO_MAX_LABEL_SIZE;
  }

  if (servoCount > SERVO_COUNT) {
    servoCount = SERVO_COUNT;
  }
  for (byte i = 0; i < SERVO_COUNT; i++) {
    uint16_t start = 2 + (i * SERVO_DUMP_BYTES_PER_SERVO);
    enabled[i] = b[start];
    lowerLimit[i] = b[start + 1];
    upperLimit[i] = b[start + 2];

    uint16_t minPWM = pwmForVal(i, 0);
    uint16_t maxPWM = pwmForVal(i, 255);

    // skip 3 & 4 (idle & smooth)
    currentValue[i] = map(b[start + 5], 0, 255, minPWM, maxPWM);
    centerValues[i] = map(b[start + 5], 0, 255, minPWM, maxPWM);
    targetValue[i] = map(b[start + 6], 0, 255, minPWM, maxPWM);
    arrived[i] = false;
    for (byte j = 0; j < destinationLabelSize; j++) {
      labels[i][j] = b[start + 9 + j];
    }
  }
  return SRS_OK;
}

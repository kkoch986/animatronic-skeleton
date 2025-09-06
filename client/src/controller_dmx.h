#pragma once
#include "servo_controller.h"

class DMXController {
private:
  ServoController *servoController;
  bool send;
  uint8_t dmxOffset;
  uint8_t dmxData[512];
  static DMXController *instance;

public:
  static DMXController *getInstance() {
    if (!instance) {
      instance = new DMXController();
    }
    return instance;
  }
  void setup();
  void loop();
  void sendOnNext();

  void setServoController(ServoController *sc) { servoController = sc; }
  void setDMXOffset(uint8_t offset) { dmxOffset = offset; }
  uint8_t getDMXOffset() { return dmxOffset; }
  uint8_t getDMXValue(uint16_t slot);
};

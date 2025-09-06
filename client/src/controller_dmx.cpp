#include "controller_dmx.h"
#include <LXESP8266UARTDMX.h>

DMXController *DMXController::instance;

// LATER: figure out why this doesnt seem to trigger often enough.
//        when leveraging the send flag, we end up with these massive lags
//        between moves
void onDataReceivedCallback(int slots) {
  DMXController::getInstance()->sendOnNext();
}

void DMXController::sendOnNext() { send = true; }

void DMXController::setup() {
  send = false;
  ESP8266DMX.setDataReceivedCallback(onDataReceivedCallback);
  ESP8266DMX.startInput();
}

void DMXController::loop() {
  for (int i = 0; i < servoController->motorCount(); i++) {
    uint8_t sval = ESP8266DMX.getSlot(i + dmxOffset);

    // the first 3 slots are reserved for eye color, the rest are mapped onto
    // the servo motors (up to 16, but really the last 3 are eye colors)
    switch (i) {
    case 0:
      servoController->setEyeRed(sval);
      break;
    case 1:
      servoController->setEyeGreen(sval);
      break;
    case 2:
      servoController->setEyeBlue(sval);
      break;
    default:
      if (servoController->isEnabled(i - 3)) {
        // force 0 to the middle.
        // this is because the default state tends to be all 0s
        // which causes the head to fling itself to the extreme limits
        // before flinging back once values start flowing
        // just need to be mindful when remapping values to set the range as
        // 1 - 255 instead of 0 - 255
        if (sval == 0) {
          sval = 127;
        }
        servoController->set(i - 3, sval);
      }
    }
  }
}

uint8_t DMXController::getDMXValue(uint16_t slot) {
  if (slot >= 512) {
    return 0;
  }
  return ESP8266DMX.getSlot(slot);
}

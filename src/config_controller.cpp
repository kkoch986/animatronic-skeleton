#include <Arduino.h>
#include <EEPROM.h>
#include "./config_controller.h"

void ConfigController::init(Servo *s[SERVO_COUNT], byte ep[3]) {
    servos = s;
    for (int i = 0 ; i < SERVO_COUNT ; i++) {
        sweep[i] = false;
        sweepPositions[i] = 0;
        lastUpdateTimes[i] = 0;
    }

    eyePins = ep;

    // play some kind of animation to indicate programming mode
    for(int i = 0 ; i < 10 ; i++) {
      digitalWrite(eyePins[0], LOW);
      digitalWrite(eyePins[1], LOW);
      digitalWrite(eyePins[2], LOW);
      digitalWrite(17, LOW);
      delay(100);
      digitalWrite(eyePins[0], HIGH);
      digitalWrite(eyePins[1], HIGH);
      digitalWrite(eyePins[2], HIGH);
      digitalWrite(17, HIGH);
      delay(100);
    }
}

void ConfigController::initEEPROMToDefault() {
    byte eeprom[1 + (SERVO_COUNT * 6)] = {
          0, // Initialized
        // JAW (min, center, max, smooth, mtype, enabled)
          1, 180, 255, 100, MTYPE_SG90 , 255,
        // PAN
         90, 180, 255,  32, MTYPE_MG995, 255,
        // TILT
         40,  75, 127,  32, MTYPE_MG995, 255,
        // NOD
          1,  90, 127,  32, MTYPE_MG995, 255,
        // EYEH
         60,  90, 127,  32, MTYPE_SG90 , 255,
        // EYEV
        120, 160, 180,  32, MTYPE_SG90 , 255,
        // EXP6
          1, 127, 255,  32, MTYPE_SG90 ,   0,
        // EXP7
          1, 127, 255,  32, MTYPE_SG90 ,   0,
        // EXP8
          1, 127, 255,  32, MTYPE_SG90 ,   0,
        // EXP9
          1, 127, 255,  32, MTYPE_SG90 ,   0,
        // EXP10
          1, 127, 255,  32, MTYPE_SG90 ,   0,
        // EXP11
          1, 127, 255,  32, MTYPE_SG90 ,   0,
        // EXP12
          1, 127, 255,  32, MTYPE_SG90 ,   0,
        // EXP13
          1, 127, 255,  32, MTYPE_SG90 ,   0,
        // EXP14
          1, 127, 255,  32, MTYPE_SG90 ,   0,
        // EXP15
          1, 127, 255,  32, MTYPE_SG90 ,   0
    };
    EEPROM.put(0, eeprom);
}

void ConfigController::loadConfigFromEEPROM() {
    int idx = 0;
    for (int i = 1 ; i <= SERVO_COUNT * 6 ; i += 6) {
        servos[idx]->setConfig(
            EEPROM.read(i + CONFIG_ADDR_MIN_OFFSET),
            EEPROM.read(i + CONFIG_ADDR_CENTER_OFFSET),
            EEPROM.read(i + CONFIG_ADDR_MAX_OFFSET),
            EEPROM.read(i + CONFIG_ADDR_SMOOTH_OFFSET) / 255.0 / 10.0,
            EEPROM.read(i + CONFIG_ADDR_MTYPE_OFFSET),
            EEPROM.read(i + CONFIG_ADDR_ENABLED_OFFSET)
        );
        idx++;
    }
}

void ConfigController::initFromEEPROM() {
  // if eeprom[0] === 255 its never been written to
  if (EEPROM.read(CONFIG_ADDR_INITIALIZED) == 255) {
    Serial.println("no config detected, intializing");
    initEEPROMToDefault();
  }
  loadConfigFromEEPROM();
}

void ConfigController::writeConfigToEEPROM() {
    byte eeprom[1 + (SERVO_COUNT * 6)] = {
      0, // Initialized
    };
    int idx = 0;
    for (int i = 1 ; i <= SERVO_COUNT * 6 ; i += 6) {
        eeprom[i + CONFIG_ADDR_MIN_OFFSET] = servos[idx]->cfgmin();
        eeprom[i + CONFIG_ADDR_CENTER_OFFSET] = servos[idx]->cfgcenter();
        eeprom[i + CONFIG_ADDR_MAX_OFFSET] = servos[idx]->cfgmax();
        eeprom[i + CONFIG_ADDR_SMOOTH_OFFSET] = servos[idx]->cfgsmoothing() * 10.0 * 255;
        eeprom[i + CONFIG_ADDR_MTYPE_OFFSET] = servos[idx]->cfgmtype();
        eeprom[i + CONFIG_ADDR_ENABLED_OFFSET] = servos[idx]->cfgenabled();
        idx++;
    }
    EEPROM.put(0, eeprom);
}

void ConfigController::loop() {
  if (DMXSerial.dataUpdated()) {
    DMXSerial.resetUpdated();

    // if the write channel is set to 127, reset everything to default
    if(DMXSerial.read(REPR_CHANNEL_WRITE_MODE) == 127) {
      Serial.println("reverting to defaults");
      // flash the lights for some feedback
      for(int i = 0 ; i < 4 ; i++) {
        digitalWrite(17, HIGH);
        digitalWrite(eyePins[2], LOW);
        delay(100);
        digitalWrite(eyePins[0], HIGH);
        digitalWrite(eyePins[1], HIGH);
        digitalWrite(eyePins[2], HIGH);
        digitalWrite(17, LOW);
        delay(100);
      }

      initEEPROMToDefault();
      loadConfigFromEEPROM();
      writeConfigToEEPROM();
      return;
    }

    // flicker the lights to indicate we recieved new data
    for(int i = 0 ; i <5 ; i++) {
      digitalWrite(17, HIGH);
      digitalWrite(eyePins[1], LOW);
      delay(100);
      digitalWrite(eyePins[0], HIGH);
      digitalWrite(eyePins[1], HIGH);
      digitalWrite(eyePins[2], HIGH);
      digitalWrite(17, LOW);
      delay(100);
    }

    // process the data sent
    // we will loop over 1 -> (7 * SERVO_COUNT) + 1
    // skipping 0 since thats the write mode flag
    int idx = 0;
    for (int i = 1 ; i < (7 * SERVO_COUNT) + 1 ; i += 7) {
        Serial.print(1 + i + REPR_SWEEP_OFFSET);
        Serial.print(" (sweep): ");
        Serial.println(DMXSerial.read(1 + i + REPR_SWEEP_OFFSET));
        if(DMXSerial.read(1 + i + REPR_SWEEP_OFFSET) == 255) {
            Serial.print("ENABLED SWEEP FOR ");
            Serial.println(idx);
            sweep[idx] = true;
        } else {
            Serial.print("DISABLED SWEEP FOR ");
            Serial.println(idx);
            sweep[idx] = false;
        }
        servos[idx]->setConfig(
            DMXSerial.read(1 + i + REPR_MIN_OFFSET),
            DMXSerial.read(1 + i + REPR_CENTER_OFFSET),
            DMXSerial.read(1 + i + REPR_MAX_OFFSET),
            DMXSerial.read(1 + i + REPR_SMOOTH_OFFSET) / 255.0 / 10.0,
            DMXSerial.read(1 + i + REPR_MTYPE_OFFSET),
            DMXSerial.read(1 + i + REPR_EN_OFFSET)
        );
        idx++;
    }

    // if the write channel is 255, write the new data to eeprom
    if(DMXSerial.read(REPR_CHANNEL_WRITE_MODE) == 255) {
      Serial.println("writing to EEPROM");
      writeConfigToEEPROM();
      for(int i = 0 ; i < 10 ; i++) {
        digitalWrite(17, HIGH);
        digitalWrite(eyePins[0], LOW);
        delay(100);
        digitalWrite(eyePins[0], HIGH);
        digitalWrite(eyePins[1], HIGH);
        digitalWrite(eyePins[2], HIGH);
        digitalWrite(17, LOW);
        delay(100);
      }
    }
  }

  // Update anything that is sweeping
  // TODO: when sweeping, if the smoothing is too low, it can end up slower than the 
  //       SWEEP_RATE_MS and you wont see the full extension in both directions
  unsigned long loopStart = millis();
  for (int i = 0 ; i < SERVO_COUNT ; i++) {
      if (sweep[i]) {
          if (loopStart - lastUpdateTimes[i] >= SWEEP_RATE_MS) {
              if (sweepPositions[i] == 255) {
                  sweepPositions[i] = 1;
              } else if(sweepPositions[i] == 1) {
                  sweepPositions[i] = 127;
              } else {
                  sweepPositions[i] = 255;
              }
              lastUpdateTimes[i] = loopStart;
          }
          servos[i]->setPosition(sweepPositions[i]);
      } else {
          servos[i]->setPosition(servos[i]->cfgcenter());
      }
      servos[i]->loop();
  }
}

bool ConfigController::waitForConfigMode() {
  byte tries = 0;
  while(!DMXSerial.dataUpdated()) {
    if(tries++ == 10) {
      return false;
    }
    delay(1000);
  }

  // if the DMX values are 1 - 255, 1 - 255
  // (1, 2, 3, 4, ... , 255, 1, 2, 3, ..., 255, 1, 2)
  // also enter setup mode
  for (uint16_t i = 0; i < 512 ; i++) {
    byte actual = DMXSerial.read(i + 1);
    byte exp = (i % 255) + 1;
    if (actual != exp) {
      return false;
    }
    DMXSerial.resetUpdated();
  }

  return true;
}



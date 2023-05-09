#pragma once

#define SERVO_COUNT 16

#define MTYPE_SG90    0
#define MTYPE_MG90S   1
#define MTYPE_MG995   2
#define MTYPE_HS805BB 3

#include <Arduino.h>
#include <Adafruit_PWMServoDriver.h>

class Servo {
    private:
      Adafruit_PWMServoDriver controller;
      bool enabled;
      unsigned int pulseMin, pulseMax, index;
      byte mtype, limitAngleLow, limitAngleHigh, initialValue;
      float targetPos;
      float currentPos, smoothingFactor;
      unsigned int pulseMins[4] = {
        135, // SG90
        100, // MG90S
        100, // MG995
        100   // HS805BB
      };
      unsigned int pulseMaxes[4] = {
        500, // SG90
        500, // MG90S
        500, // MG995
        500  // HS805BB
      };
    public:
      Servo() {};
      Servo(
          Adafruit_PWMServoDriver controller,
          byte mtype,
          unsigned int index,
          byte limitAngleLow,
          byte limitAngleHigh,
          byte initialValue,
          float smoothingFactor,
          bool enabled
      ) {
        this->enabled         = enabled;
        this->mtype           = mtype;
        this->pulseMin        = pulseMins[mtype];
        this->pulseMax        = pulseMaxes[mtype];
        this->index           = index;
        this->limitAngleHigh  = limitAngleHigh;
        this->limitAngleLow   = limitAngleLow;
        this->initialValue    = initialValue;
        this->controller      = controller;
        this->smoothingFactor = smoothingFactor;
      }
      
      byte cfgmin() {
        return limitAngleLow;
      }
      byte cfgmax() {
        return limitAngleHigh;
      }
      byte cfgcenter() {
        return initialValue;
      }
      float cfgsmoothing() {
        return smoothingFactor;
      }
      bool cfgenabled() {
        return enabled;
      }
      byte cfgmtype() {
        return mtype;
      }

      void setConfig(
          byte min,
          byte center,
          byte max,
          float sf,
          byte mt,
          bool en
      ) {
        Serial.print(index);
        Serial.print(" SET CONFIG -- MIN: ");
        Serial.print(min);
        Serial.print("  CENTER: ");
        Serial.print(center);
        Serial.print("  MAX: ");
        Serial.print(max);
        Serial.print("  SMOOTH: ");
        Serial.print(sf);
        Serial.print("  ENABLED: ");
        Serial.print(en);
        Serial.print("  MTYPE: ");
        Serial.println(mt);
        limitAngleLow = min;
        limitAngleHigh = max;
        initialValue = center;
        smoothingFactor = sf;
        enabled = en;
        mtype = mt;
        pulseMin = pulseMins[mtype];
        pulseMax = pulseMaxes[mtype];
      }

      void init(unsigned int i) {
        index = i;
        currentPos = initialValue;
        targetPos = initialValue;
        controller.setPWM(index, 0, getPulseForAngle(currentPos));
      }

      void loop() {
        if (enabled) {
          if (round(currentPos) != round(targetPos)) {
            currentPos = (targetPos * smoothingFactor) + (currentPos * (1 - smoothingFactor));
            if(abs(targetPos - currentPos) < 2) {
              currentPos = targetPos;
            }
          }
          controller.setPWM(index, 0, getPulseForAngle(currentPos));
        } else {
          controller.setPWM(index, 0, 0);
        }
      }

      float getPulseForAngle(byte v) {
        // convert the byte to degree scale, then map to pulse lengths
        return map(map(v, 0, 255, limitAngleLow, limitAngleHigh), 0, 255, pulseMin, pulseMax);
      }
      void setPosition(byte v) {
        // If we get a hard 0 for a target, go to the middle
        //  this is since xlights loves writing everything to 0 all the time
        //  and i dont want to have to codify them all constantly to a neutral state
        if (v == 0) {
          v = initialValue;
        }
        targetPos = v;
      }
};




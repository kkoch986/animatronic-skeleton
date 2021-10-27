#include "./TriaxialSkeleton.h"
#include "./ControlModes.h"
#include "./fmap.h"

TriaxialSkeleton::TriaxialSkeleton(Adafruit_PWMServoDriver p, unsigned int min,
                                   unsigned int max, unsigned int pi,
                                   unsigned int ti, unsigned int yi,
                                   unsigned int ji, unsigned int ymin,
                                   unsigned int ymax, unsigned int jmin,
                                   unsigned int jmax, unsigned int pmin,
                                   unsigned int pmax, unsigned int tmin,
                                   unsigned int tmax) {
  pwm = p;
  servoMin = min;
  servoMax = max;
  panServoIndex = pi;
  yawServoIndex = yi;
  tiltServoIndex = ti;
  jawServoIndex = ji;
  jawMaxPct = jmax;
  jawMinPct = jmin;
  panMinPct = pmin;
  panMaxPct = pmax;
  yawMaxPct = ymax;
  yawMinPct = ymin;
  tiltMinPct = tmin;
  tiltMaxPct = tmax;
  lastLoop = millis();
}

void TriaxialSkeleton::setPan(unsigned int pct) {
  if (pct == 0) {
    pct = 128;
  }
  pwm.setPWM(
      panServoIndex, 0,
      map(fmap(pct, 0, 255, panMinPct, panMaxPct), 0, 100, servoMin, servoMax));
}

void TriaxialSkeleton::setTilt(unsigned int pct) {
  if (pct == 0) {
    pct = 128;
  }
  pwm.setPWM(tiltServoIndex, 0,
             map(fmap(pct, 0, 255, tiltMinPct, tiltMaxPct), 0, 100, servoMin,
                 servoMax));
}

void TriaxialSkeleton::setYaw(unsigned int pct) {
  if (pct == 0) {
    pct = 128;
  }
  pwm.setPWM(
      yawServoIndex, 0,
      map(fmap(pct, 0, 255, yawMinPct, yawMaxPct), 0, 100, servoMin, servoMax));
}

void TriaxialSkeleton::setJaw(unsigned int pct) {
  pwm.setPWM(
      jawServoIndex, 0,
      map(fmap(pct, 0, 255, jawMinPct, jawMaxPct), 100, 0, servoMin, servoMax));
}

void TriaxialSkeleton::loop() {
  // If we are in the idle control mode,
  //  TODO: ....
  if (controlMode == CONTROL_MODE_IDLE) {
  }
}

void TriaxialSkeleton::updateControlMode(unsigned int cm, unsigned int i1,
                                         unsigned int i2, unsigned int i3,
                                         unsigned int i4) {
  controlMode = cm;
  input1 = i1;
  input2 = i2;
  input3 = i3;
  input4 = i4;

  // if its DMX control mode, just move the servos
  if (controlMode == CONTROL_MODE_DMX) {
    setPan(input1);
    setTilt(input2);
    setYaw(input3);
    setJaw(input4);
  }
}

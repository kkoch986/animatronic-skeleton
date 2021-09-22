#include "./Skeleton.h"
#include "./ControlModes.h"

Skeleton::Skeleton(
        Adafruit_PWMServoDriver p,
        unsigned int min, unsigned int max, 
        unsigned int pi,
        unsigned int ti,
        unsigned int ji,
        unsigned int jmin, unsigned int jmax,
        unsigned int pmin, unsigned int pmax,
        unsigned int tmin, unsigned int tmax
) {
    pwm = p;
    servoMin = min;
    servoMax = max;
    panServoIndex = pi;
    tiltServoIndex = ti;
    jawServoIndex = ji;
    jawMaxPct = jmax;
    jawMinPct = jmin;
    panMinPct = pmin;
    panMaxPct = pmax;
    tiltMinPct = tmin;
    tiltMaxPct = tmax;
    lastLoop = millis();
}

float fmap(float x, float in_min, float in_max, float out_min, float out_max) {
    return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

void Skeleton::setPan(unsigned int pct) {
    pwm.setPWM(panServoIndex, 0, map(
        fmap(pct, 0, 255, panMinPct, panMaxPct), 
        0, 100, 
        servoMin, servoMax
    ));
}

void Skeleton::setTilt(unsigned int pct) {
    pwm.setPWM(tiltServoIndex, 0, map(
        fmap(pct, 0, 255, tiltMinPct, tiltMaxPct), 
        0, 100, 
        servoMin, servoMax
    ));
}

void Skeleton::setJaw(unsigned int pct) {
    pwm.setPWM(jawServoIndex, 0, map(
        fmap(pct, 0, 255, jawMinPct, jawMaxPct), 
        0, 100, 
        servoMin, servoMax
    ));
}



void Skeleton::loop() {
    // If we are in the idle control mode,
    //  TODO: .... 
    if(controlMode == CONTROL_MODE_IDLE) {
        
    }
}



void Skeleton::updateControlMode(
    unsigned int cm,
    unsigned int i1,
    unsigned int i2,
    unsigned int i3
) {
    controlMode = cm;
    input1 = i1;
    input2 = i2;
    input3 = i3;
    
    // if its DMX control mode, just move the servos
    if (controlMode == CONTROL_MODE_DMX) {
        setPan(input1);
        setTilt(input2);
        setJaw(input3);
    }
}

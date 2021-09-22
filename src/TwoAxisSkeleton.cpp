#include "./TwoAxisSkeleton.h"
#include "./ControlModes.h"

TwoAxisSkeleton::TwoAxisSkeleton(
		Adafruit_PWMServoDriver p,
		unsigned int _lServo,
		unsigned int _lMin,
		unsigned int _lMid,
		unsigned int _lMax,
		unsigned int _rServo,
		unsigned int _rMin,
		unsigned int _rMid,
		unsigned int _rMax,
		unsigned int _jawServo,
		unsigned int _jawMin,
		unsigned int _jawMax
) {
	pwm = p;
	lServo = _lServo;
	lMin = _lMin;
	lMax = _lMax;
	lMid = _lMid;
	rServo = _rServo;
	rMin = _rMin;
	rMax = _rMax;
	rMid = _rMid;
	jawServo = _jawServo;
	jawMin = _jawMin;
	jawMax = _jawMax;
}

void TwoAxisSkeleton::lookStraight() {
	pwm.setPWM(lServo, 0, lMid);
	pwm.setPWM(rServo, 0, rMid);
}

void TwoAxisSkeleton::lookUp(unsigned int pct) {
	pwm.setPWM(lServo, 0, map(pct, 100, 0, lMin, lMid));
	pwm.setPWM(rServo, 0, map(pct, 100, 0, rMin, rMid));
}

void TwoAxisSkeleton::lookDown(unsigned int pct) {
	pwm.setPWM(lServo, 0, map(pct, 100, 0, lMax, lMid));
	pwm.setPWM(rServo, 0, map(pct, 100, 0, rMax, rMid));
}

void TwoAxisSkeleton::updateControlMode(
	unsigned int cm,
	unsigned int i1,
	unsigned int i2,
	unsigned int i3
) {
	controlMode = cm;
	input1 = i1;
	input2 = i2;
	input3 = i3;

	// if the control mode is DMX, just set the servos directly
	if (controlMode == CONTROL_MODE_DMX) {
		// map the 0 - 255 inputs to the min - max range
		pwm.setPWM(lServo, 0, map(input1, 0, 255, lMin, lMax));
		pwm.setPWM(rServo, 0, map(input2, 0, 255, rMin, rMax));
		pwm.setPWM(jawServo, 0, map(input3, 0, 255, jawMin, jawMax));
	}
}

void TwoAxisSkeleton::loop() {

}

#ifndef _2AXISSKEL
#define _2AXISSKEL

#include <Adafruit_PWMServoDriver.h>

class TwoAxisSkeleton {
	private:
		Adafruit_PWMServoDriver pwm;
		unsigned int lServo, lMin, lMax, lMid, rServo, rMin, rMax, rMid;
		unsigned int jawServo, jawMin, jawMax;
		unsigned int controlMode, input1, input2, input3;
	public:
		TwoAxisSkeleton(
			Adafruit_PWMServoDriver pwm,
			unsigned int lServo,
			unsigned int lMin,
			unsigned int lMid,
			unsigned int lMax,
			unsigned int rServo,
			unsigned int rMin,
			unsigned int rMid,
			unsigned int rMax,
			unsigned int jawServo,
			unsigned int jawMin,
			unsigned int jawMax
		);
		void updateControlMode(
			unsigned int cm,
			unsigned int input1,
			unsigned int input2,
			unsigned int input3
		);
		void lookStraight();
		void lookUp(unsigned int pct);
		void lookDown(unsigned int pct);
		void loop();
};

#endif

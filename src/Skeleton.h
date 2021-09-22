#ifndef _SKELETON
#define _SKELETON

#include <Adafruit_PWMServoDriver.h>

class Skeleton {
    private:
        Adafruit_PWMServoDriver pwm;
        unsigned int servoMin, servoMax, panServoIndex, tiltServoIndex, jawServoIndex, jawMinPct, jawMaxPct;
        unsigned int panMinPct, panMaxPct, tiltMinPct, tiltMaxPct;
        unsigned long lastLoop;
        unsigned int controlMode, input1, input2, input3;
    public:
        Skeleton(
                Adafruit_PWMServoDriver pwm,
                unsigned int servoMin, unsigned int servoMax, 
                unsigned int panServoIndex,
                unsigned int tiltServoIndex,
                unsigned int jawServoIndex,
                unsigned int jawMin, unsigned int jawMax,
                unsigned int panMin, unsigned int panMax,
                unsigned int tiltMin, unsigned int tiltMax
        );
		void updateControlMode(
			unsigned int cm,
			unsigned int input1,
			unsigned int input2,
			unsigned int input3
		);
        void setPan(unsigned int percentage);
        void setTilt(unsigned int percentage);
        void setJaw(unsigned int percentage);
        void loop();
};

#endif

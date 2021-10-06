#ifndef _TRIAXIALSKELETON
#define _TRIAXIALSKELETON

#include <Adafruit_PWMServoDriver.h>

class TriaxialSkeleton {
    private:
        Adafruit_PWMServoDriver pwm;
        unsigned int servoMin, servoMax;
        unsigned int panServoIndex, tiltServoIndex, yawServoIndex, jawServoIndex;
        unsigned int jawMinPct, jawMaxPct;
        unsigned int panMinPct, panMaxPct;
        unsigned int yawMinPct, yawMaxPct;
        unsigned int tiltMinPct, tiltMaxPct;
        unsigned long lastLoop;
        unsigned int controlMode, input1, input2, input3, input4;
    public:
        TriaxialSkeleton(
                Adafruit_PWMServoDriver pwm,
                unsigned int servoMin, unsigned int servoMax, 
                unsigned int panServoIndex,
                unsigned int tiltServoIndex,
                unsigned int yawServoIndex,
                unsigned int jawServoIndex,
                unsigned int yawMin, unsigned int yawMax,
                unsigned int jawMin, unsigned int jawMax,
                unsigned int panMin, unsigned int panMax,
                unsigned int tiltMin, unsigned int tiltMax
        );
		void updateControlMode(
			unsigned int cm,
			unsigned int input1,
			unsigned int input2,
			unsigned int input3,
            unsigned int input4
		);
        void setPan(unsigned int percentage);
        void setTilt(unsigned int percentage);
        void setYaw(unsigned int percentage);
        void setJaw(unsigned int percentage);
        void loop();
};

#endif

#pragma once

#include "./servos.h"

class SkullDMXController {
  private:
    unsigned int startChannel;
    Servo **servos;
    unsigned int eyeR, eyeG, eyeB;
  public:
    void init(
        unsigned int startChannel,
        Servo *servos[SERVO_COUNT],
        unsigned int eyeR,
        unsigned int eyeG,
        unsigned int eyeB
    );
    void loop();
};


#pragma once

#include <DMXSerial.h>
#include "./servos.h"

// | ChannelType  | Description                                         |
// | ------------ | --------------------------------------------------- |
// | WRITE_MODE   | write to eeprom (only if 255)                       |
// | <IDX>_EN     | if 255, the motor should be enabled and used        |
// | <IDX>_SWEEP  | Enable motor sweep (255 - on (everything else off)) |
// | <IDX>_MIN    | Set motor MIN (0 - 255)                             |
// | <IDX>_CENTER | Set motor CENTER (0 - 255)                          |
// | <IDX>_MAX    | Set motor MAX (0 - 255)                             |
// | <IDX>_SMOOTH | Set motor SMOOTHING (0 - 255 maps from 0 - 2)       |
// | <IDX>_MTYPE  | Set motor type for <IDX>                            |
#define REPR_CHANNEL_WRITE_MODE   1
#define REPR_SWEEP_OFFSET  0
#define REPR_MIN_OFFSET    1
#define REPR_CENTER_OFFSET 2
#define REPR_MAX_OFFSET    3
#define REPR_SMOOTH_OFFSET 4
#define REPR_MTYPE_OFFSET  5
#define REPR_EN_OFFSET     6

#define CONFIG_ADDR_INITIALIZED    0
#define CONFIG_ADDR_MIN_OFFSET     0
#define CONFIG_ADDR_CENTER_OFFSET  1
#define CONFIG_ADDR_MAX_OFFSET     2
#define CONFIG_ADDR_SMOOTH_OFFSET  3
#define CONFIG_ADDR_MTYPE_OFFSET   4
#define CONFIG_ADDR_ENABLED_OFFSET 5

#define SWEEP_RATE_MS 5000

class ConfigController {
    public:
        ConfigController(){};
        void init(Servo *s[SERVO_COUNT], byte ep[3]);
        bool waitForConfigMode();
        void initFromEEPROM();
        void loop();
    private:
        byte *eyePins;
        Servo **servos;
        bool sweep[SERVO_COUNT];
        byte sweepPositions[SERVO_COUNT];
        unsigned long lastUpdateTimes[SERVO_COUNT];

        /**
         * initEEPROMToDefault initializes the eeprom
         * to a sane default configuration.
         * This should be called if no existing configuration
         *   is detected.
         */
        void initEEPROMToDefault();

        /**
         * loadConfigFromEEPROM will read the config from 
         * EEPROM and configure the various motors.
         * It does not check if EEPROM has been initialized
         */
        void loadConfigFromEEPROM();
        void writeConfigToEEPROM();
};



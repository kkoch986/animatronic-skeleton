#pragma once

// Networking configuration
#define OTA_PORT 8266

// Status LED Pins
#define STATUS_GREEN 16
#define STATUS_BLUE 13
#define STATUS_RED 12

// The pins to read the control address from
// this is purely an application level address
// not to be confused with the I2C address for controlling
// the PWM chip
#define ADDR_0 10
#define ADDR_1 9
#define ADDR_2 5
#define ADDR_3 4

#define RESET_BUTTON 0

// The total number of available servos
#define SERVO_COUNT 16
#define SERVO_MAX_LABEL_SIZE 8

// ENABLED, MIN, MAX, IDLE, SMOOTH, CURRENT, TARGET, ARRIVED, LABELSIZE, LABEL
// (MAX_LABEL_SIZE bytes)
#define SERVO_DUMP_BYTES_PER_SERVO (9 + SERVO_MAX_LABEL_SIZE)

// This will get used as the hostname for the device so it needs to
// conform to the valid character set of a DNS name
// also its loaded into a 32 byte buffer so keep it below 29 characters
// to allow room for up to 2 digits of address
#define CONTROL_ADDR_FMT "skeleton-%d"

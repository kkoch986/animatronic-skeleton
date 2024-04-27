#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <WiFiManager.h>
#include <ArduinoOTA.h>
#include "PCA9685.h"
#include <WebSocketsClient.h>

// Networking configuration
#define OTA_PORT 8266

// Status LED Pins
#define STATUS_GREEN 16
#define STATUS_BLUE  13
#define STATUS_RED   12

// I2C Configuration for the PCA9685
#define PCA9685_I2C_ADDR B1010101
#define PCA9685_I2C_SDA  2
#define PCA9685_I2C_SCL  14

// The pins to read the control address from
// this is purely an application level address
// not to be confused with the I2C address for controlling
// the PWM chip
#define ADDR_0 10
#define ADDR_1  9
#define ADDR_2  5
#define ADDR_3  4

#define SERVO_COUNT 16

// Define some standard indexes for common usages
// got the numbering kinda weird on the board silkscreen so the S*
// constants are just to ease matching what you see on the board
// with the socket messages
// LATER: if we get rid of the text message format we can probably remove all of these constants
#define JAW_SERVO   0
#define PAN_SERVO   1
#define TILT_SERVO  2
#define NOD_SERVO   3
#define EYE_H_SERVO 4
#define EYE_V_SERVO 5
#define EYE_R      15
#define EYE_G      14
#define EYE_B      13
#define S1          1
#define S2          2
#define S3          3
#define S4          4
#define S5          5
#define S6          6
#define S7          7
#define S8          0
#define S9          9
#define S10        10
#define S11        11
#define S12        12
#define S13         8

// This will get used as the hostname for the device so it needs to 
// conform to the valid character set of a DNS name
// also its loaded into a 32 byte buffer so keep it below 29 characters
// to allow room for up to 2 digits of address
#define CONTROL_ADDR_FMT "skeleton-%d"

// The various commands that we can handle over the websocket format
enum CommandType {
  // MOVE is used to set the position of a particular servo (or light)
  // when MOVE is used, the value will be travelled to via the smoothing parameters
  // and not move immediately to the desired position.
  // This should be the preferred way to adjust motor position during performances
  // since it prevents high-speed, jerky motion from happening
  // message format is as follows:
  // <address> MOVE <servo-identifier> <value>
  // value should be a number from 0 - 255, with 127 representing the center position
  // servo-identifier can be either a number from [0 - SERVO_COUNT-1) or one of these
  // predefined constants:
  // jaw, pan, tilt, nod, eyeh, eyev
  MOVE,
  // SET works like move, but will bypass any software based smoothing
  SET,
  // UNKNOWN is used when we cannot understand the message sent
  UNKNOWN
};

// parse command will attempt to match the given character string
// to a known command type.
CommandType parseCommandType(char *cmd) {
  if (strcmp(cmd, "MOVE") == 0) {
    return MOVE;
  } else if (strcmp(cmd, "SET") == 0) {
    return SET;
  }
  return UNKNOWN;
}

unsigned short parseServoIdentifier(char *servo) {
  if (strcmp(servo, "JAW") == 0) {
    return JAW_SERVO;
  } else if (strcmp(servo, "PAN") == 0) {
    return PAN_SERVO;
  } else if (strcmp(servo, "TILT") == 0) {
    return TILT_SERVO;
  } else if (strcmp(servo, "NOD") == 0) {
    return NOD_SERVO;
  } else if (strcmp(servo, "EYEH") == 0) {
    return EYE_H_SERVO;
  } else if (strcmp(servo, "EYEV") == 0) {
    return EYE_V_SERVO;
  } else if (strcmp(servo, "S1") == 0) {
    return S1;
  } else if (strcmp(servo, "S2") == 0) {
    return S2;
  } else if (strcmp(servo, "S3") == 0) {
    return S3;
  } else if (strcmp(servo, "S4") == 0) {
    return S4;
  } else if (strcmp(servo, "S5") == 0) {
    return S5;
  } else if (strcmp(servo, "S6") == 0) {
    return S6;
  } else if (strcmp(servo, "S7") == 0) {
    return S7;
  } else if (strcmp(servo, "S8") == 0) {
    return S8;
  } else if (strcmp(servo, "S9") == 0) {
    return S9;
  } else if (strcmp(servo, "S10") == 0) {
    return S10;
  } else if (strcmp(servo, "S11") == 0) {
    return S11;
  } else if (strcmp(servo, "S12") == 0) {
    return S12;
  } else if (strcmp(servo, "S13") == 0) {
    return S13;
  }
  return atoi(servo);
}

PCA9685 pwmController(PCA9685_I2C_ADDR);
PCA9685_ServoEval pwmServos[SERVO_COUNT];

// websocket server configuration
WiFiManager wifiManager;
WebSocketsClient webSocket;

// LATER: define a protocol for indicating back to the server the current actual state
// might need a flag or something to enable/disable this behavior if performance is too low

// TODO: accept this as part of the wifimanager set up. also need a way to reset it somehow
char webSocketHost[40] = "192.168.1.171";
int webSocketPort = 8888;
char controlAddr[32];
bool websocketConnected = false;
void webSocketEvent(WStype_t type, uint8_t * payload, size_t length) {
#ifdef ENABLE_DEBUG
  Serial.printf("websocket event %d\n", type);
#endif
  switch(type) {
    // ignore these events
    case WStype_FRAGMENT_TEXT_START:
    case WStype_FRAGMENT_BIN_START:
    case WStype_FRAGMENT:
    case WStype_FRAGMENT_FIN:
    case WStype_PING:
    case WStype_PONG:
      return ;

    // handle these events
    case WStype_ERROR:
      // LATER: error handling
#ifdef ENABLE_DEBUG
       Serial.printf("[WSc] error: %s\n", payload);
#endif
       break;
    case WStype_DISCONNECTED:
       // LATER: error handling
      websocketConnected = false;
      break;
    case WStype_CONNECTED:
      websocketConnected = true;
      break ;
    case WStype_BIN:
      // LATER: replace the text protocol with a binary one
      break ;
    case WStype_TEXT:
      // Text commands are expected in the format
      // <address> <cmd> [<argument-values> ...]
      // for example
      // "/skeleton1 MOVE jaw 255"
     char *addr  = strtok((char *)payload, " ");
     int match   = strcmp(addr, controlAddr);
     
     // if the address matches, parse the command
     if (match == 0) {
       CommandType cmd = parseCommandType(strtok(NULL, " "));
#ifdef ENABLE_DEBUG
       Serial.printf("command: %d\n", cmd);
#endif
       switch(cmd) {
         case MOVE: 
           // TODO: implement smoothing here instead of falling through to SET
         case SET: {
            // the next token should be a servo index
            // since we also support some predefined constants, convert it 
            // to an index via a helper
            unsigned short index = parseServoIdentifier(strtok(NULL, " "));
            if (index != -1) {
              // the last token should be the value to move to
              char *value = strtok(NULL, " ");
              int val = atoi(value);
              #ifdef ENABLE_DEBUG
                Serial.printf("set channel %d PWM to: %d\n", index, map(val, 0, 255, -90, 90));
              #endif
              // TODO: convert this to use a servo controller that will implement smoothing
              pwmController.setChannelPWM(index, pwmServos[index].pwmForAngle(
                map(val, 0, 255, -90, 90)
              ));
            }
           break ;
         }
         case UNKNOWN: {}
       }
     }
     break ;
  }
}

void resetWifi() {
  wifiManager.erase();
  wifiManager.resetSettings();
  ESP.restart();
}

// write the colors to the status LED
void indicatorColor(byte red, byte green, byte blue) {
  analogWrite(STATUS_RED, red);
  analogWrite(STATUS_GREEN, green);
  analogWrite(STATUS_BLUE, blue);
}

bool wifiSetup(char *hostname) {
 bool res;
 WiFi.setHostname(hostname);
 res = wifiManager.autoConnect(hostname);

#ifdef ENABLE_DEBUG
 if(!res) {
   Serial.println("Failed to connect");
 } 
 else {
   //if you get here you have connected to the WiFi    
   Serial.println("connected to wifi");
 }
#endif
 return res;
}

void otaSetup(char *hostname) {
 // Prepare ArduinoOTA
 // No authentication by default
 // ArduinoOTA.setPassword("admin");
 // Password can be set with it's md5 value as well
 // MD5(admin) = 21232f297a57a5a743894a0e4a801fc3
 // ArduinoOTA.setPasswordHash("21232f297a57a5a743894a0e4a801fc3");
  ArduinoOTA.setPort(OTA_PORT);
  ArduinoOTA.setHostname(hostname);

  ArduinoOTA.onStart([]() {
#ifdef ENABLE_DEBUG
    String type;
    if (ArduinoOTA.getCommand() == U_FLASH) {
      type = "sketch";
    } else {  // U_FS
      type = "filesystem";
    }
    Serial.println("Start updating " + type);
#endif
    indicatorColor(255, 255, 255);
  });

  ArduinoOTA.onEnd([]() {
    indicatorColor(0,0,0);
#ifdef ENABLE_DEBUG
    Serial.println("\nEnd");
#endif
  });

  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
      // Fade the LED out as progress is completed
      unsigned int pct = (progress / (total / 100));
      byte color = 255 - (pct * 255) / 100;
      indicatorColor(color, color, color);
#ifdef ENABLE_DEBUG
      Serial.printf("Progress: %u%%\r", pct);
#endif
  });

  ArduinoOTA.onError([](ota_error_t error) {
#ifdef ENABLE_DEBUG
    Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) {
      Serial.println("Auth Failed");
    } else if (error == OTA_BEGIN_ERROR) {
      Serial.println("Begin Failed");
    } else if (error == OTA_CONNECT_ERROR) {
      Serial.println("Connect Failed");
    } else if (error == OTA_RECEIVE_ERROR) {
      Serial.println("Receive Failed");
    } else if (error == OTA_END_ERROR) {
      Serial.println("End Failed");
    }
#endif
    // flash the status LED to indicate an error
    for (int i = 0; i < 3; i++) {
      indicatorColor(255, 0, 0);
      delay(300);
      indicatorColor(0, 0, 0);
      delay(300);
    }
  });

  ArduinoOTA.begin();
}

bool pwmSetup() {
  byte err;
  Wire.begin(PCA9685_I2C_SDA, PCA9685_I2C_SCL);
  pwmController.resetDevices();
  pwmController.init();
#ifdef PCA9685_ENABLE_DEBUG_OUTPUT
  pwmController.printModuleInfo();
#endif
  pwmController.setPWMFreqServo();
  err = pwmController.getLastI2CError();
  return err == 0;
}

// Read from the address jumpers to determine the address of this
// board. the address is used for the hostname as well as parsing
// incoming control messages
byte readAddress() {
  byte p1 = digitalRead(ADDR_0) == HIGH ? 0b0001 : 0b0000;
  byte p2 = digitalRead(ADDR_1) == HIGH ? 0b0010 : 0b0000;
  byte p3 = digitalRead(ADDR_2) == HIGH ? 0b0100 : 0b0000;
  byte p4 = digitalRead(ADDR_3) == HIGH ? 0b1000 : 0b0000;
  return p1 + p2 + p3 + p4;
}

void setup() {
 Serial.begin(115200);
 Serial.printf("WEBSOCKETS_NETWORK_TYPE %d\n", WEBSOCKETS_NETWORK_TYPE);

 // configure pins
 pinMode(STATUS_GREEN, OUTPUT);
 pinMode(STATUS_BLUE,  OUTPUT);
 pinMode(STATUS_RED,   OUTPUT);
 pinMode(ADDR_0,       INPUT);
 pinMode(ADDR_1,       INPUT);
 pinMode(ADDR_2,       INPUT);
 pinMode(ADDR_3,       INPUT);

 indicatorColor(0, 0, 0);

 // read in the address
 byte address = readAddress();
 sprintf(controlAddr, CONTROL_ADDR_FMT, address);

#ifdef ENABLE_DEBUG
 Serial.print("Control Address: ");
 Serial.println(controlAddr);
#endif

 // set up the major components
 // TODO: improve the status sequence to be more understandable
 indicatorColor(0, 0, 255);
 if (!wifiSetup(controlAddr)) {
   while(true) {
     indicatorColor(255, 0, 0);
     delay(300);
     indicatorColor(0, 0, 0);
     delay(300);
   }
 }
 otaSetup(controlAddr);
  if (!pwmSetup()) {
   while(true) {
     indicatorColor(255, 0, 0);
     delay(300);
     indicatorColor(0, 0, 0);
     delay(300);
   }
 }
 indicatorColor(0, 255, 0);

 // connect to the websocket server
  webSocket.onEvent(webSocketEvent);
  webSocket.setReconnectInterval(1000);
#ifdef ENABLE_DEBUG
  Serial.printf("connecting to %s:%d\n", webSocketHost, webSocketPort);
#endif
  webSocket.begin("192.168.1.171", 8888, "/");
}

void loop() 
{
  // OTA
  ArduinoOTA.handle();
  webSocket.loop();

  // State checking
  if (!websocketConnected) {
    indicatorColor(255, 0, 0);
  } else {
    indicatorColor(0, 255, 0);
  }
}

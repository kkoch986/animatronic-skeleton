#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <PCA9685.h>
#include <Preferences.h>
#include <WiFiManager.h>

#ifdef REMOTE_DEBUG
#include "ESPTelnet.h"
ESPTelnet telnet;
#endif

#include "config.h"
#include "connection_manager.h"
#include "controller_dmx.h"
#include "controller_websocket.h"
#include "servo_controller.h"
#include "status_led.h"

// State holds the various states of execution that we can be in
// Since everything is async, this allows us to have the loop operate
// correctly and trigger transistions when necessary.
enum State {
  STATE_UNKNOWN,
  STATE_WIFI_CONNECTING,
  STATE_POST_WIFI_CONNECT,
  STATE_CONNECTED,
};
// TODO: move this out of the global ns
State currentState = STATE_UNKNOWN;

#ifdef DMX_CTRL
DMXController *dmxController;
#endif
#ifdef WEBSOCKET_CTRL
WebSocketController webSocketController;
char webSocketHost[40];
#endif

ServoController servoController;
ConnectionManager connectionManager;

char controlAddr[32];
byte address = 0;

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

#ifdef REMOTE_DEBUG
#include "telnet.h"
#endif

void setup() {
#ifdef ENABLE_DEBUG
  Serial.begin(115200);
#endif
  // configure pins
  pinMode(RESET_BUTTON, INPUT_PULLUP);
  pinMode(ADDR_0, INPUT);
  // NOTE: for some reason on skeleton-1 calling this pinMode causes the board
  // to enter a reset loop so just comment this line out when compiling for that
  // board
  pinMode(ADDR_1, INPUT);
  pinMode(ADDR_2, INPUT);
  pinMode(ADDR_3, INPUT);

  StatusLED::setup(STATUS_RED, STATUS_GREEN, STATUS_BLUE);
  StatusLED::setColor(0, 0, 0);

  // read in the address
  address = readAddress();
  sprintf(controlAddr, CONTROL_ADDR_FMT, address);

#ifdef ENABLE_DEBUG
  Serial.print("\n\nControl Address: ");
  Serial.println(controlAddr);
#endif

  // Load the preferences and configure the controllers
  Preferences prefs;
  prefs.begin("connection_params", true);
#ifdef DMX_CTRL
  uint8_t dmxOffset = 0;
  dmxOffset = prefs.getUChar("dmx_offset", dmxOffset);
  DMXController::getInstance()->setDMXOffset(dmxOffset);
  DMXController::getInstance()->setServoController(&servoController);
#ifdef ENABLE_DEBUG
  Serial.printf("\nreading dmx offset %d\n", dmxOffset);
#endif
#endif
#ifdef WEBSOCKET_CTRL
  uint16_t webSocketPort = 8888;
  prefs.getString("websocket_host", webSocketHost, 40);
  webSocketPort = prefs.getShort("websocket_port", webSocketPort);
  webSocketController.setAddress(address);
  webSocketController.setServoController(&servoController);
  webSocketController.setConectionManager(&connectionManager);
  webSocketController.setHost(webSocketHost, webSocketPort);
#endif
  prefs.end();

  // start the wifi set up, its the first state we start in
  currentState = STATE_WIFI_CONNECTING;
#ifdef ENABLE_DEBUG
  Serial.println("Starting wifi manager");
#endif
  connectionManager.setup(controlAddr, OTA_PORT);

#ifdef ENABLE_DEBUG
  Serial.println("done starting wifi manager");
#endif

  // TODO: add this to the possible error states
  if (!servoController.setup()) {
    while (true) {
      StatusLED::setColor(255, 0, 0);
      delay(300);
      StatusLED::setColor(0, 0, 0);
      delay(300);
    }
  }
  StatusLED::setColor(0, 255, 0);
}

void loop() {
#ifdef REMOTE_DEBUG
  telnet.loop();
#endif
  webSocketController.loop();
#ifdef DMX_CTRL
  DMXController::getInstance()->loop();
#endif

  StatusLED::loop();
  connectionManager.loop();
  servoController.loop();
  switch (currentState) {
  case STATE_UNKNOWN:
    break;
  case STATE_WIFI_CONNECTING:
    if (connectionManager.currentState() == CONNECTION_STATE_CONNECTED) {
#ifdef ENABLE_DEBUG
      Serial.println("wifi connected");
#endif
      currentState = STATE_POST_WIFI_CONNECT;
    }
    return;
  case STATE_POST_WIFI_CONNECT: {
#ifdef ENABLE_DEBUG
    Serial.println("post wifi connect, configuring controllers");
#endif
#ifdef REMOTE_DEBUG
    telnet.onConnect(onTelnetConnect);
    telnet.onConnectionAttempt(onTelnetConnectionAttempt);
    telnet.onReconnect(onTelnetReconnect);
    telnet.onDisconnect(onTelnetDisconnect);
    telnet.onInputReceived(onTelnetInput);
    bool connected = telnet.begin();
#ifdef ENABLE_DEBUG
    Serial.print("- Telnet: ");
    if (connected) {
      Serial.println("running");
    } else {
      Serial.println("error.");
    }
#endif
#endif
    currentState = STATE_CONNECTED;
#ifdef WEBSOCKET_CTRL
    webSocketController.setup();
#endif
#ifdef DMX_CTRL
    DMXController::getInstance()->setup();
#endif
    connectionManager.otaSetup();
    break;
  }
  case STATE_CONNECTED:
    break;
  }
}

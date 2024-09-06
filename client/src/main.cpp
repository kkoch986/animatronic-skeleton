#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <PCA9685.h>
#include <WiFiManager.h>

#include "config.h"
#include "connection_manager.h"
#include "servo_controller.h"
#include "status_led.h"
#include "websocket.h"

ServoController servoController;
ConnectionManager connectionManager;

// TODO: accept this as part of the wifimanager set up. also need a way to reset
// it somehow
WebSocketsClient webSocket;
char webSocketHost[40] = "192.168.1.171";
int webSocketPort = 8888;
char controlAddr[32];
byte address = 0;
bool websocketConnected = false;
void webSocketEvent(WStype_t type, uint8_t *payload, size_t length) {
#ifdef ENABLE_DEBUG
  Serial.printf("websocket event %d\n", type);
#endif
  switch (type) {
  // ignore these events
  case WStype_TEXT:
  case WStype_FRAGMENT_TEXT_START:
  case WStype_FRAGMENT_BIN_START:
  case WStype_FRAGMENT:
  case WStype_FRAGMENT_FIN:
  case WStype_PONG:
    return;

  // handle these events
  case WStype_ERROR:
    // LATER: error handling
#ifdef ENABLE_DEBUG
    Serial.printf("[WSc] error: %s\n", payload);
#endif
    break;
  case WStype_DISCONNECTED:
    currentState = STATE_WEBSOCKET_CONNECTING;
    break;
  case WStype_CONNECTED:
    currentState = STATE_WEBSOCKET_CONNECTED;
    break;
  case WStype_PING:
    // handle ping message by responding with address
    char buff[128];
    sprintf(buff, "{\"address\": %d}", address);
    webSocket.sendTXT(buff, strlen(buff));
    return;
  case WStype_BIN: {
    byte addr = payload[0];
    if (addr != address) {
      // ignore messages that are not intended for this board
      return;
    }
    // LATER: replace the text protocol with a binary one
    uint8_t cmd = payload[1];
    switch (cmd) {
    case CT_SETMIN: {
      byte index = payload[2];
      byte val = payload[3];
      servoController.setLowerLimit(index, val);
      break;
    }
    case CT_SETMAX: {
      byte index = payload[2];
      byte val = payload[3];
      servoController.setUpperLimit(index, val);
      break;
    }
    case CT_QUERY_MOTOR_STATE: {
      uint16_t len = servoController.stateDumpLength();
      byte buff[len + 1];
      servoController.dumpState(buff);
      // shift the dump right and prepend the command code
      for (uint16_t i = len; i > 0; i--) {
        buff[i] = buff[i - 1];
      }
      buff[0] = CT_QUERY_MOTOR_STATE;
      webSocket.sendBIN(buff, len + 1);
      return;
    }
    case CT_MOVE: {
      byte index = payload[2];
      byte val = payload[3];
      servoController.move(index, val);
      break;
    }
    case CT_SET: {
      byte index = payload[2];
      byte val = payload[3];
      servoController.set(index, val);
      break;
    }
    case CT_SET_ENABLED: {
      byte index = payload[2];
      byte val = payload[3];
      servoController.setEnabled(index, val == 1);
      break;
    }
    case CT_SET_LABEL: {
      byte index = payload[2];
      byte labelSize = payload[3];
      servoController.setLabel(index, (char *)&payload[4], labelSize);
      break;
    }
    case CT_MOVEALL: {
      for (size_t i = 0; i < length - 2; i++) {
        if (i > SERVO_COUNT) {
          break;
        }
        servoController.move(i, payload[i + 2]);
      }
      break;
    }
    case CT_SETALL: {
      for (size_t i = 0; i < length - 2; i++) {
        if (i > SERVO_COUNT) {
          break;
        }
        servoController.set(i, payload[i + 2]);
      }
      break;
    }
    case CT_COMMIT: {
      servoController.commitState();
      break;
    }
    case CT_RESTORE: {
      StateRestoreStatus s = servoController.restoreFromStateDump();
      byte resp[2] = {CT_RESTORE, s};
      webSocket.sendBIN(resp, 2);
      break;
    }
    }
    break;
  }
  }
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
#ifdef ENABLE_DEBUG
  Serial.begin(115200);
#endif

  // configure pins
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
  Serial.print("Control Address: ");
  Serial.println(controlAddr);
#endif

  // start the wifi set up, its the first state we start in
  currentState = STATE_WIFI_CONNECTING;
  connectionManager.setup(controlAddr, OTA_PORT);

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
  switch (currentState) {
  case STATE_WIFI_CONNECTING:
    StatusLED::setColor(255, 0, 0);
    StatusLED::loop();
    connectionManager.loop();
    if (connectionManager.currentState() == CONNECTION_STATE_CONNECTED) {
      currentState = STATE_POST_WIFI_CONNECT;
    }
    return;
  case STATE_POST_WIFI_CONNECT:
    StatusLED::setColor(0, 0, 255);
    // Set up the OTA and start the websocket connection
    connectionManager.otaSetup();
    // connect to the websocket server
    webSocket.onEvent(webSocketEvent);
    webSocket.setReconnectInterval(1000);
#ifdef ENABLE_DEBUG
    Serial.printf("connecting to %s:%d\n", webSocketHost, webSocketPort);
#endif
    webSocket.begin(webSocketHost, webSocketPort, "/");
    currentState = STATE_WEBSOCKET_CONNECTING;
    break;
  case STATE_WEBSOCKET_CONNECTING:
    StatusLED::setColor(255, 0, 0);
    break;
  case STATE_WEBSOCKET_CONNECTED:
    StatusLED::setColor(0, 255, 0);
    break;
  }
  webSocket.loop();
  StatusLED::loop();
  connectionManager.loop();
  servoController.loop();
}

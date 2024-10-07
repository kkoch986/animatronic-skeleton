#include "controller_websocket.h"
#include "controller.h"
#ifdef ENABLE_DEBUG
#include <Arduino.h>
#endif

bool WebSocketController::setup() {
  // connect to the websocket server
  webSocket.onEvent([this](WStype_t type, uint8_t *payload, size_t length) {
    return webSocketEvent(type, payload, length);
  });
  webSocket.setReconnectInterval(1000);
#ifdef ENABLE_DEBUG
  Serial.printf("[WS] connecting to %s:%d\n", webSocketHost, webSocketPort);
#endif
  webSocket.begin(webSocketHost, webSocketPort, "/");
  return true;
}

void WebSocketController::setHost(char *host, uint16_t port) {
  webSocketHost = host;
  webSocketPort = port;
  connected = false;
}

void WebSocketController::queryMotorState() {
  uint16_t len = servoController->stateDumpLength();
  byte buff[len + 1];
  servoController->dumpState(buff);
  // shift the dump right and prepend the command code
  for (uint16_t i = len; i > 0; i--) {
    buff[i] = buff[i - 1];
  }
  buff[0] = CT_QUERY_MOTOR_STATE;
  webSocket.sendBIN(buff, len + 1);
}

void WebSocketController::sendStateRestoreStatus(StateRestoreStatus s) {
  byte resp[2] = {CT_RESTORE, s};
  webSocket.sendBIN(resp, 2);
}

void WebSocketController::loop() { webSocket.loop(); }

bool WebSocketController::isConnected() { return connected; }

void WebSocketController::webSocketEvent(WStype_t type, uint8_t *payload,
                                         size_t length) {
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
    connected = false;
    break;
  case WStype_CONNECTED:
    connected = true;
    break;
  case WStype_PING: {
    // handle ping message by responding with address
    char buff[128];
    sprintf(buff, "{\"address\": %d}", address);
    webSocket.sendTXT(buff, strlen(buff));
    return;
  }
  case WStype_BIN:
    handleEvent(payload, length);
  }
}

void WebSocketController::handleEvent(uint8_t *payload, size_t length) {
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
    servoController->setLowerLimit(index, val);
    break;
  }
  case CT_SETMAX: {
    byte index = payload[2];
    byte val = payload[3];
    servoController->setUpperLimit(index, val);
    break;
  }
  case CT_QUERY_MOTOR_STATE: {
    queryMotorState();
    return;
  }
  case CT_MOVE: {
    byte index = payload[2];
    byte val = payload[3];
    servoController->move(index, val);
    break;
  }
  case CT_SET: {
    byte index = payload[2];
    byte val = payload[3];
    servoController->set(index, val);
    break;
  }
  case CT_EYE_COLOR: {
    byte r = payload[2];
    byte g = payload[3];
    byte b = payload[4];
    servoController->setEyeColor(r, g, b);
    break;
  }
  case CT_SET_ENABLED: {
    byte index = payload[2];
    byte val = payload[3];
    servoController->setEnabled(index, val == 1);
    break;
  }
  case CT_SET_LABEL: {
    byte index = payload[1];
    byte labelSize = payload[3];
    servoController->setLabel(index, (char *)&payload[4], labelSize);
    break;
  }
  case CT_MOVEALL: {
    for (size_t i = 0; i < length - 2; i++) {
      if (i > SERVO_COUNT) {
        break;
      }
      servoController->move(i, payload[i + 2]);
    }
    break;
  }
  case CT_SETALL: {
    for (size_t i = 0; i < length - 2; i++) {
      if (i > SERVO_COUNT) {
        break;
      }
      servoController->set(i, payload[i + 2]);
    }
    break;
  }
  case CT_COMMIT: {
    servoController->commitState();
    break;
  }
  case CT_RESTORE: {
    StateRestoreStatus s = servoController->restoreFromStateDump();
    sendStateRestoreStatus(s);
    break;
  }
  case CT_RESTART: {
    ESP.restart();
    break;
  }
  case CT_RESET: {
    connectionManager->wipeConfig();
    break;
  }
  }
}

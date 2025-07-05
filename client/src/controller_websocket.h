#pragma once
#include "connection_manager.h"
#include "servo_controller.h"
#include <WebSocketsClient.h>

class WebSocketController {
private:
  char *webSocketHost;
  uint16_t webSocketPort;
  WebSocketsClient webSocket;
  bool connected;
  ServoController *servoController;
  ConnectionManager *connectionManager;
  byte address;
  void handleEvent(uint8_t *payload, size_t length);
  void webSocketEvent(WStype_t type, uint8_t *payload, size_t length);

protected:
  void queryMotorState();
  void sendStateRestoreStatus(StateRestoreStatus s);

public:
  bool setup();
  void loop();
  bool isConnected();

  void setHost(char *host, uint16_t port);
  void setAddress(byte addr) { address = addr; }
  void setServoController(ServoController *sc) { servoController = sc; }
  void setConectionManager(ConnectionManager *cm) { connectionManager = cm; }
  char *getHost() { return webSocketHost; }
  uint16_t getPort() { return webSocketPort; }
  byte getAddress() { return address; }
};

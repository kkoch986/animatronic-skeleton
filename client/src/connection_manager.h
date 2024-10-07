#pragma once

#include <ESP8266WiFi.h>
#include <Preferences.h>
#include <WiFiManager.h>

enum ConnectionState {
  CONNECTION_STATE_UNKNOWN,
  CONNECTION_STATE_NEEDS_CONFIG,
  CONNECTION_STATE_CONNECTED,
};

class ConnectionManager {
private:
  WiFiManager wifiManager;
  volatile bool connected;
  char hostname[32];
  uint16_t otaPort;

#ifdef WEBSOCKET_CTRL
  char webSocketHost[40];
  char webSocketPortStr[6];
  uint16_t webSocketPort;
#endif
#ifdef DMX_CTRL
  uint16_t dmxOffset;
#endif

  Preferences prefs;
  bool shouldSaveConfig = false;

public:
  void setup(char _hostname[32], uint16_t _otaPort);
  void loop();
  ConnectionState currentState();
  void wipeConfig();
  void otaSetup();
  uint16_t getOTAPort() { return otaPort; }
};

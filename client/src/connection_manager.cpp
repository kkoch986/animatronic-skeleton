#include "connection_manager.h"
#ifdef ENABLE_DEBUG
#include <Arduino.h>
#endif

#include "config.h"
#include <ArduinoOTA.h>

void ConnectionManager::wipeConfig() {
  wifiManager.erase();
  wifiManager.resetSettings();
  delay(200);
  ESP.restart();
}

ConnectionState ConnectionManager::currentState() {
  return connected ? CONNECTION_STATE_CONNECTED : CONNECTION_STATE_NEEDS_CONFIG;
}

void ConnectionManager::setup(char _hostname[32], uint16_t _otaPort) {
#ifdef ENABLE_DEBUG
  Serial.println("in connection manager setup\n");
#endif
  shouldSaveConfig = false;
  for (int i = 0; i < 32; i++) {
    hostname[i] = _hostname[i];
  }
  otaPort = _otaPort;

  WiFi.setHostname(hostname);
  wifiManager.setClass("invert");
  connected = false;
  wifiManager.setAPCallback([this](WiFiManager *wm) {
#ifdef ENABLE_DEBUG
    Serial.printf("AP CALLBACK REACHED, connected = false");
#endif
    connected = false;
  });
  wifiManager.setSaveConfigCallback([this]() {
    connected = true;
    shouldSaveConfig = true;
#ifdef ENABLE_DEBUG
    Serial.printf("SAVE CONFIG CALLBACK REACHED, connected = true");
#endif
  });

#ifdef WEBSOCKET_CTRL
  WiFiManagerParameter webSocketHostParam(
      "websocket_host", "WebSocket Hostname", "192.168.1.171", 40);
  WiFiManagerParameter webSocketPortParam("websocket_port", "WebSocket Port",
                                          "8888", 6);

  wifiManager.addParameter(&webSocketHostParam);
  wifiManager.addParameter(&webSocketPortParam);
#endif
#ifdef DMX_CTRL
  WiFiManagerParameter offsetParam("dmx_offset", "DMX Offset", "0", 4);
  wifiManager.addParameter(&offsetParam);
#endif
  connected = wifiManager.autoConnect(hostname);

#ifdef WEBSOCKET_CTRL
  strcpy(webSocketHost, webSocketHostParam.getValue());
  webSocketPort = atoi(webSocketPortParam.getValue());
#endif
#ifdef DMX_CTRL
  dmxOffset = atoi(offsetParam.getValue());
#endif
}

void ConnectionManager::loop() {
  if (shouldSaveConfig) {
    prefs.begin("connection_params", false);
#ifdef WEBSOCKET_CTRL
    prefs.putString("websocket_host", webSocketHost);
    prefs.putShort("websocket_port", webSocketPort);
#endif
#ifdef DMX_CTRL
#ifdef ENABLE_DEBUG
    Serial.printf("writing dmx offset %d", dmxOffset);
#endif
    prefs.putUChar("dmx_offset", dmxOffset);
#endif
    prefs.end();
    shouldSaveConfig = false;
  }
  wifiManager.process();
  ArduinoOTA.handle();

  if (digitalRead(RESET_BUTTON) == LOW) {
    wipeConfig();
  }
}

void ConnectionManager::otaSetup() {
  ArduinoOTA.setPort(otaPort);
  ArduinoOTA.setHostname(hostname);

  ArduinoOTA.onStart([]() {
#ifdef ENABLE_DEBUG
    String type;
    if (ArduinoOTA.getCommand() == U_FLASH) {
      type = "sketch";
    } else { // U_FS
      type = "filesystem";
    }
    Serial.println("Start updating " + type);
#endif
  });

  ArduinoOTA.onEnd([]() {
#ifdef ENABLE_DEBUG
    Serial.println("\nEnd");
#endif
  });

  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
  // Fade the LED out as progress is completed
#ifdef ENABLE_DEBUG
    unsigned int pct = (progress / (total / 100));
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
  });

  ArduinoOTA.begin();
}

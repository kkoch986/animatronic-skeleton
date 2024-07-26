#include "connection_manager.h"

#include "status_led.h"
#include <ArduinoOTA.h>

// TODO: can we use the gpio0 (flash button) to indicate we want to reset the
// wifi config?

void ConnectionManager::wipeConfig() {
  wifiManager.erase();
  wifiManager.resetSettings();
  ESP.restart();
}

ConnectionState ConnectionManager::currentState() {
  return connected ? CONNECTION_STATE_CONNECTED : CONNECTION_STATE_NEEDS_CONFIG;
}

void ConnectionManager::setup(char _hostname[32], uint16_t _otaPort) {
  for (int i = 0; i < 32; i++) {
    hostname[i] = _hostname[i];
  }
  otaPort = _otaPort;

  // TODO: set the hostname in the constructor
  WiFi.setHostname(hostname);
  wifiManager.setClass("invert");
  wifiManager.setConfigPortalBlocking(false);
  connected = false;
  wifiManager.setAPCallback([this](WiFiManager *wm) {
#ifdef ENABLE_DEBUG
    Serial.printf("AP CALLBACK REACHED, connected = false");
#endif
    connected = false;
  });
  wifiManager.setSaveConfigCallback([this]() {
    connected = true;
#ifdef ENABLE_DEBUG
    Serial.printf("SAVE CONFIG CALLBACK REACHED, connected = true");
#endif
  });
  connected = wifiManager.autoConnect(hostname);
}

void ConnectionManager::loop() {
  wifiManager.process();
  ArduinoOTA.handle();
}

// TODO: support the parameters

void ConnectionManager::otaSetup() {
  // TODO: parameterize this
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
    StatusLED::setColor(255, 255, 255);
  });

  ArduinoOTA.onEnd([]() {
  /* indicator.setColor(0, 0, 0); */
#ifdef ENABLE_DEBUG
    Serial.println("\nEnd");
#endif
  });

  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    // Fade the LED out as progress is completed
    unsigned int pct = (progress / (total / 100));
    /* byte color = 255 - (pct * 255) / 100; */
    /* indicator.setColor(color, color, color); */
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
      /* indicator.setColor(255, 0, 0); */
      delay(300);
      /* indicator.setColor(0, 0, 0); */
      delay(300);
    }
  });

  ArduinoOTA.begin();
}

#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <Preferences.h>

void onTelnetConnect(String ip) {
  Serial.print("- Telnet: ");
  Serial.print(ip);
  Serial.println(" connected");

  telnet.println("\nWelcome " + telnet.getIP());
  telnet.println("(Use ^] + q  to disconnect.)");
}

void onTelnetDisconnect(String ip) {
  Serial.print("- Telnet: ");
  Serial.print(ip);
  Serial.println(" disconnected");
}

void onTelnetReconnect(String ip) {
  Serial.print("- Telnet: ");
  Serial.print(ip);
  Serial.println(" reconnected");
}

void onTelnetConnectionAttempt(String ip) {
  Serial.print("- Telnet: ");
  Serial.print(ip);
  Serial.println(" tried to connected");
}

void _telnetDMX() {
#ifdef DMX_CTRL
  telnet.printf("> dmxc offset %d\n",
                DMXController::getInstance()->getDMXOffset());
#endif
}

void _telnetWS() {
#ifdef WEBSOCKET_CTRL
  telnet.printf("> ws host %s\n", webSocketController.getHost());
  telnet.printf("> ws port %d\n", webSocketController.getPort());
#endif
}

void _telnetNet() {
  uint8_t mac[6];
  WiFi.macAddress(mac);
  telnet.printf("> controlAddr %s\n", controlAddr);
  telnet.printf("> ota port    %d\n", connectionManager.getOTAPort());
  telnet.printf("> IP          %s\n", WiFi.localIP().toString().c_str());
  telnet.printf("> mac         %02x:%02x:%02x:%02x:%02x:%02x\n", mac[0], mac[1],
                mac[2], mac[3], mac[4], mac[5]);
}

void _telnetPrefs() {
  Preferences prefs;
  prefs.begin("connection_params", true);
#ifdef WEBSOCKET_CTRL
  char webSocketHost[40];
  uint16_t webSocketPort = 9999;
  prefs.getString("websocket_host", webSocketHost, 40);
  webSocketPort = prefs.getShort("websocket_port", webSocketPort);
  telnet.printf("> websocket host %s\n", webSocketHost);
  telnet.printf("> websocket port %d\n", webSocketPort);
#endif
#ifdef DMX_CTRL
  uint8_t dmxOffset = 99;
  dmxOffset = prefs.getUChar("dmx_offset", dmxOffset);
  telnet.printf("> dmx offset %d\n", dmxOffset);
#endif
  prefs.end();
}

void onTelnetInput(String str) {
  // checks for a certain command
  if (str == "dmx") {
    _telnetDMX();
  } else if (str == "ws") {
    _telnetWS();
  } else if (str == "net") {
    _telnetNet();
  } else if (str == "prefs") {
    _telnetPrefs();
  } else if (str == "restart") {
    ESP.restart();
  } else if (str == "reset") {
    connectionManager.wipeConfig();
  } else if (str == "help") {
    telnet.println("Available commands:");
    telnet.println("  dmx - print info from the dmx controller");
    telnet.println("  ws - print info about the websocket controller");
    telnet.println("  net - print network info");
    telnet.println("  prefs - print saved preferences");
    telnet.println("  restart - restart the device");
    telnet.println("  reset - reset the saved wifimanager configuration");
    telnet.println("  help - print this help message");
  } else {
    telnet.printf("unknown command '%s' (try 'help' for a list of commands)\n",
                  str);
  }
}

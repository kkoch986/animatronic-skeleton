#include "config.h"
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

void _telnetDMX(String arg) {
#ifdef DMX_CTRL
  if (arg.length() > 0) {
    int newOffset = arg.toInt();
    if (newOffset < 1 || newOffset > 512) {
      telnet.println("DMX offset must be between 1 and 512");
      return;
    }
    DMXController::getInstance()->setDMXOffset(newOffset);
    // store it in prefs
    Preferences prefs;
    prefs.begin("connection_params", false);
    prefs.putUChar("dmx_offset", newOffset);
    prefs.end();
    telnet.printf("Set DMX offset to %d\n", newOffset);
  }

  telnet.printf("> dmxc offset %d\n",
                DMXController::getInstance()->getDMXOffset());
#endif
}

void _telnetDMXValues() {
#ifdef DMX_CTRL
  uint8_t offset = DMXController::getInstance()->getDMXOffset();
  telnet.println("Current DMX Values:");
  telnet.println("┌──────┬───────┬─────────┬──────────────────────────────┐");
  telnet.println("│ Slot │ Value │ Purpose │ Description                  │");
  telnet.println("├──────┼───────┼─────────┼──────────────────────────────┤");

  // Show eye colors (first 3 slots)
  telnet.printf("│ %3d  │  %3d  │ Eye Red │ Red component of eye color   │\n",
                offset, DMXController::getInstance()->getDMXValue(offset));
  telnet.printf("│ %3d  │  %3d  │ Eye Grn │ Green component of eye color │\n",
                offset + 1,
                DMXController::getInstance()->getDMXValue(offset + 1));
  telnet.printf("│ %3d  │  %3d  │ Eye Blu │ Blue component of eye color  │\n",
                offset + 2,
                DMXController::getInstance()->getDMXValue(offset + 2));

  // Show servo values (slots 3 and up)
  for (int i = 3; i < SERVO_COUNT; i++) {
    int servoIndex = i - 3;
    uint8_t dmxValue = DMXController::getInstance()->getDMXValue(offset + i);
    char *label = servoController.getLabel(servoIndex);
    bool enabled = servoController.isEnabled(servoIndex);

    telnet.printf("│ %3d  │  %3d  │ Servo%2d │ %-7s %s                  │\n",
                  offset + i, dmxValue, servoIndex, label ? label : "N/A",
                  enabled ? "(EN)" : "(DIS)");
  }

  telnet.println("└──────┴───────┴─────────┴──────────────────────────────┘");
#else
  telnet.println("DMX controller not enabled in this build");
#endif
}

void _telnetWS() {
#ifdef WEBSOCKET_CTRL
  telnet.printf("> ws host %s\n", webSocketController.getHost());
  telnet.printf("> ws port %d\n", webSocketController.getPort());
  telnet.printf("> ws address 0x%02x\n", webSocketController.getAddress());
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

int consumeIntArg(String &cmd) {
  int idx = cmd.indexOf(' ');
  if (idx == -1) {
    cmd = "";
    return -1;
  }
  String arg = cmd.substring(0, idx);
  cmd = cmd.substring(idx + 1);
  return arg.toInt();
}

void _telnetMove(String cmd) {
  // parse the 2 args out
  int servoIndex = consumeIntArg(cmd);
  int position = consumeIntArg(cmd);

  if (servoIndex < 0 || servoIndex > 16) {
    telnet.println("servo index must be between 0 and 16");
    return;
  }
  if (position < 0 || position > 255) {
    telnet.println("position must be between 0 and 255");
    return;
  }

  // TODO: call the servo controller
  servoController.move(servoIndex, position);
}

void _telnetServo() {
  telnet.println("Servo Controller Status:");
  telnet.println(
      "┌────┬────────┬─────┬──────┬──────┬──────┬─────┬─────┬─────────┐");
  telnet.println(
      "│ ID │ Label  │ EN  │ Curr │ Targ │ Cntr │ Min │ Max │ Arrived │");
  telnet.println(
      "├────┼────────┼─────┼──────┼──────┼──────┼─────┼─────┼─────────┤");

  for (int i = 0; i < SERVO_COUNT; i++) {
    telnet.printf(
        "│%3d │ %-6s │ %-3s │ %3d  │ %3d  │ %3d  │ %3d │ %3d │ %-7s │\n", i,
        servoController.getLabel(i), servoController.isEnabled(i) ? "Y" : "N",
        servoController.getCurrentPosition(i),
        servoController.getTargetPosition(i),
        servoController.getCenterPosition(i), servoController.getLowerLimit(i),
        servoController.getUpperLimit(i),
        servoController.hasArrived(i) ? "Y" : "N");
  }

  telnet.println(
      "└────┴────────┴─────┴──────┴──────┴──────┴─────┴─────┴─────────┘");
}

void _printHelp() {
  telnet.println("Available commands:");
  telnet.println("  dmx [<dmx addr>] - print info from the dmx controller. if "
                 "provided, will set and store the provided address");
  telnet.println(
      "  dmxvalues - print current dmx values for all servos and eye colors");
  telnet.println("  ws - print info about the websocket controller");
  telnet.println("  net - print network info");
  telnet.println(
      "  move <servo index 0-16> <position 0 - 255> - move a particular servo");
  telnet.println("  prefs - print saved preferences");
  telnet.println("  servo - print info about servo configuration");
  telnet.println("  restart - restart the device");
  telnet.println("  reset - reset the saved wifimanager configuration");
  telnet.println("  help - print this help message");
}

void onTelnetInput(String str) {
  // checks for a certain command
  if (str.startsWith("dmx ")) {
    _telnetDMX(str.substring(4));
  } else if (str == "servo") {
    _telnetServo();
  } else if (str == "dmxvalues") {
    _telnetDMXValues();
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
  } else if (str.startsWith("move")) {
    _telnetMove(str.substring(5));
  } else if (str == "help") {
    _printHelp();
  } else {
    telnet.printf("unknown command '%s'\n\n", str);
    _printHelp();
  }
}

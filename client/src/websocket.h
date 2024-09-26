#include <WebSocketsClient.h>

// State holds the various states of execution that we can be in
// Since everything is async, this allows us to have the loop operate
// correctly and trigger transistions when necessary.
enum State {
  STATE_UNKNOWN,
  STATE_WIFI_CONNECTING,
  STATE_POST_WIFI_CONNECT,
  STATE_WEBSOCKET_CONNECTING,
  STATE_WEBSOCKET_CONNECTED,
};
State currentState = STATE_UNKNOWN;

// The various commands that we can handle over the websocket format
// These should be provided as the first byte of a binary websocket message
enum CommandType {
  // UNKNOWN is used when we cannot understand the message sent
  CT_UNKNOWN,
  // MOVE is used to set the position of a particular servo (or light)
  // when MOVE is used, the value will be travelled to via the smoothing
  // parameters
  // and not move immediately to the desired position.
  // This should be the preferred way to adjust motor position during
  // performances
  // since it prevents high-speed, jerky motion from happening
  // message format is as follows (4 bytes):
  // [ADDR][OP][S0][Val]
  CT_MOVE,
  // SET works like move, but will bypass any software based smoothing
  CT_SET,
  // MOVEALL works like move but will set a value for all of the servos
  // it should be provided as
  // [ADDR][OP][S0][S1]....[S15]
  // if less than 16 value bytes are provided, will fill up to the point that
  // is provided and not modify any others
  CT_MOVEALL,
  // SETALL works like MOVEALL but will bypass any software based smoothing
  CT_SETALL,
  // SETMIN will set the minimum PWM range for the given motor
  // Message format is:
  // [ADDR][OP][MINVAL]
  // MINVAL should be a single byte which will map 0 - 255 (indirectly) onto the
  // range of 100 - PCA9685_PWM_FULL (2^16~=65k)
  CT_SETMIN,
  // SETMAX will set the maximum PWM range for the given motor
  // works the same as SETMIN
  CT_SETMAX,
  // SETIDLE will set the idle position for the given motor
  // Message format is:
  // [ADDR][OP][IDLEVAL]
  // IDLEVAL should be a single byte which will map 0 - 255 onto the range of
  // [lowerLimit, upperLimit]
  CT_SETIDLE,
  // COMMIT will commit the current settings to the EEPROM
  CT_COMMIT,
  // RESTORE will restore the settings from the EEPROM
  CT_RESTORE,
  // CT_QUERY_MOTOR_STATE will cause the firmware to report the current state of
  // the motors. State is provided in a binary format as follows:
  // [CT_QUERY_MOTOR_STATE][servoController->dumpState]
  // see the ServoController dumpState function for the detailed breakdown
  // of the format
  CT_QUERY_MOTOR_STATE,
  // CT_SET_ENABLED will enable or disable a motor
  // message format is:
  // [ADDR][OP][SERVO_INDEX][ENABLED]
  CT_SET_ENABLED,
  // CT_SET_LABEL will set the label for a motor
  // message format is:
  // [ADDR][OP][SERVO_INDEX][LABEL_SIZE][LABEL]
  // Labels cannot exceed the SERVO_MAX_LABEL_SIZE
  // if one longer is provided, it will be truncated
  CT_SET_LABEL,
  // CT_EYE_COLOR will set the color of the eyes
  // message format is:
  // [ADDR][OP][R][G][B]
  CT_EYE_COLOR,
  // CT_RESTART will reboot the device
  CT_RESTART,
};

class WebSocketController {
private:
  WebSocketsClient webSocket;
  bool connected;
  void webSocketEvent(WStype_t type, uint8_t *payload, size_t length);

public:
  bool setup();
  void loop();

  bool isConnected();
};

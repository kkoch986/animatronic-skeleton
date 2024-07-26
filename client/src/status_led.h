#pragma once

class StatusLED {
private:
  static char greenPin;
  static char bluePin;
  static char redPin;

public:
  static void setup(char redPin, char greenPin, char bluePin);
  static void loop();

  // setColor will immediately set the status LED to the given color
  static void setColor(char red, char green, char blue);

  // flashSync will flash between the 2 colors with `delayMs` delay between
  // flashes `flashes` times. It is a blocking function that uses delay
  // internally so this should be reserved for use during startup since it could
  // result in delayed loop timings otherwise.
  static void flashSync(char red, char green, char blue, char red2, char green2,
                        char blue2, int delayMs, int flashes);
};

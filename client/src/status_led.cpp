#include "./status_led.h"
#include <Arduino.h>

char StatusLED::redPin = 0;
char StatusLED::greenPin = 0;
char StatusLED::bluePin = 0;

void StatusLED::setup(char redPin, char greenPin, char bluePin) {
  redPin = redPin;
  greenPin = greenPin;
  bluePin = bluePin;

  pinMode(redPin, OUTPUT);
  pinMode(greenPin, OUTPUT);
  pinMode(bluePin, OUTPUT);

  digitalWrite(redPin, LOW);
  digitalWrite(greenPin, LOW);
  digitalWrite(bluePin, LOW);
}

void StatusLED::loop() {
  // do nothing for now, eventually layer in more interesting stuff
  // like pulsing or blinking
}

void StatusLED::setColor(char red, char green, char blue) {
  digitalWrite(redPin, red);
  digitalWrite(greenPin, green);
  digitalWrite(bluePin, blue);
}

void StatusLED::flashSync(char red, char green, char blue, char red2,
                          char green2, char blue2, int delayMs, int flashes) {
  for (int i = 0; i < flashes; i++) {
    digitalWrite(redPin, red);
    digitalWrite(greenPin, green);
    digitalWrite(bluePin, blue);
    delay(delayMs);
    digitalWrite(redPin, red2);
    digitalWrite(greenPin, green2);
    digitalWrite(bluePin, blue2);
    delay(delayMs);
  }
}

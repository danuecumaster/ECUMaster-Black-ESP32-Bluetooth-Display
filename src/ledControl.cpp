#include <Arduino.h>

void ledRed() {
  digitalWrite(4, 0);
  digitalWrite(16, 1);
  digitalWrite(17, 1);
}

void ledGreen() {
  digitalWrite(4, 1);
  digitalWrite(16, 0);
  digitalWrite(17, 1);
}

void ledBlue() {
  digitalWrite(4, 1);
  digitalWrite(16, 1);
  digitalWrite(17, 0);
}

void ledWhite() {
  digitalWrite(4, 0);
  digitalWrite(16, 0);
  digitalWrite(17, 0);
}

void ledOff() {
  digitalWrite(4, 1);
  digitalWrite(16, 1);
  digitalWrite(17, 1);
}

#include "color.h"

#include <Arduino.h>

#include "config.h"

namespace color {

namespace {

TeamColor selected = TeamColor::BLUE;
bool locked = false;
uint32_t lastReadMs = 0;

const char* colorName(TeamColor teamColor) {
  return teamColor == TeamColor::BLUE ? "BLEU" : "JAUNE";
}

TeamColor readSelector() {
  // Pull-up interne : interrupteur ouvert = état haut = bleu ; fermé (relié à GND) = jaune.
  return (digitalRead(PIN_COLOR_SELECTOR) == LOW) ? TeamColor::YELLOW : TeamColor::BLUE;
}

}  // namespace

void begin() {
  pinMode(PIN_COLOR_SELECTOR, INPUT_PULLUP);
  selected = readSelector();
  if (DEBUG_LOGS) {
    Serial.printf("[COUL] Couleur : %s\n", colorName(selected));
  }
}

void update(uint32_t nowMs) {
  if (locked || nowMs - lastReadMs < COLOR_READ_PERIOD_MS) {
    return;
  }
  lastReadMs = nowMs;
  const TeamColor reading = readSelector();
  if (reading != selected) {
    selected = reading;
    if (DEBUG_LOGS) {
      Serial.printf("[COUL] Couleur : %s\n", colorName(selected));
    }
  }
}

void lock() {
  if (locked) {
    return;
  }
  locked = true;
  if (DEBUG_LOGS) {
    Serial.printf("[COUL] Couleur verrouillee : %s\n", colorName(selected));
  }
}

TeamColor current() {
  return selected;
}

}  // namespace color

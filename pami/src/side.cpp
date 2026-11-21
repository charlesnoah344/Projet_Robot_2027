#include "side.h"

#include <Arduino.h>

#include "config.h"
#include "console.h"

namespace side {

namespace {

StartSide selected = StartSide::LEFT;
bool locked = false;
uint32_t lastReadMs = 0;

StartSide readSelector() {
  // Pull-down : interrupteur ouvert = état bas = gauche ; fermé (relié au 3,3 V) = droite.
  // Un fil débranché lit aussi l'état bas : gauche. D'où le message à chaque changement.
  return (digitalRead(PIN_SIDE_SELECTOR) == HIGH) ? StartSide::RIGHT : StartSide::LEFT;
}

}  // namespace

void begin() {
  // Pull-down interne de l'ESP32 : pas de résistance à ajouter, l'interrupteur va de GPIO 27 au 3,3 V.
  pinMode(PIN_SIDE_SELECTOR, INPUT_PULLDOWN);
  selected = readSelector();
  if (DEBUG_LOGS) {
    Console.printf("[COTE] Depart a %s\n", sideName(selected));
  }
}

void update(uint32_t nowMs) {
  if (locked || nowMs - lastReadMs < SIDE_READ_PERIOD_MS) {
    return;
  }
  lastReadMs = nowMs;
  const StartSide reading = readSelector();
  if (reading != selected) {
    selected = reading;
    if (DEBUG_LOGS) {
      Console.printf("[COTE] Depart a %s\n", sideName(selected));
    }
  }
}

void lock() {
  if (locked) {
    return;
  }
  locked = true;
  if (DEBUG_LOGS) {
    Console.printf("[COTE] Cote verrouille : %s\n", sideName(selected));
  }
}

StartSide current() {
  return selected;
}

}  // namespace side

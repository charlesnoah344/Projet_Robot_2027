#include "match_timer.h"

#include <Arduino.h>

#include "config.h"
#include "logic/match_clock.h"
#include "logic/start_button.h"

namespace matchTimer {

namespace {

StartButtonDetector startButton(START_BUTTON_DEBOUNCE_MS);
bool wasArmed = false;
bool started = false;
bool over = false;
uint32_t startMs = 0;

}  // namespace

void begin() {
  // La carte a déjà une résistance de rappel sur GPIO 0 ; le pull-up interne ne gêne pas.
  pinMode(PIN_START_BUTTON, INPUT_PULLUP);
}

void update(uint32_t nowMs) {
  // Bouton BOOT : appuyé = état bas.
  const bool pressed = (digitalRead(PIN_START_BUTTON) == LOW);

  if (startButton.update(pressed, nowMs)) {
    started = true;
    startMs = nowMs;
    if (DEBUG_LOGS) {
      Serial.println("[TIME] DEPART ! Arret total dans 99,5 s.");
    }
  }
  if (DEBUG_LOGS && !wasArmed && startButton.isArmed()) {
    Serial.println("[TIME] Pret : appuyez sur BOOT pour partir.");
  }
  wasArmed = startButton.isArmed();

  if (started && !over && isMatchOver(startMs, nowMs)) {
    over = true;  // verrouillé : une fois fini, c'est fini
    if (DEBUG_LOGS) {
      Serial.println("[TIME] FIN DU MATCH (99,5 s) : tout s'arrete.");
    }
  }
}

bool isStarted() {
  return started;
}

bool isOver() {
  return over;
}

uint32_t elapsedMs(uint32_t nowMs) {
  return started ? elapsedSinceMs(startMs, nowMs) : 0;
}

}  // namespace matchTimer

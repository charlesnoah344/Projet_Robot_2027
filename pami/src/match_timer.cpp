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
bool moveAllowed = false;
bool over = false;
uint32_t startMs = 0;

}  // namespace

void begin() {
  // Pull-up interne : bouton relâché = état haut, sans résistance à ajouter.
  pinMode(PIN_START_BUTTON, INPUT_PULLUP);
}

void update(uint32_t nowMs) {
  // Bouton de départ : appuyé = état bas.
  // Avec la tirette (plus tard), cette ligne deviendra : retirée = état haut.
  const bool pressed = (digitalRead(PIN_START_BUTTON) == LOW);

  if (startButton.update(pressed, nowMs)) {
    started = true;
    startMs = nowMs;
    if (DEBUG_LOGS) {
      Serial.printf("[TIME] DEPART ! Mouvement dans %.1f s, arret total a %.1f s.\n",
                    MATCH_MOVE_ALLOWED_MS / 1000.0f, MATCH_STOP_MS / 1000.0f);
    }
  }
  if (DEBUG_LOGS && !wasArmed && startButton.isArmed()) {
    Serial.println("[TIME] Pret : appuyez sur le bouton de depart (GPIO 16).");
  }
  wasArmed = startButton.isArmed();

  if (started && !over && isMatchOver(startMs, nowMs)) {
    over = true;  // verrouillé : une fois fini, c'est fini
    if (DEBUG_LOGS) {
      Serial.printf("[TIME] FIN (%.1f s) : tout s'arrete.\n", MATCH_STOP_MS / 1000.0f);
    }
  }

  const bool wasMoveAllowed = moveAllowed;
  moveAllowed = started && !over && isInMovePhase(startMs, nowMs);
  if (DEBUG_LOGS && moveAllowed && !wasMoveAllowed) {
    Serial.println("[TIME] Mouvement autorise.");
  }
}

bool isStarted() {
  return started;
}

bool isMoveAllowed() {
  return moveAllowed;
}

bool isOver() {
  return over;
}

uint32_t elapsedMs(uint32_t nowMs) {
  return started ? elapsedSinceMs(startMs, nowMs) : 0;
}

}  // namespace matchTimer

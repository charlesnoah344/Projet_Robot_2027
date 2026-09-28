#include "state_machine.h"

#include <Arduino.h>

#include "config.h"
#include "match_timer.h"

namespace stateMachine {

namespace {

PamiState current = PamiState::IDLE;

void enter(PamiState next) {
  if (DEBUG_LOGS) {
    Serial.printf("[FSM] %s -> %s\n", stateName(current), stateName(next));
  }
  current = next;
}

}  // namespace

void begin() {
  current = PamiState::IDLE;
}

void update(uint32_t nowMs) {
  (void)nowMs;

  // Priorité absolue : à 99,5 s, END, quel que soit l'état (règle d'or n° 3).
  if (matchTimer::isOver()) {
    if (current != PamiState::END) {
      enter(PamiState::END);
    }
    return;
  }

  switch (current) {
    case PamiState::IDLE:
      if (matchTimer::isStarted()) {
        enter(PamiState::FORWARD);
      }
      break;

    case PamiState::FORWARD:
      // Étape 1 : on reste ici jusqu'à la fin du match. Les moteurs ne sont pas encore commandés.
      break;

    default:
      // STOP, DECIDE, BACKUP, TURN : étapes 5 à 7. END et FAULT : états finaux.
      break;
  }
}

PamiState state() {
  return current;
}

}  // namespace stateMachine

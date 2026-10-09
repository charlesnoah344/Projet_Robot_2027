#include "state_machine.h"

#include <Arduino.h>

#include "config.h"
#include "match_timer.h"
#include "sensors.h"

namespace stateMachine {

namespace {

PamiState current = PamiState::WAIT_START;

void enter(PamiState next) {
  if (DEBUG_LOGS) {
    Serial.printf("[FSM] %s -> %s\n", stateName(current), stateName(next));
  }
  current = next;
}

}  // namespace

void begin() {
  current = PamiState::WAIT_START;
}

void update(uint32_t nowMs) {
  (void)nowMs;

  // Priorité absolue : à 14,5 s, END, quel que soit l'état (règle d'or n° 3, profil labo D007).
  if (matchTimer::isOver()) {
    if (current != PamiState::END) {
      enter(PamiState::END);
    }
    return;
  }

  // Capteur en panne : FAULT jusqu'au redémarrage. safety freine déjà de son côté.
  if (sensors::isFaulty()) {
    if (current != PamiState::FAULT) {
      enter(PamiState::FAULT);
    }
    return;
  }

  switch (current) {
    case PamiState::WAIT_START:
      if (matchTimer::isStarted()) {
        enter(PamiState::WAIT_DELAY);
      }
      break;

    case PamiState::WAIT_DELAY:
      // Immobile tant que les 5 s ne sont pas écoulées.
      if (matchTimer::isMoveAllowed()) {
        enter(PamiState::MISSION);
      }
      break;

    case PamiState::MISSION:
      // Étape E : déroulement de la mission.
      break;

    default:
      // AVOID_STOP, DETOUR, DEFEND : étapes F et H. END et FAULT : états finaux.
      break;
  }
}

PamiState state() {
  return current;
}

}  // namespace stateMachine

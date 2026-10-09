#include "state_machine.h"

#include <Arduino.h>

#include "config.h"
#include "logic/avoidance.h"
#include "match_timer.h"
#include "mission.h"
#include "motion.h"
#include "safety.h"
#include "sensors.h"

namespace stateMachine {

namespace {

PamiState current = PamiState::WAIT_START;
uint32_t stateStartMs = 0;  // instant d'entrée dans l'état actuel

TeamColor teamColor = TeamColor::BLUE;  // étape G : lue sur le sélecteur

// Évitement.
enum class DetourPhase { TURNING, SIDESTEP };
DetourPhase detourPhase = DetourPhase::TURNING;
uint8_t detourCount = 0;

void enter(PamiState next, uint32_t nowMs) {
  if (DEBUG_LOGS) {
    Serial.printf("[FSM] %s -> %s\n", stateName(current), stateName(next));
  }
  current = next;
  stateStartMs = nowMs;
}

// Le PAMI avance vers un obstacle : on arrête le mouvement et on passe en AVOID_STOP.
// safety a déjà freiné ; ici, on annule le mouvement pour pouvoir en choisir un autre.
bool stopIfObstacle(uint32_t nowMs) {
  if (safety::obstacleAhead() && motion::isDriving()) {
    motion::stop();
    enter(PamiState::AVOID_STOP, nowMs);
    return true;
  }
  return false;
}

void startDetour(uint32_t nowMs) {
  detourCount++;
  const float side = chooseDetourSide(sensors::irLeftBlocked(), sensors::irRightBlocked(), teamColor);
  if (DEBUG_LOGS) {
    Serial.printf("[FSM] Detour %u par la %s\n", detourCount, side > 0.0f ? "gauche" : "droite");
  }
  motion::turnAngle(side * DETOUR_TURN_DEG);
  detourPhase = DetourPhase::TURNING;
  enter(PamiState::DETOUR, nowMs);
}

}  // namespace

void begin() {
  current = PamiState::WAIT_START;
  stateStartMs = millis();
}

void update(uint32_t nowMs) {
  // Priorité absolue : à 14,5 s, END, quel que soit l'état (règle d'or n° 3, profil labo D007).
  if (matchTimer::isOver()) {
    if (current != PamiState::END) {
      motion::stop();
      enter(PamiState::END, nowMs);
    }
    return;
  }

  // Capteur en panne ou roue bloquée : FAULT jusqu'au redémarrage. safety freine déjà de son côté.
  if (sensors::isFaulty() || motion::isFaulty()) {
    if (current != PamiState::FAULT) {
      motion::stop();
      enter(PamiState::FAULT, nowMs);
    }
    return;
  }

  switch (current) {
    case PamiState::WAIT_START:
      if (matchTimer::isStarted()) {
        enter(PamiState::WAIT_DELAY, nowMs);
      }
      break;

    case PamiState::WAIT_DELAY:
      // Immobile tant que les 5 s ne sont pas écoulées.
      if (matchTimer::isMoveAllowed()) {
        mission::start(teamColor);
        enter(PamiState::MISSION, nowMs);
      }
      break;

    case PamiState::MISSION:
      if (stopIfObstacle(nowMs)) {
        break;
      }
      mission::update();
      break;

    case PamiState::AVOID_STOP:
      // Niveau 1 : arrêté. Si l'obstacle s'en va, on reprend la mission là où elle en était.
      if (!safety::obstacleAhead()) {
        enter(PamiState::MISSION, nowMs);
        break;
      }
      // Niveau 2 : toujours là après AVOID_WAIT_MS -> détour (nombre limité).
      if (nowMs - stateStartMs >= AVOID_WAIT_MS && detourCount < AVOID_MAX_DETOURS) {
        startDetour(nowMs);
      }
      break;

    case PamiState::DETOUR:
      if (detourPhase == DetourPhase::SIDESTEP && stopIfObstacle(nowMs)) {
        break;
      }
      if (!motion::isDone()) {
        break;
      }
      if (detourPhase == DetourPhase::TURNING) {
        // Quart de tour fini : pas de côté.
        motion::driveDistance(DETOUR_SIDE_MM);
        detourPhase = DetourPhase::SIDESTEP;
      } else {
        // Niveau 3 : la mission vise de nouveau son point et contourne ainsi l'obstacle.
        enter(PamiState::MISSION, nowMs);
      }
      break;

    default:
      // DEFEND : étape H. END et FAULT : états finaux.
      break;
  }
}

PamiState state() {
  return current;
}

}  // namespace stateMachine

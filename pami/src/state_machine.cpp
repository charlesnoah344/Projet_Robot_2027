#include "state_machine.h"

#include <Arduino.h>

#include "arm.h"
#include "config.h"
#include "console.h"
#include "logic/avoidance.h"
#include "logic/detour.h"
#include "match_timer.h"
#include "mission.h"
#include "motion.h"
#include "safety.h"
#include "sensors.h"
#include "side.h"

namespace stateMachine {

namespace {

PamiState current = PamiState::WAIT_START;
uint32_t stateStartMs = 0;  // instant d'entrée dans l'état actuel

// Évitement (D016) : le contournement lui-même est décrit dans logic/detour.h.
Detour detour;
uint8_t detourCount = 0;  // contournements commencés depuis le début du match

void enter(PamiState next, uint32_t nowMs) {
  if (DEBUG_LOGS) {
    Console.printf("[FSM] %s -> %s\n", stateName(current), stateName(next));
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
  // Toujours vers le centre de la table : à droite si on est parti de la gauche, et inversement.
  const float detourSide = chooseDetourSide(side::current());
  if (DEBUG_LOGS) {
    Console.printf("[FSM] Contournement %u par la %s (vers le centre)\n", detourCount,
                   detourSide > 0.0f ? "gauche" : "droite");
  }
  detour.start(detourSide);
  enter(PamiState::DETOUR, nowMs);
}

// Exécute l'ordre suivant du contournement (logic/detour.h). Appelé quand le mouvement précédent est fini.
void runDetour(uint32_t nowMs) {
  const uint16_t frontMm = sensors::frontDistanceMm();
  const bool wasLooking = detour.isLooking();
  const DetourOrder order = detour.next(frontMm, nowMs);
  switch (order.type) {
    case DetourOrderType::TURN:
      if (DEBUG_LOGS) {
        Console.printf("[FSM] Contournement : quart de tour (%+.0f deg)\n", order.value);
      }
      motion::turnAngle(order.value);
      break;
    case DetourOrderType::DRIVE:
      if (DEBUG_LOGS && wasLooking) {
        Console.printf("[FSM] Contournement : voie libre (%u mm), on depasse l'obstacle (%.0f mm)\n", frontMm,
                       order.value);
      } else if (DEBUG_LOGS) {
        Console.printf("[FSM] Contournement : pas de cote (%.0f mm)\n", order.value);
      }
      motion::driveDistance(order.value);
      break;
    case DetourOrderType::WAIT:
      break;  // pause avant de regarder
    case DetourOrderType::DONE:
      // Obstacle dépassé : la mission vise de nouveau son point.
      if (DEBUG_LOGS) {
        Console.println("[FSM] Contournement reussi : reprise de la mission");
      }
      enter(PamiState::MISSION, nowMs);
      break;
    case DetourOrderType::FAILED:
      // Toujours bloqué après les pas de côté : on reste arrêté. AVOID_STOP repart dès que la voie se
      // libère, ou tente un autre contournement s'il en reste.
      if (DEBUG_LOGS) {
        Console.printf("[FSM] Contournement impossible : voie bloquee a %u mm apres %u pas de cote\n", frontMm,
                       detour.sidesteps());
      }
      enter(PamiState::AVOID_STOP, nowMs);
      break;
  }
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
      arm::release();
      enter(PamiState::END, nowMs);
    }
    return;
  }

  // Capteur en panne ou roue bloquée : FAULT jusqu'au redémarrage. safety freine déjà de son côté.
  if (sensors::isFaulty() || motion::isFaulty()) {
    if (current != PamiState::FAULT) {
      motion::stop();
      arm::release();
      enter(PamiState::FAULT, nowMs);
    }
    return;
  }

  switch (current) {
    case PamiState::WAIT_START:
      // La commande de départ a aussi choisi le côté (D020) : il ne changera plus.
      if (matchTimer::isStarted()) {
        enter(PamiState::WAIT_DELAY, nowMs);
      }
      break;

    case PamiState::WAIT_DELAY:
      // Immobile tant que les 5 s ne sont pas écoulées.
      if (matchTimer::isMoveAllowed()) {
        mission::start(side::current());
        enter(PamiState::MISSION, nowMs);
      }
      break;

    case PamiState::MISSION:
      if (stopIfObstacle(nowMs)) {
        break;
      }
      mission::update();
      if (mission::isDefendReached()) {
        arm::retract();
        enter(PamiState::DEFEND, nowMs);
      }
      break;

    case PamiState::AVOID_STOP:
      // Niveau 1 : arrêté. Si l'obstacle s'en va, on reprend la mission là où elle en était.
      if (!safety::obstacleAhead()) {
        enter(PamiState::MISSION, nowMs);
        break;
      }
      // Niveau 2 : toujours là après AVOID_WAIT_MS -> contournement (nombre limité, D016).
      if (nowMs - stateStartMs >= AVOID_WAIT_MS && detourCount < AVOID_MAX_DETOURS) {
        startDetour(nowMs);
      }
      break;

    case PamiState::DETOUR:
      // Niveau 3 : contournement par le centre. Chaque ligne droite (pas de côté, dépassement) reste
      // surveillée : un obstacle devant arrête tout et renvoie en AVOID_STOP.
      if (stopIfObstacle(nowMs)) {
        break;
      }
      // Un mouvement est en cours : on le laisse finir, puis on demande l'ordre suivant.
      if (!motion::isDone()) {
        break;
      }
      runDetour(nowMs);
      break;

    case PamiState::DEFEND:
      // Immobile dans la douve. Un adversaire à portée du bras : attaque à 120°, 3 s, puis retour
      // (D021, logic/arm_attack.h). L'évitement reste actif : safety interdit toujours d'avancer.
      arm::defend(sensors::frontDistanceMm() < ARM_TRIGGER_DISTANCE_MM, nowMs);
      break;

    default:
      // END et FAULT : états finaux.
      break;
  }
}

PamiState state() {
  return current;
}

}  // namespace stateMachine

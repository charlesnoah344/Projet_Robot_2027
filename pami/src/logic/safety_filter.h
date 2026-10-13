// Décisions du filtre de sécurité (logique pure, testable sur PC).
//
// safety.cpp lit le matériel (minuteur, capteurs) et appelle ces fonctions.
// Ici, aucun accès au matériel : on peut vérifier chaque règle sur PC.
#pragma once

#include <math.h>
#include <stdint.h>

#include "../config.h"
#include "wheel_command.h"

// Obstacle devant, avec une marge pour ne pas osciller autour du seuil :
// - pas d'obstacle avant : il en faut un plus proche que le seuil pour bloquer ;
// - obstacle avant : il faut qu'il soit au-delà de seuil + marge pour libérer.
inline bool isObstacleAhead(bool wasBlocked, uint16_t distanceMm) {
  if (wasBlocked) {
    return distanceMm < OBSTACLE_STOP_DISTANCE_MM + OBSTACLE_CLEAR_MARGIN_MM;
  }
  return distanceMm < OBSTACLE_STOP_DISTANCE_MM;
}

// Vrai si la commande fait tourner le PAMI sur place : roues en sens opposés, et la part
// « avance » (moyenne des roues) reste petite devant la part « rotation » (demi-différence).
// Les deux roues ne reçoivent jamais exactement la même PWM : d'où cette tolérance.
inline bool isTurnInPlace(const WheelCommand& command) {
  const bool oppositeSigns = (command.leftPct > 0.0f && command.rightPct < 0.0f) ||
                             (command.leftPct < 0.0f && command.rightPct > 0.0f);
  const float forwardPct = fabsf(command.leftPct + command.rightPct) / 2.0f;
  const float rotationPct = fabsf(command.rightPct - command.leftPct) / 2.0f;
  return oppositeSigns && forwardPct <= rotationPct / 2.0f;
}

// Commande vraiment envoyée aux moteurs. Freine (0, 0) dans chacun de ces cas :
// - en dehors de la période de mouvement (avant 5 s, après 14,5 s) ;
// - capteur en panne (sécurité par défaut : panne = obstacle) ;
// - le PAMI reculerait : aucun capteur ne regarde derrière ;
// - le PAMI avancerait vers un obstacle.
// Tourner sur place reste permis devant un obstacle : c'est ce qui permet de s'en écarter,
// et les capteurs avant balaient pendant la rotation.
inline WheelCommand filterCommand(const WheelCommand& requested, bool moveAllowed, bool sensorFault,
                                  bool obstacleAhead) {
  const WheelCommand stop;
  if (!moveAllowed || sensorFault) {
    return stop;
  }
  if (isTurnInPlace(requested)) {
    return requested;
  }
  // Pas une rotation sur place : aucune roue ne doit reculer.
  if (requested.leftPct < 0.0f || requested.rightPct < 0.0f) {
    return stop;
  }
  if (obstacleAhead && (requested.leftPct > 0.0f || requested.rightPct > 0.0f)) {
    return stop;
  }
  return requested;
}

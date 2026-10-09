// Décisions du filtre de sécurité (logique pure, testable sur PC).
//
// safety.cpp lit le matériel (minuteur, capteurs) et appelle ces fonctions.
// Ici, aucun accès au matériel : on peut vérifier chaque règle sur PC.
#pragma once

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

// Commande vraiment envoyée aux moteurs. Freine (0, 0) dans chacun de ces cas :
// - en dehors de la période de mouvement (avant 5 s, après 14,5 s) ;
// - capteur en panne (sécurité par défaut : panne = obstacle) ;
// - le PAMI reculerait : aucun capteur ne regarde derrière ;
// - le PAMI avancerait vers un obstacle.
// Tourner sur place (une roue en avant, l'autre en arrière, à la même vitesse) reste permis
// devant un obstacle : c'est ce qui permet de s'en écarter.
inline WheelCommand filterCommand(const WheelCommand& requested, bool moveAllowed, bool sensorFault,
                                  bool obstacleAhead) {
  const WheelCommand stop;
  if (!moveAllowed || sensorFault) {
    return stop;
  }
  // Composante « avance » de la commande : moyenne des deux roues.
  // 1 % de tolérance : une rotation sur place n'est jamais parfaitement symétrique.
  const float forwardPct = (requested.leftPct + requested.rightPct) / 2.0f;
  if (forwardPct < -1.0f) {
    return stop;
  }
  if (obstacleAhead && forwardPct > 1.0f) {
    return stop;
  }
  return requested;
}

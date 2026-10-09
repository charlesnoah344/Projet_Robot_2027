// Profil de vitesse en trapèze (logique pure, testable sur PC).
//
// Vitesse
//   ^      ______________
//   |     /              \        accélère, roule à vitesse constante, freine
//   |    /                \       pour s'arrêter pile sur la cible
//   +---------------------------> distance
#pragma once

#include <math.h>

#include "../config.h"

// Vitesse voulue pour ce cycle, en mm/s (toujours positive).
// currentMmS : vitesse voulue au cycle précédent. remainingMm : distance restante (>= 0).
inline float nextProfileSpeed(float currentMmS, float remainingMm, float cruiseMmS, float dtS) {
  // On accélère d'au plus MOTION_ACCEL_MM_S2 par seconde.
  float speedMmS = currentMmS + MOTION_ACCEL_MM_S2 * dtS;
  if (speedMmS > cruiseMmS) {
    speedMmS = cruiseMmS;
  }
  // Vitesse qui permet encore de s'arrêter sur la distance restante : v = racine(2 × a × d).
  const float brakingMmS = sqrtf(2.0f * MOTION_ACCEL_MM_S2 * remainingMm);
  if (speedMmS > brakingMmS) {
    speedMmS = brakingMmS;
  }
  // Jamais trop lent : en dessous, le moteur risque de caler avant la cible.
  if (speedMmS < MOTION_MIN_SPEED_MM_S) {
    speedMmS = MOTION_MIN_SPEED_MM_S;
  }
  return speedMmS;
}

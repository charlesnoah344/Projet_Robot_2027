// Choix de la douve la plus proche (logique pure, testable sur PC, D014).
#pragma once

#include <stdint.h>

#include "../config.h"

// Indice de la douve la plus proche du point (xMm, yMm). En cas d'égalité, la première de la liste.
// On choisit au départ, depuis (0, 0), sur les positions écrites pour un départ à gauche :
// le miroir (départ à droite) ne change aucune distance depuis (0, 0), donc le choix est le même
// des deux côtés.
inline uint8_t nearestMoatIndex(const MoatPoint moats[], uint8_t count, float xMm, float yMm) {
  uint8_t bestIndex = 0;
  float bestSquaredMm2 = 0.0f;
  for (uint8_t i = 0; i < count; i++) {
    const float dxMm = moats[i].xMm - xMm;
    const float dyMm = moats[i].yMm - yMm;
    // Distance au carré : pour comparer deux distances, pas besoin de la racine carrée.
    const float squaredMm2 = dxMm * dxMm + dyMm * dyMm;
    if (i == 0 || squaredMm2 < bestSquaredMm2) {
      bestIndex = i;
      bestSquaredMm2 = squaredMm2;
    }
  }
  return bestIndex;
}

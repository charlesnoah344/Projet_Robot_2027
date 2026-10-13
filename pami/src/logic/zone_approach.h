// Approche de la zone avec les capteurs de sol (logique pure, testable sur PC, D009).
//
// Principe : au début de l'approche, on note ce que voient les deux capteurs de sol.
// Dès que l'un des deux voit autre chose, le PAMI est au bord de la zone.
// Comparer au sol de départ marche que la zone soit plus claire ou plus foncée que le sol.
#pragma once

#include <math.h>

#include "../config.h"
#include "odometry.h"

// Vrai si le sol sous l'un des deux capteurs a changé depuis le début de l'approche.
inline bool floorChanged(bool startLeft, bool startRight, bool nowLeft, bool nowRight) {
  return nowLeft != startLeft || nowRight != startRight;
}

// Distance qu'il reste à parcourir avant d'abandonner la recherche (jamais négative).
// On mesure en ligne droite depuis le point où l'approche a commencé : après un détour,
// la distance déjà faite compte toujours.
inline float remainingSearchMm(const Pose& searchStart, const Pose& now) {
  const float dxMm = now.xMm - searchStart.xMm;
  const float dyMm = now.yMm - searchStart.yMm;
  const float doneMm = sqrtf(dxMm * dxMm + dyMm * dyMm);
  return fmaxf(ZONE_SEARCH_MAX_MM - doneMm, 0.0f);
}

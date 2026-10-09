// Choix du côté du détour (logique pure, testable sur PC).
#pragma once

#include "mission_step.h"

// Renvoie +1 pour contourner par la gauche, -1 par la droite.
// Les IR avant-gauche et avant-droit disent où est l'obstacle : on part de l'autre côté.
// Sans indice (aucun IR ou les deux), on prend un côté par défaut : la gauche pour le bleu.
// Le jaune prend la droite : son trajet est le miroir de celui du bleu.
inline float chooseDetourSide(bool irLeftBlocked, bool irRightBlocked, TeamColor color) {
  if (irLeftBlocked && !irRightBlocked) {
    return -1.0f;
  }
  if (irRightBlocked && !irLeftBlocked) {
    return 1.0f;
  }
  return (color == TeamColor::BLUE) ? 1.0f : -1.0f;
}

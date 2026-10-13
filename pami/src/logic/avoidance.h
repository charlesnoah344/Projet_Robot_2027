// Choix du côté du détour (logique pure, testable sur PC).
#pragma once

#include "mission_step.h"

// Renvoie +1 pour contourner par la gauche, -1 par la droite.
// Aucun capteur ne dit de quel côté l'obstacle est le plus court à contourner (les IR regardent
// le sol, D009) : on prend un côté fixe. La gauche pour le bleu ; la droite pour le jaune, dont
// le trajet est le miroir de celui du bleu.
inline float chooseDetourSide(TeamColor color) {
  return (color == TeamColor::BLUE) ? 1.0f : -1.0f;
}

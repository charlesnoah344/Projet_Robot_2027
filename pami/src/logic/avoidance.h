// Choix du côté du détour (logique pure, testable sur PC).
#pragma once

#include "mission_step.h"

// Renvoie +1 pour contourner par la gauche, -1 par la droite.
// Le détour se fait toujours vers le CENTRE de la table (D014) : on n'a pas envie de se coincer
// contre une bordure. PAMI GAUCHE (bord à sa gauche) : le centre est à sa droite ; PAMI DROITE :
// à sa gauche (D017). C'est aussi le miroir l'un de l'autre, comme les trajets.
// Limite : c'est juste tant que le PAMI ne s'est pas retourné de plus de 90° par rapport à son cap
// de départ. Sur le trajet de la douve (D017), le plus grand virage fait environ 51° : c'est bon.
inline float chooseDetourSide(StartSide side) {
  return (side == StartSide::LEFT) ? -1.0f : 1.0f;
}

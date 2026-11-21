// Choix du côté du détour (logique pure, testable sur PC).
#pragma once

#include "mission_step.h"

// Renvoie +1 pour contourner par la gauche, -1 par la droite.
// Le détour se fait toujours vers le CENTRE de la table (D014) : on n'a pas envie de se coincer
// contre une bordure. Parti du côté gauche de la table, le PAMI a le centre à sa droite ; parti du
// côté droit, il l'a à sa gauche. C'est aussi le miroir l'un de l'autre, comme les trajets.
// Limite : c'est juste tant que le PAMI ne s'est pas retourné de plus de 90° par rapport à son cap
// de départ. À vérifier quand les vraies positions des douves seront connues.
inline float chooseDetourSide(StartSide side) {
  return (side == StartSide::LEFT) ? -1.0f : 1.0f;
}

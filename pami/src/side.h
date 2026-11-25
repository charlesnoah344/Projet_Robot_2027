// Côté du PAMI (D014, D017, D020) : posé dans son écurie, regard vers l'adversaire, le PAMI a-t-il
// le bord de la table à sa gauche (GAUCHE) ou à sa droite (DROITE) ?
// Le centre est toujours de l'autre côté. Ce n'est pas le côté vu du public (docs/materiel.md).
//
// Plus d'interrupteur : le côté est choisi avec la commande de départ (« go g » ou « go d », D020),
// une seule fois. Il ne change plus jusqu'à la fin.
// Le côté sert à deux choses :
// - les trajets, écrits pour GAUCHE, sont mis en miroir pour DROITE (logic/mission_step.h) ;
// - le détour se fait vers le centre de la table (logic/avoidance.h).
#pragma once

#include "logic/mission_step.h"

namespace side {

// Appelé par le minuteur quand la commande de départ arrive. Seul le premier appel compte.
void choose(StartSide chosen);

// Vrai une fois le côté choisi (au départ).
bool isChosen();

// Côté choisi. Avant le départ : GAUCHE, mais rien ne bouge avant le départ.
StartSide current();

}  // namespace side

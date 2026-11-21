// Interrupteur de côté (GPIO 27, D014, D015) : le PAMI part-il du côté gauche ou du côté droit de la table ?
//
// Lu en continu avant le départ, puis verrouillé au départ : toucher l'interrupteur pendant
// le trajet ne change plus rien. Ouvert = GAUCHE, fermé = DROITE.
// Le côté sert à deux choses :
// - les trajets, écrits pour la gauche, sont mis en miroir pour la droite (logic/mission_step.h) ;
// - le détour se fait vers le centre de la table (logic/avoidance.h).
#pragma once

#include <stdint.h>

#include "logic/mission_step.h"

namespace side {

void begin();

// À appeler à chaque tour de loop(). Ne fait plus rien une fois le côté verrouillé.
void update(uint32_t nowMs);

// Fige le côté. À appeler au moment du départ.
void lock();

StartSide current();

}  // namespace side

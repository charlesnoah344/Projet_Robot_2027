// Couche mouvement : transforme « avance », « tourne », « recule » en commandes pour les roues.
//
// Étape 1 : squelette. Il demande toujours l'arrêt.
// La régulation par les encodeurs arrive à l'étape 4.
#pragma once

#include <stdint.h>

#include "logic/wheel_command.h"

namespace motion {

void begin();

void update(uint32_t nowMs);

// Commande demandée pour les roues. Elle passe ensuite par safety, qui a le dernier mot.
WheelCommand command();

}  // namespace motion

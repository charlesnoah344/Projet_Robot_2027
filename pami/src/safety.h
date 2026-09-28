// Filtre de sécurité : la dernière étape avant le driver moteurs.
//
// Toutes les commandes moteurs du firmware passent par ici. C'est le seul module
// qui appelle motorDriver. Aucun réglage ne permet de désactiver ce filtre.
#pragma once

#include <stdint.h>

#include "logic/wheel_command.h"

namespace safety {

// Reçoit la commande demandée par motion et décide de ce qui est vraiment envoyé aux moteurs.
void apply(const WheelCommand& requested, uint32_t nowMs);

}  // namespace safety

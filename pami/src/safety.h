// Filtre de sécurité : la dernière étape avant le driver moteurs.
//
// Toutes les commandes moteurs du firmware passent par ici. C'est le seul module
// qui appelle motorDriver. Aucun réglage ne permet de désactiver ce filtre.
// Les règles elles-mêmes sont dans logic/safety_filter.h.
#pragma once

#include <stdint.h>

#include "logic/wheel_command.h"

namespace safety {

// Reçoit la commande demandée par motion et décide de ce qui est vraiment envoyé aux moteurs.
void apply(const WheelCommand& requested, uint32_t nowMs);

// Vrai si un obstacle est devant, plus proche que le seuil d'arrêt (avec marge de libération).
// La machine à états s'en sert pour passer en AVOID_STOP.
bool obstacleAhead();

// Vrai si safety a dû bloquer la dernière commande (utile pour les logs et pour motion).
bool isBlocking();

}  // namespace safety

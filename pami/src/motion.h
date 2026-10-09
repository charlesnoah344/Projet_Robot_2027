// Couche mouvement : transforme « avance de 300 mm » ou « tourne de 90° » en commandes de roues.
//
// Mesures par les encodeurs (D004) :
// - vitesse de chaque roue régulée (logic/speed_controller.h) ;
// - ligne droite : l'écart entre les distances des deux roues est corrigé en permanence ;
// - accélération et freinage progressifs (logic/speed_profile.h) ;
// - roue bloquée ou encodeur inversé : FAULT.
// Pas de marche arrière : on avance, ou on tourne sur place (safety refuse le reste).
#pragma once

#include <stdint.h>

#include "logic/wheel_command.h"

namespace motion {

void begin();

// À appeler à chaque tour de loop(), avant safety. Recalcule les commandes toutes les 20 ms.
void update(uint32_t nowMs);

// Avance tout droit de distanceMm (> 0). Remplace le mouvement en cours.
void driveDistance(float distanceMm);

// Tourne sur place de angleDeg. Positif = vers la gauche (sens inverse des aiguilles d'une montre).
void turnAngle(float angleDeg);

// Arrête tout mouvement (freinage).
void stop();

// Vrai quand le dernier mouvement demandé est terminé (ou qu'il n'y en a pas).
bool isDone();

// Vrai si une roue est bloquée ou tourne à l'envers. Le reste jusqu'au redémarrage.
bool isFaulty();

// Commande demandée pour les roues. Elle passe ensuite par safety, qui a le dernier mot.
WheelCommand command();

}  // namespace motion

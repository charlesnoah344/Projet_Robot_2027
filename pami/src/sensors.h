// Couche capteurs : ultrason devant (étape C), 2 IR dans les angles avant (étape F).
//
// Rien n'attend jamais : l'écho de l'ultrason est mesuré par une interruption sur ECHO,
// pas avec pulseIn(), qui bloquerait la boucle jusqu'à 38 ms.
#pragma once

#include <stdint.h>

namespace sensors {

void begin();

// À appeler à chaque tour de loop(). Chaque capteur est lu à son propre rythme.
void update(uint32_t nowMs);

// Distance devant, filtrée (médiane des 3 dernières mesures), en mm.
// US_NO_OBSTACLE_MM quand il n'y a rien dans la portée.
uint16_t frontDistanceMm();

// Vrai si un capteur est en panne (aucune mesure valide depuis 1 s).
// Une fois vrai, le reste jusqu'au redémarrage : on ne repart pas avec un capteur douteux.
bool isFaulty();

}  // namespace sensors

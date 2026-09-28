// Couche capteurs : ultrason (devant) et 2 IR (angles avant).
//
// Étape 1 : squelette vide. L'ultrason arrive à l'étape 5, les IR à l'étape 6.
#pragma once

#include <stdint.h>

namespace sensors {

void begin();

// À appeler à chaque tour de loop(). Chaque capteur est lu à son propre rythme, sans jamais attendre.
void update(uint32_t nowMs);

}  // namespace sensors

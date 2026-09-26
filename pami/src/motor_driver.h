// Couche la plus basse : pilote le driver moteurs MDD3A (D006).
//
// SEUL safety appelle ce module dans le firmware.
// Seuls les programmes de test_bench/, jamais utilisés en match, peuvent l'appeler directement.
#pragma once

namespace motorDriver {

// À appeler EN PREMIER dans setup() : met les 4 entrées du driver à l'état bas (moteurs freinés).
void begin();

// Freine les deux moteurs : A = B = 0 sur le MDD3A.
void brake();

}  // namespace motorDriver

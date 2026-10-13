// Machine à états du PAMI défenseur (voir docs/plan-logiciel-pami.md, § 3).
//
// WAIT_START -> WAIT_DELAY -> MISSION <-> AVOID_STOP / DETOUR -> DEFEND, et END à 14,5 s.
#pragma once

#include <stdint.h>

#include "logic/pami_state.h"

namespace stateMachine {

void begin();

// À appeler à chaque tour de loop(), après le minuteur et les capteurs.
void update(uint32_t nowMs);

PamiState state();

}  // namespace stateMachine

// Machine à états d'exploration (voir pami/CLAUDE.md).
//
// Étape 1 : seulement IDLE -> FORWARD -> END. En FORWARD, les moteurs ne sont pas encore commandés.
#pragma once

#include <stdint.h>

#include "logic/pami_state.h"

namespace stateMachine {

void begin();

// À appeler à chaque tour de loop(), après le minuteur et les capteurs.
void update(uint32_t nowMs);

PamiState state();

}  // namespace stateMachine

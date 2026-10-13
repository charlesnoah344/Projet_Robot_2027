// Sélecteur de couleur (interrupteur sur GPIO 17).
//
// Lu en continu avant le départ, puis verrouillé au départ : toucher l'interrupteur pendant
// le trajet ne change plus rien. Ouvert = bleu, fermé = jaune.
#pragma once

#include <stdint.h>

#include "logic/mission_step.h"

namespace color {

void begin();

// À appeler à chaque tour de loop(). Ne fait plus rien une fois la couleur verrouillée.
void update(uint32_t nowMs);

// Fige la couleur. À appeler au moment du départ.
void lock();

TeamColor current();

}  // namespace color

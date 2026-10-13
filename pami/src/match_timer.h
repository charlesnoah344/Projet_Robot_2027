// Minuteur global du match, profil labo (D007) :
// - départ par le bouton-poussoir externe (GPIO 16), jamais automatique à l'allumage ;
// - immobile pendant MATCH_MOVE_ALLOWED_MS (5 s) après le départ ;
// - arrêt total à MATCH_STOP_MS (14,5 s) après le départ, quel que soit l'état du programme.
#pragma once

#include <stdint.h>

namespace matchTimer {

void begin();

// À appeler à chaque tour de loop(), avant tout le reste : lit le bouton et suit le temps.
void update(uint32_t nowMs);

// Vrai dès que le départ a été donné.
bool isStarted();

// Vrai entre 5 s et 14,5 s après le départ : les moteurs ont le droit de tourner.
bool isMoveAllowed();

// Vrai à partir de 14,5 s après le départ. Une fois vrai, le reste pour toujours.
bool isOver();

// Temps écoulé depuis le départ (0 avant le départ).
uint32_t elapsedMs(uint32_t nowMs);

}  // namespace matchTimer

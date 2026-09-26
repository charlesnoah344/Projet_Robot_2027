// Minuteur global du match.
//
// Phase de développement (exception D005) :
// - départ provisoire par le bouton BOOT, jamais automatique à l'allumage ;
// - PAS d'attente de 85 s. À IMPLÉMENTER AVANT TOUT MATCH (avec la tirette) ;
// - arrêt total à 99,5 s après le départ, quel que soit l'état du programme.
#pragma once

#include <stdint.h>

namespace matchTimer {

void begin();

// À appeler à chaque tour de loop(), avant tout le reste : lit le bouton et suit le temps.
void update(uint32_t nowMs);

// Vrai dès que le départ a été donné.
bool isStarted();

// Vrai à partir de 99,5 s après le départ. Une fois vrai, le reste pour toujours.
bool isOver();

// Temps écoulé depuis le départ (0 avant le départ).
uint32_t elapsedMs(uint32_t nowMs);

}  // namespace matchTimer

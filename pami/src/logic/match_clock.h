// Calculs de temps du match (logique pure, testable sur PC).
#pragma once

#include <stdint.h>

#include "../config.h"

// Temps écoulé depuis le départ.
// La soustraction entre entiers non signés reste juste même quand millis() repasse par zéro
// (au bout de 49 jours). C'est pour ça qu'on n'écrit jamais « nowMs > startMs + duree ».
inline uint32_t elapsedSinceMs(uint32_t startMs, uint32_t nowMs) {
  return nowMs - startMs;
}

// Vrai à partir de MATCH_STOP_MS (99,5 s) après le départ : tout doit être arrêté.
inline bool isMatchOver(uint32_t startMs, uint32_t nowMs) {
  return elapsedSinceMs(startMs, nowMs) >= MATCH_STOP_MS;
}

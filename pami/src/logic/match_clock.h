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

// Vrai à partir de MATCH_STOP_MS (14,5 s, profil labo D007) après le départ : tout doit être arrêté.
inline bool isMatchOver(uint32_t startMs, uint32_t nowMs) {
  return elapsedSinceMs(startMs, nowMs) >= MATCH_STOP_MS;
}

// Vrai entre MATCH_MOVE_ALLOWED_MS (5 s) et MATCH_STOP_MS (14,5 s) après le départ :
// c'est la seule période où les moteurs ont le droit de tourner.
inline bool isInMovePhase(uint32_t startMs, uint32_t nowMs) {
  const uint32_t elapsedMs = elapsedSinceMs(startMs, nowMs);
  return elapsedMs >= MATCH_MOVE_ALLOWED_MS && elapsedMs < MATCH_STOP_MS;
}

// Médiane de 3 valeurs (logique pure, testable sur PC).
//
// Pourquoi la médiane et pas la moyenne : une mesure ultrason aberrante (écho parasite)
// est simplement écartée, alors qu'une moyenne la mélangerait aux bonnes mesures.
#pragma once

#include <stdint.h>

inline uint16_t median3(uint16_t a, uint16_t b, uint16_t c) {
  if (a > b) {
    const uint16_t swap = a;
    a = b;
    b = swap;
  }
  // Ici a <= b. La médiane est b si c est au-dessus, sinon le plus grand de a et c.
  if (c >= b) {
    return b;
  }
  return (c > a) ? c : a;
}

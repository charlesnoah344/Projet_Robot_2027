// Couche encodeurs : compte les ticks des deux roues (D004, D022).
//
// Encodeurs à effet Hall des FIT0450 : deux voies A et B décalées d'un quart de période.
// Le comptage est fait par le compteur matériel de l'ESP32 (PCNT), un par roue, et non plus par
// des interruptions (D022) : à chaque front de A, le circuit lit lui-même B pour connaître le sens.
// - Pas de retard : une interruption en retard (Wi-Fi) faisait compter des ticks à l'envers.
// - Filtre anti-parasites : les impulsions plus courtes que ENCODER_GLITCH_FILTER_NS sont ignorées.
//   Les moteurs envoyaient des parasites sur les fils des encodeurs (T021 : à 350 mm/s, une roue
//   tournait 32 % plus vite que l'autre alors que les encodeurs les annonçaient égales).
#pragma once

#include <stdint.h>

namespace encoders {

// Configure les deux compteurs matériels et le minuteur qui ramasse leurs valeurs.
void begin();

// Lit les deux compteurs au même instant (section critique).
// Ticks depuis l'allumage, positifs quand la roue fait avancer le PAMI.
void read(int32_t& leftTicks, int32_t& rightTicks);

}  // namespace encoders

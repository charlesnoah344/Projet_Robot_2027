// Couche encodeurs : compte les ticks des deux roues (D004).
//
// Encodeurs à effet Hall des FIT0450 : deux voies A et B décalées d'un quart de période.
// Une interruption sur chaque front de A ; à chaque front, on lit B pour connaître le sens.
// Effet Hall = pas de contact mécanique, donc pas de rebond : aucun anti-rebond nécessaire.
#pragma once

#include <stdint.h>

namespace encoders {

// Configure les broches A/B et attache les interruptions.
void begin();

// Lit les deux compteurs au même instant (section critique : une interruption ne peut pas
// modifier un compteur pendant la lecture).
// Ticks depuis l'allumage, positifs quand la roue fait avancer le PAMI.
void read(int32_t& leftTicks, int32_t& rightTicks);

}  // namespace encoders

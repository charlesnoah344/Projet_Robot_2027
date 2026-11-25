// Bras du PAMI : un servo SG90 qui attaque le PAMI adverse (D008, D021).
//
// - Pas de PWM avant la défense : le servo reste libre, le bras est mis en place à la main.
// - Le bras ne bouge que pendant la période de mouvement (5 s à 14,5 s).
// - Attaque : 0° -> 120°, 3 s, puis retour à 0° (logic/arm_attack.h).
// - À la fin, le PWM est coupé (servo « détaché ») : le bras reste où il est, sans force.
#pragma once

#include <stdint.h>

namespace arm {

void begin();

// À appeler à chaque tour de loop(). Coupe le servo dès la fin du temps, quel que soit l'état.
void update();

// Tient le bras en position de départ (ARM_REST_DEG). Appelé à l'arrivée dans la douve.
void retract();

// À appeler à chaque tour pendant la défense : attaque si l'adversaire est à portée, tient 3 s, revient.
void defend(bool adversaryNear, uint32_t nowMs);

// Coupe le PWM du servo. Définitif : le bras ne bougera plus.
void release();

// Vrai pendant une attaque (bras à 120°).
bool isAttacking();

}  // namespace arm

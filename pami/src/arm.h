// Bras du défenseur : un servo SG90 qui vient toucher le PAMI adverse (D008).
//
// - Pas de PWM avant la défense : le servo reste libre, le bras est replié à la main.
// - Le bras ne bouge que pendant la période de mouvement (5 s à 14,5 s).
// - À la fin, le PWM est coupé (servo « détaché ») : le bras reste où il est, sans force.
#pragma once

#include <stdint.h>

namespace arm {

void begin();

// À appeler à chaque tour de loop(). Coupe le servo dès la fin du temps, quel que soit l'état.
void update();

// Tient le bras replié (début de la défense).
void retract();

// Déploie le bras vers l'avant.
void deploy();

// Coupe le PWM du servo. Définitif : le bras ne bougera plus.
void release();

bool isDeployed();

}  // namespace arm

// Régulateur de vitesse d'une roue (logique pure, testable sur PC).
//
// Il compare la vitesse mesurée par l'encodeur à la vitesse voulue et en déduit la PWM.
// Ainsi les deux roues tournent à la vitesse demandée, même si les deux moteurs ne sont pas
// identiques ou si la batterie baisse.
#pragma once

#include "../config.h"

class SpeedController {
public:
  // Renvoie la PWM à appliquer (-100 à +100 %).
  // targetMmS : vitesse voulue (négative = en arrière). measuredMmS : vitesse mesurée.
  float update(float targetMmS, float measuredMmS, float dtS) {
    if (targetMmS == 0.0f) {
      reset();
      return 0.0f;  // vitesse nulle demandée : frein
    }
    const float errorMmS = targetMmS - measuredMmS;

    // Intégrale : corrige un écart qui dure. Bornée pour ne pas accumuler sans fin.
    integralPct_ += SPEED_KI_PCT_PER_MM * errorMmS * dtS;
    if (integralPct_ > SPEED_INTEGRAL_MAX_PCT) {
      integralPct_ = SPEED_INTEGRAL_MAX_PCT;
    }
    if (integralPct_ < -SPEED_INTEGRAL_MAX_PCT) {
      integralPct_ = -SPEED_INTEGRAL_MAX_PCT;
    }

    // Anticipation : la PWM qui donnerait à peu près la bonne vitesse, sans attendre l'erreur.
    const float sign = (targetMmS > 0.0f) ? 1.0f : -1.0f;
    const float feedForwardPct =
        sign * MOTOR_DEADBAND_PCT + targetMmS * MOTOR_PCT_PER_MM_S;

    float pct = feedForwardPct + SPEED_KP_PCT_PER_MM_S * errorMmS + integralPct_;
    if (pct > 100.0f) {
      pct = 100.0f;
    }
    if (pct < -100.0f) {
      pct = -100.0f;
    }
    return pct;
  }

  void reset() { integralPct_ = 0.0f; }

private:
  float integralPct_ = 0.0f;
};

// Régulateur de vitesse d'une roue (logique pure, testable sur PC).
//
// Il compare la vitesse mesurée par l'encodeur à la vitesse voulue et en déduit la PWM.
// Ainsi les deux roues tournent à la vitesse demandée, même si les deux moteurs ne sont pas
// identiques ou si la batterie baisse.
#pragma once

#include "../config.h"

class SpeedController {
public:
  // deadbandPct : PWM nécessaire pour que la roue décolle. pctPerMmS : PWM par mm/s voulu.
  // Chaque roue a les siennes (config.h) ; sans argument, on prend les valeurs communes.
  explicit SpeedController(float deadbandPct = MOTOR_DEADBAND_PCT,
                           float pctPerMmS = MOTOR_PCT_PER_MM_S)
      : deadbandPct_(deadbandPct), pctPerMmS_(pctPerMmS) {}

  // Renvoie la PWM à appliquer (-100 à +100 %).
  // targetMmS : vitesse voulue (négative = en arrière). measuredMmS : vitesse mesurée.
  float update(float targetMmS, float measuredMmS, float dtS) {
    if (targetMmS == 0.0f) {
      reset();
      return 0.0f;  // vitesse nulle demandée : frein
    }
    const float errorMmS = targetMmS - measuredMmS;

    // Anticipation : la PWM qui donnerait à peu près la bonne vitesse, sans attendre l'erreur.
    const float sign = (targetMmS > 0.0f) ? 1.0f : -1.0f;
    const float feedForwardPct =
        sign * deadbandPct_ + targetMmS * pctPerMmS_;

    // Intégrale : corrige un écart qui dure. Deux protections contre l'emballement :
    // - on ne l'augmente pas quand la PWM est déjà au maximum et que l'erreur pousse encore
    //   dans ce sens (la roue ne peut pas aller plus vite : intégrer ne servirait à rien et
    //   la roue dépasserait ensuite la consigne) ;
    // - elle reste bornée à SPEED_INTEGRAL_MAX_PCT dans tous les cas.
    const float pctBefore = feedForwardPct + SPEED_KP_PCT_PER_MM_S * errorMmS + integralPct_;
    const bool saturatedAndPushing = (pctBefore >= PWM_LIMIT_PCT && errorMmS > 0.0f) ||
                                     (pctBefore <= -PWM_LIMIT_PCT && errorMmS < 0.0f);
    if (!saturatedAndPushing) {
      integralPct_ += SPEED_KI_PCT_PER_MM * errorMmS * dtS;
      if (integralPct_ > SPEED_INTEGRAL_MAX_PCT) {
        integralPct_ = SPEED_INTEGRAL_MAX_PCT;
      }
      if (integralPct_ < -SPEED_INTEGRAL_MAX_PCT) {
        integralPct_ = -SPEED_INTEGRAL_MAX_PCT;
      }
    }

    float pct = feedForwardPct + SPEED_KP_PCT_PER_MM_S * errorMmS + integralPct_;
    if (pct > PWM_LIMIT_PCT) {
      pct = PWM_LIMIT_PCT;
    }
    if (pct < -PWM_LIMIT_PCT) {
      pct = -PWM_LIMIT_PCT;
    }
    return pct;
  }

  void reset() { integralPct_ = 0.0f; }

private:
  // Limite de la PWM demandée (le driver applique la sienne : MOTOR_MAX_PWM_PCT).
  static constexpr float PWM_LIMIT_PCT = 100.0f;

  const float deadbandPct_;
  const float pctPerMmS_;
  float integralPct_ = 0.0f;
};
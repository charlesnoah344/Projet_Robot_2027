// Séquence d'attaque du bras (logique pure, testable sur PC, D021).
//
// Le bras attend à ARM_REST_DEG. Quand un adversaire est à portée :
//   1. ATTACKING : le servo va à ARM_ATTACK_DEG (120°) et y reste ARM_ATTACK_HOLD_MS (3 s) ;
//   2. RETURNING : retour à ARM_REST_DEG, ARM_RETURN_MS laissés au servo pour y arriver ;
//   3. READY : prêt pour une nouvelle attaque si l'adversaire est toujours là.
// Pendant l'attaque et le retour, ce que voit l'ultrason ne change rien : la séquence va au bout.
// Le temps est compté avec millis(), passé en paramètre : aucune attente bloquante (règle d'or n° 4).
#pragma once

#include <stdint.h>

#include "../config.h"

enum class ArmPhase { READY, ATTACKING, RETURNING };

class ArmAttack {
public:
  // À appeler à chaque tour, pendant la défense. Renvoie true quand la phase vient de changer
  // (pour écrire un message).
  bool update(bool adversaryNear, uint32_t nowMs) {
    switch (phase_) {
      case ArmPhase::READY:
        if (adversaryNear) {
          return enter(ArmPhase::ATTACKING, nowMs);
        }
        break;
      case ArmPhase::ATTACKING:
        if (nowMs - phaseStartMs_ >= ARM_ATTACK_HOLD_MS) {
          return enter(ArmPhase::RETURNING, nowMs);
        }
        break;
      case ArmPhase::RETURNING:
        if (nowMs - phaseStartMs_ >= ARM_RETURN_MS) {
          return enter(ArmPhase::READY, nowMs);
        }
        break;
    }
    return false;
  }

  // Angle que le servo doit tenir maintenant.
  float angleDeg() const { return phase_ == ArmPhase::ATTACKING ? ARM_ATTACK_DEG : ARM_REST_DEG; }

  ArmPhase phase() const { return phase_; }

  // Nombre d'attaques lancées depuis le début.
  uint16_t attackCount() const { return attackCount_; }

private:
  bool enter(ArmPhase next, uint32_t nowMs) {
    phase_ = next;
    phaseStartMs_ = nowMs;
    if (next == ArmPhase::ATTACKING) {
      attackCount_++;
    }
    return true;
  }

  ArmPhase phase_ = ArmPhase::READY;
  uint32_t phaseStartMs_ = 0;
  uint16_t attackCount_ = 0;
};

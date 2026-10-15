// Filtre de sécurité de l'ultrason : une distance ne peut pas s'allonger d'un coup
// (logique pure, testable sur PC).
//
// - Une distance qui DIMINUE est crue tout de suite : un obstacle qui approche doit faire freiner
//   sans attendre.
// - Une distance qui AUGMENTE de plus de US_RISE_THRESHOLD_MM doit être confirmée par
//   US_RISE_CONFIRM_COUNT mesures de suite.
//
// Pourquoi : le HC-SR04 rate parfois l'écho d'un obstacle tout proche. Il répond alors « pas d'écho »
// (2000 mm) ou entend le mur du fond (vers 1 m). Mesuré le 15/10/2026 avec le banc test_ultrason :
// jusqu'à 3 ratés de suite devant un PAMI immobile. Sans ce filtre, un seul écho raté faisait croire
// la voie libre, et le PAMI repartait vers l'obstacle.
//
// Au démarrage, la distance retenue vaut 0 : « obstacle » tant que rien n'est confirmé
// (sécurité par défaut).
#pragma once

#include <stdint.h>

#include "../config.h"

class RiseFilter {
public:
  // Ajoute une mesure et renvoie la distance retenue, en mm.
  uint16_t update(uint16_t measuredMm) {
    // Plus proche, ou à peine plus loin : on croit la mesure tout de suite.
    if (measuredMm <= acceptedMm_ + US_RISE_THRESHOLD_MM) {
      acceptedMm_ = measuredMm;
      pendingCount_ = 0;
      return acceptedMm_;
    }
    // Beaucoup plus loin d'un coup : on garde l'ancienne distance et on attend la confirmation.
    // Pendant l'attente, on retient la plus petite des distances vues : c'est la plus prudente.
    if (pendingCount_ == 0 || measuredMm < pendingMinMm_) {
      pendingMinMm_ = measuredMm;
    }
    pendingCount_++;
    if (pendingCount_ >= US_RISE_CONFIRM_COUNT) {
      acceptedMm_ = pendingMinMm_;
      pendingCount_ = 0;
    }
    return acceptedMm_;
  }

  uint16_t value() const { return acceptedMm_; }

private:
  uint16_t acceptedMm_ = 0;    // distance retenue
  uint16_t pendingMinMm_ = 0;  // plus petite distance vue pendant une attente de confirmation
  uint8_t pendingCount_ = 0;   // nombre de mesures « beaucoup plus loin » vues de suite
};

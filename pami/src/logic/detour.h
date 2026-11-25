// Contournement prudent d'un obstacle, par le centre de la table (logique pure, testable sur PC, D016).
//
// Le PAMI est arrêté devant un obstacle qui ne part pas. Il le contourne en mouvements séparés, et
// chaque ligne droite reste surveillée par l'évitement (la machine à états s'en charge) :
//   1. quart de tour vers le centre de la table (côté donné par chooseDetourSide, logic/avoidance.h) ;
//   2. pas de côté de DETOUR_SIDE_MM ;
//   3. quart de tour retour : le PAMI est de nouveau parallèle à sa route ;
//   4. pause de DETOUR_LOOK_MS (le PAMI et la mesure de l'ultrason se stabilisent), puis il REGARDE :
//      - voie libre sur plus de DETOUR_PASS_MM : étape 5 ;
//      - sinon : un nouveau pas de côté (étapes 1 à 3), DETOUR_MAX_SIDESTEPS au plus ;
//   5. il avance de DETOUR_PASS_MM pour dépasser l'obstacle.
// Ensuite, la mission reprend : elle vise de nouveau son point (logic/next_move.h).
//
// Pourquoi revenir parallèle et regarder avant de repartir : si le PAMI visait son point juste après
// le pas de côté, il repartirait en diagonale vers le coin de l'obstacle, sans l'avoir dépassé.
//
// Cette classe ne fait rien bouger : elle dit quoi faire. La machine à états (et le banc test_parcours)
// exécutent l'ordre avec motion, puis lui demandent l'ordre suivant quand le mouvement est fini.
#pragma once

#include <stdint.h>

#include "../config.h"

enum class DetourOrderType {
  TURN,    // tourner sur place de value degrés (positif = vers la gauche)
  DRIVE,   // avancer tout droit de value mm
  WAIT,    // ne rien faire pour l'instant (pause avant de regarder) : redemander au tour suivant
  DONE,    // contournement réussi : reprendre la mission
  FAILED   // la voie reste bloquée après DETOUR_MAX_SIDESTEPS pas de côté
};

struct DetourOrder {
  DetourOrderType type;
  float value;  // degrés (TURN) ou mm (DRIVE), 0 sinon
};

class Detour {
public:
  // Commence un contournement. sideSign : +1 pour contourner par la gauche, -1 par la droite.
  void start(float sideSign) {
    sideSign_ = sideSign;
    step_ = Step::TURN_OUT;
    sidesteps_ = 0;
  }

  // Ordre suivant. À appeler quand le mouvement précédent est fini, et à chaque tour pendant la pause.
  // frontDistanceMm : distance vue devant par l'ultrason, maintenant. nowMs : millis().
  DetourOrder next(uint16_t frontDistanceMm, uint32_t nowMs) {
    switch (step_) {
      case Step::TURN_OUT:
        step_ = Step::SIDESTEP;
        return {DetourOrderType::TURN, sideSign_ * DETOUR_TURN_DEG};

      case Step::SIDESTEP:
        sidesteps_++;
        step_ = Step::TURN_BACK;
        return {DetourOrderType::DRIVE, DETOUR_SIDE_MM};

      case Step::TURN_BACK:
        step_ = Step::LOOK;
        lookStartMs_ = nowMs;  // la pause commence quand le retour parallèle est demandé
        return {DetourOrderType::TURN, -sideSign_ * DETOUR_TURN_DEG};

      case Step::LOOK:
        // Soustraction : reste juste quand millis() repasse par zéro.
        if (nowMs - lookStartMs_ < DETOUR_LOOK_MS) {
          return {DetourOrderType::WAIT, 0.0f};
        }
        if (frontDistanceMm > DETOUR_PASS_MM) {
          step_ = Step::PASS;
          return {DetourOrderType::DRIVE, DETOUR_PASS_MM};
        }
        if (sidesteps_ < DETOUR_MAX_SIDESTEPS) {
          // Encore bloqué : un pas de côté de plus, toujours vers le centre.
          step_ = Step::SIDESTEP;
          return {DetourOrderType::TURN, sideSign_ * DETOUR_TURN_DEG};
        }
        step_ = Step::FAILED;
        return {DetourOrderType::FAILED, 0.0f};

      case Step::PASS:
        step_ = Step::SUCCEEDED;
        return {DetourOrderType::DONE, 0.0f};

      case Step::SUCCEEDED:
        return {DetourOrderType::DONE, 0.0f};

      case Step::FAILED:
        break;
    }
    return {DetourOrderType::FAILED, 0.0f};
  }

  // Nombre de pas de côté faits depuis start() (pour les messages).
  uint8_t sidesteps() const { return sidesteps_; }

  // Vrai pendant la pause avant de regarder (pour les messages).
  bool isLooking() const { return step_ == Step::LOOK; }

private:
  enum class Step { TURN_OUT, SIDESTEP, TURN_BACK, LOOK, PASS, SUCCEEDED, FAILED };

  // Sans start(), un contournement ne fait rien : il est déjà terminé (échec).
  Step step_ = Step::FAILED;
  float sideSign_ = -1.0f;
  uint8_t sidesteps_ = 0;
  uint32_t lookStartMs_ = 0;
};

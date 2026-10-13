// Choix du prochain mouvement pour une étape de mission (logique pure, testable sur PC).
//
// Principe : on ne mémorise pas « où on en était ». À chaque fois, on regarde où est le PAMI
// (odométrie) et où il doit aller, et on choisit :
// - c'est atteint -> étape suivante ;
// - mal orienté   -> tourner vers la cible ;
// - bien orienté  -> avancer de la distance restante.
// C'est ce qui rend le contournement simple : après un détour, le même calcul ramène le PAMI
// vers son point, sans code spécial.
#pragma once

#include <math.h>

#include "../config.h"
#include "mission_step.h"
#include "odometry.h"

enum class MoveAction {
  DONE,   // étape terminée
  TURN,   // tourner sur place de value degrés (positif = vers la gauche)
  DRIVE   // avancer tout droit de value mm
};

struct MoveDecision {
  MoveAction action;
  float value;
};

inline MoveDecision decideNextMove(const MissionStep& step, const Pose& pose) {
  if (step.type == StepType::GOTO) {
    const float dxMm = step.xMm - pose.xMm;
    const float dyMm = step.yMm - pose.yMm;
    const float distanceMm = sqrtf(dxMm * dxMm + dyMm * dyMm);
    if (distanceMm <= GOTO_REACHED_TOLERANCE_MM) {
      return MoveDecision{MoveAction::DONE, 0.0f};
    }
    // Direction du point vu depuis le PAMI, et écart avec le cap actuel.
    const float bearingDeg = atan2f(dyMm, dxMm) * 180.0f / 3.14159265f;
    const float headingErrorDeg = normalizeDeg(bearingDeg - pose.headingDeg);
    if (fabsf(headingErrorDeg) > HEADING_TOLERANCE_DEG) {
      return MoveDecision{MoveAction::TURN, headingErrorDeg};
    }
    return MoveDecision{MoveAction::DRIVE, distanceMm};
  }

  if (step.type == StepType::TURN_TO) {
    const float headingErrorDeg = normalizeDeg(step.headingDeg - pose.headingDeg);
    if (fabsf(headingErrorDeg) > HEADING_TOLERANCE_DEG) {
      return MoveDecision{MoveAction::TURN, headingErrorDeg};
    }
    return MoveDecision{MoveAction::DONE, 0.0f};
  }

  // DEFEND : rien à parcourir. REACH_ZONE est géré à part dans mission.cpp (il lit les capteurs).
  return MoveDecision{MoveAction::DONE, 0.0f};
}

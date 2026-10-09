// Étapes d'une mission et miroir selon la couleur (logique pure, testable sur PC).
//
// Une mission est une simple liste d'étapes, écrite UNE fois, pour le bleu (règle d'or n° 5).
// Pour le jaune, on prend l'image dans un miroir : y devient -y, et les caps changent de signe.
// Repère : départ en (0, 0), regard vers +x, y positif à gauche (voir logic/odometry.h).
#pragma once

enum class StepType {
  GOTO,     // aller au point (x, y) : se tourner vers lui, puis avancer
  TURN_TO,  // se tourner vers un cap absolu (degrés)
  DEFEND    // dernière étape du défenseur : rester sur place, bras prêt
};

struct MissionStep {
  StepType type;
  float xMm;
  float yMm;
  float headingDeg;
};

enum class TeamColor { BLUE, YELLOW };

// Petites fonctions pour écrire les missions lisiblement.
constexpr MissionStep gotoPoint(float xMm, float yMm) {
  return MissionStep{StepType::GOTO, xMm, yMm, 0.0f};
}
constexpr MissionStep turnTo(float headingDeg) {
  return MissionStep{StepType::TURN_TO, 0.0f, 0.0f, headingDeg};
}
constexpr MissionStep defend() {
  return MissionStep{StepType::DEFEND, 0.0f, 0.0f, 0.0f};
}

// L'étape telle qu'elle doit être jouée avec cette couleur.
inline MissionStep forColor(const MissionStep& blueStep, TeamColor color) {
  if (color == TeamColor::BLUE) {
    return blueStep;
  }
  MissionStep yellowStep = blueStep;
  yellowStep.yMm = -blueStep.yMm;
  yellowStep.headingDeg = -blueStep.headingDeg;
  return yellowStep;
}

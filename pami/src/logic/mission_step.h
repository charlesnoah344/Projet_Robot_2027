// Étapes d'une mission et miroir selon le côté de départ (logique pure, testable sur PC).
//
// Une mission est une simple liste d'étapes, écrite UNE fois, pour un départ à GAUCHE de la table
// (règle d'or n° 5). Pour un départ à droite, on prend l'image dans un miroir : y devient -y, et les
// caps changent de signe. C'est juste parce que les deux points de départ sont symétriques par
// rapport à l'axe central de la table.
// Repère : départ en (0, 0), regard vers +x, y positif à gauche (voir logic/odometry.h).
#pragma once

enum class StepType {
  GOTO,     // aller au point (x, y) : se tourner vers lui, puis avancer
  TURN_TO,  // se tourner vers un cap absolu (degrés)
  DEFEND    // dernière étape : rester sur place, bras prêt
};

struct MissionStep {
  StepType type;
  float xMm;
  float yMm;
  float headingDeg;
};

// Côté de la table où le PAMI est posé au départ, vu du public (règles 2027, D.2).
// Lu sur l'interrupteur de côté (module side), verrouillé au départ (D014).
enum class StartSide { LEFT, RIGHT };

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

// L'étape telle qu'elle doit être jouée depuis ce côté.
inline MissionStep forSide(const MissionStep& leftStep, StartSide side) {
  if (side == StartSide::LEFT) {
    return leftStep;
  }
  MissionStep rightStep = leftStep;
  rightStep.yMm = -leftStep.yMm;
  rightStep.headingDeg = -leftStep.headingDeg;
  return rightStep;
}

// Nom pour les messages.
inline const char* sideName(StartSide side) {
  return side == StartSide::LEFT ? "GAUCHE" : "DROITE";
}

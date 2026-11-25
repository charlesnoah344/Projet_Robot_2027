// Étapes d'une mission et miroir selon le côté de départ (logique pure, testable sur PC).
//
// Une mission est une simple liste d'étapes, écrite UNE fois, pour un PAMI GAUCHE (règle d'or n° 5).
// Pour un PAMI DROITE, on prend l'image dans un miroir : y devient -y, et les caps changent de signe.
// C'est juste parce que, vu par le PAMI, la table est la même dans les 4 écuries : seul change le côté
// où se trouve le centre (docs/douves-coordonnees.md, D017).
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

// Côté du PAMI posé dans son écurie, regard vers l'adversaire (D017) :
// - LEFT (GAUCHE) : le bord de la table est à sa gauche, le centre à sa droite ;
// - RIGHT (DROITE) : le bord est à sa droite, le centre à sa gauche.
// Ce n'est PAS le côté vu du public : par exemple, l'écurie jaune avant est DROITE (docs/materiel.md).
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

// Les missions du PAMI, écrites pour un départ à GAUCHE de la table (logique pure).
//
// Repère : départ en (0, 0), regard vers +x, y positif à gauche.
// Pour un départ à droite, chaque étape passe par forSide() (logic/mission_step.h).
// Les dimensions sont dans config.h.
#pragma once

#include <stdint.h>

#include "../config.h"
#include "mission_step.h"

// Ligne droite (grille : qualité du mouvement).
constexpr MissionStep STRAIGHT_LINE_STEPS[] = {
    gotoPoint(STRAIGHT_LINE_LENGTH_MM, 0.0f),
};

// Un tour complet sur place, en 4 quarts de tour (grille : rotation).
// On s'arrête à chaque quart : on voit ainsi la précision de chaque rotation.
constexpr MissionStep ROTATION_STEPS[] = {
    turnTo(90.0f),
    turnTo(180.0f),
    turnTo(-90.0f),
    turnTo(0.0f),
};

// Carré, puis retour au cap de départ (grille : carré).
// À l'arrivée, l'écart avec le point de départ sert à calibrer WHEEL_DIAMETER_MM et WHEEL_BASE_MM.
constexpr MissionStep SQUARE_STEPS[] = {
    gotoPoint(SQUARE_SIDE_MM, 0.0f),
    gotoPoint(SQUARE_SIDE_MM, SQUARE_SIDE_MM),
    gotoPoint(0.0f, SQUARE_SIDE_MM),
    gotoPoint(0.0f, 0.0f),
    turnTo(0.0f),
};

// Douve la plus proche (D014) : 2 étapes, « aller à la douve » puis « y rester, bras prêt ».
// Le point dépend de la douve choisie au départ (logic/nearest_moat.h) : ces étapes sont remplies
// par mission.cpp, pas écrites ici.
// Plus de capteurs de sol (D012) : le PAMI s'arrête quand l'odométrie dit qu'il est arrivé, à moins
// de GOTO_REACHED_TOLERANCE_MM du point. La précision vient de l'étalonnage (banc test_trajectoire).
constexpr uint8_t MOAT_STEP_COUNT = 2;
inline void fillMoatSteps(const MoatPoint& moat, MissionStep steps[MOAT_STEP_COUNT]) {
  steps[0] = gotoPoint(moat.xMm, moat.yMm);
  steps[1] = defend();
}

// Liste des étapes d'une mission écrite d'avance et leur nombre (0 pour MOAT, voir plus haut).
inline const MissionStep* missionSteps(MissionId id, uint8_t& count) {
  switch (id) {
    case MissionId::STRAIGHT_LINE:
      count = sizeof(STRAIGHT_LINE_STEPS) / sizeof(MissionStep);
      return STRAIGHT_LINE_STEPS;
    case MissionId::ROTATION:
      count = sizeof(ROTATION_STEPS) / sizeof(MissionStep);
      return ROTATION_STEPS;
    case MissionId::SQUARE:
      count = sizeof(SQUARE_STEPS) / sizeof(MissionStep);
      return SQUARE_STEPS;
    case MissionId::MOAT:
      break;  // dépend de la douve choisie au départ : remplie par mission.cpp (fillMoatSteps)
  }
  count = 0;
  return nullptr;
}

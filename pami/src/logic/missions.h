// Les missions du PAMI, écrites pour un PAMI GAUCHE : bord de table à sa gauche, centre à sa droite
// (logique pure, D017).
//
// Repère : départ en (0, 0), regard vers +x, y positif à gauche.
// Pour un PAMI DROITE, chaque étape passe par forSide() (logic/mission_step.h).
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

// Convertit un point du repère de la table (config.h, docs/douves-coordonnees.md) dans le repère de
// l'odométrie, pour un départ à GAUCHE :
//   - x : on retire la position de départ ;
//   - y : à GAUCHE, le centre de la table (y de la table qui grandit) est à la DROITE du PAMI,
//     donc du côté des y négatifs de l'odométrie.
// Pour DROITE, forSide() change ensuite le signe de y.
constexpr MissionStep gotoTablePoint(TablePoint point) {
  return gotoPoint(point.xMm - START_POINT_MM.xMm, -(point.yMm - START_POINT_MM.yMm));
}

// Douve de son côté (D017) : ligne droite jusqu'au point de virage, virage vers le centre, entrée dans
// la douve, puis « y rester, bras prêt ». Pour GAUCHE : (1900, 0), puis (2079, -219).
// Plus de capteurs de sol (D012) : le PAMI s'arrête quand l'odométrie dit qu'il est arrivé, à moins
// de GOTO_REACHED_TOLERANCE_MM du point. La précision vient de l'étalonnage (banc test_trajectoire).
constexpr MissionStep MOAT_STEPS[] = {
    gotoTablePoint(MOAT_WAYPOINT_MM),
    gotoTablePoint(MOAT_NEAR_MM),
    defend(),
};

// Liste des étapes d'une mission et leur nombre.
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
      count = sizeof(MOAT_STEPS) / sizeof(MissionStep);
      return MOAT_STEPS;
  }
  count = 0;
  return nullptr;
}

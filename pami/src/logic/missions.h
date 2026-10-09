// Les missions du PAMI, écrites pour le bleu (logique pure).
//
// Repère : départ en (0, 0), regard vers +x, y positif à gauche.
// Pour le jaune, chaque étape passe par forColor() (logic/mission_step.h).
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

// Défenseur : aller dans la zone, regarder vers l'adversaire, puis défendre.
constexpr MissionStep DEFENDER_STEPS[] = {
    gotoPoint(DEFENDER_ZONE_X_MM, DEFENDER_ZONE_Y_MM),
    turnTo(DEFENDER_FACING_DEG),
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
    case MissionId::DEFENDER:
      count = sizeof(DEFENDER_STEPS) / sizeof(MissionStep);
      return DEFENDER_STEPS;
  }
  count = 0;
  return nullptr;
}

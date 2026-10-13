// Odométrie : position du PAMI calculée à partir des ticks des roues (logique pure, testable sur PC).
//
// Repère : le PAMI part en (0, 0), regard vers +x. y positif à sa gauche.
// Cap en degrés : 0 = direction de départ, positif = tourné vers la gauche.
//
// Limite : si une roue glisse, l'erreur s'accumule. D'où la calibration avec le carré (étape E).
#pragma once

#include <stdint.h>

struct Pose {
  float xMm = 0.0f;
  float yMm = 0.0f;
  float headingDeg = 0.0f;  // entre -180 et +180
};

class Odometry {
public:
  // Met à jour la position avec les compteurs totaux des deux roues.
  // Le premier appel sert seulement de point de départ.
  void update(int32_t leftTicks, int32_t rightTicks);

  Pose pose() const;

private:
  bool hasTicks_ = false;
  int32_t lastLeftTicks_ = 0;
  int32_t lastRightTicks_ = 0;
  float xMm_ = 0.0f;
  float yMm_ = 0.0f;
  float headingRad_ = 0.0f;
};

// Ramène un angle entre -180 et +180 degrés (ex. 270 -> -90).
// Sert à tourner toujours du côté le plus court.
float normalizeDeg(float angleDeg);

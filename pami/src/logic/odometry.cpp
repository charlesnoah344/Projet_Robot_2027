#include "odometry.h"

#include <math.h>

#include "../config.h"

namespace {

constexpr float PI_F = 3.14159265f;

}  // namespace

void Odometry::update(int32_t leftTicks, int32_t rightTicks) {
  if (!hasTicks_) {
    hasTicks_ = true;
    lastLeftTicks_ = leftTicks;
    lastRightTicks_ = rightTicks;
    return;
  }
  // Distance parcourue par chaque roue depuis le dernier appel.
  const float leftMm = (leftTicks - lastLeftTicks_) * MM_PER_TICK;
  const float rightMm = (rightTicks - lastRightTicks_) * MM_PER_TICK;
  lastLeftTicks_ = leftTicks;
  lastRightTicks_ = rightTicks;

  // Le centre du PAMI avance de la moyenne des deux roues.
  // Il tourne de la différence divisée par l'entraxe (en radians).
  const float centerMm = (leftMm + rightMm) / 2.0f;
  const float turnRad = (rightMm - leftMm) / WHEEL_BASE_MM;

  // On avance selon le cap du milieu du petit déplacement : plus précis dans les courbes.
  const float midHeadingRad = headingRad_ + turnRad / 2.0f;
  xMm_ += centerMm * cosf(midHeadingRad);
  yMm_ += centerMm * sinf(midHeadingRad);
  headingRad_ += turnRad;

  // Cap gardé entre -pi et +pi.
  if (headingRad_ > PI_F) {
    headingRad_ -= 2.0f * PI_F;
  } else if (headingRad_ < -PI_F) {
    headingRad_ += 2.0f * PI_F;
  }
}

Pose Odometry::pose() const {
  Pose result;
  result.xMm = xMm_;
  result.yMm = yMm_;
  result.headingDeg = headingRad_ * 180.0f / PI_F;
  return result;
}

float normalizeDeg(float angleDeg) {
  while (angleDeg > 180.0f) {
    angleDeg -= 360.0f;
  }
  while (angleDeg <= -180.0f) {
    angleDeg += 360.0f;
  }
  return angleDeg;
}

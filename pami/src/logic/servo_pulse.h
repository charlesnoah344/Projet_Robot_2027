// Conversion d'un angle de servo en rapport cyclique pour le générateur PWM de l'ESP32 (logique pure).
// Partagée par le module arm (match) et le banc test_bras : le banc teste donc le vrai calcul.
#pragma once

#include <stdint.h>

#include "../config.h"

// Un servo lit la largeur de l'impulsion : SERVO_MIN_PULSE_US pour 0°, SERVO_MAX_PULSE_US pour 180°.
// On convertit l'angle en largeur d'impulsion (µs), puis en fraction de la période de 20 ms.
// Angle hors de 0..180° : ramené dans les limites, pour ne jamais forcer le servo en butée.
inline uint32_t servoDutyForAngle(float angleDeg) {
  if (angleDeg < 0.0f) {
    angleDeg = 0.0f;
  } else if (angleDeg > 180.0f) {
    angleDeg = 180.0f;
  }
  const float pulseUs = SERVO_MIN_PULSE_US + angleDeg / 180.0f * (SERVO_MAX_PULSE_US - SERVO_MIN_PULSE_US);
  const float periodUs = 1000000.0f / SERVO_PWM_FREQ_HZ;
  const uint32_t maxDuty = (1UL << SERVO_PWM_RESOLUTION_BITS) - 1;
  return (uint32_t)(pulseUs / periodUs * maxDuty + 0.5f);
}

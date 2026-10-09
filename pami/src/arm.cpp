#include "arm.h"

#include <Arduino.h>

#include "config.h"
#include "match_timer.h"

namespace arm {

namespace {

bool attached = false;
bool released = false;
bool deployed = false;

// Rapport cyclique pour un angle : on convertit l'angle en largeur d'impulsion (µs),
// puis en fraction de la période de 20 ms.
uint32_t dutyForAngle(float angleDeg) {
  const float pulseUs =
      SERVO_MIN_PULSE_US + angleDeg / 180.0f * (SERVO_MAX_PULSE_US - SERVO_MIN_PULSE_US);
  const float periodUs = 1000000.0f / SERVO_PWM_FREQ_HZ;
  const uint32_t maxDuty = (1UL << SERVO_PWM_RESOLUTION_BITS) - 1;
  return (uint32_t)(pulseUs / periodUs * maxDuty + 0.5f);
}

void moveTo(float angleDeg) {
  // Le bras ne bouge que pendant la période de mouvement, et jamais après release().
  if (released || !matchTimer::isMoveAllowed()) {
    return;
  }
  if (!attached) {
    ledcSetup(LEDC_CHANNEL_SERVO, SERVO_PWM_FREQ_HZ, SERVO_PWM_RESOLUTION_BITS);
    ledcAttachPin(PIN_SERVO, LEDC_CHANNEL_SERVO);
    attached = true;
  }
  ledcWrite(LEDC_CHANNEL_SERVO, dutyForAngle(angleDeg));
}

}  // namespace

void begin() {
  // Pas de PWM au démarrage : le servo ne bouge pas avant la défense.
  pinMode(PIN_SERVO, OUTPUT);
  digitalWrite(PIN_SERVO, LOW);
}

void update() {
  // Double sécurité : même si la machine à états oubliait, le servo est coupé à la fin du temps.
  if (matchTimer::isOver()) {
    release();
  }
}

void retract() {
  moveTo(ARM_RETRACTED_DEG);
  deployed = false;
}

void deploy() {
  if (deployed) {
    return;
  }
  moveTo(ARM_DEPLOYED_DEG);
  deployed = attached && !released;
  if (DEBUG_LOGS && deployed) {
    Serial.println("[ARM] Bras deploye.");
  }
}

void release() {
  if (released) {
    return;
  }
  released = true;
  if (attached) {
    ledcDetachPin(PIN_SERVO);
    attached = false;
  }
  pinMode(PIN_SERVO, OUTPUT);
  digitalWrite(PIN_SERVO, LOW);
  if (DEBUG_LOGS) {
    Serial.println("[ARM] Servo coupe.");
  }
}

bool isDeployed() {
  return deployed;
}

}  // namespace arm

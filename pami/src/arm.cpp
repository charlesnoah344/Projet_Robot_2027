#include "arm.h"

#include <Arduino.h>

#include "config.h"
#include "console.h"
#include "logic/arm_attack.h"
#include "logic/servo_pulse.h"
#include "match_timer.h"

namespace arm {

namespace {

bool attached = false;  // le servo est alimenté et reçoit du PWM (il peut bouger)
bool released = false;  // le servo est coupé à la fin
ArmAttack attack;       // séquence attaque / tenue / retour (D021)

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
  ledcWrite(LEDC_CHANNEL_SERVO, servoDutyForAngle(angleDeg));
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
  moveTo(ARM_REST_DEG);
}

void defend(bool adversaryNear, uint32_t nowMs) {
  if (released) {
    return;
  }
  const bool changed = attack.update(adversaryNear, nowMs);
  moveTo(attack.angleDeg());
  if (!DEBUG_LOGS || !changed) {
    return;
  }
  switch (attack.phase()) {
    case ArmPhase::ATTACKING:
      Console.printf("[ARM] Attaque %u : bras a %.0f deg pendant %.1f s\n", attack.attackCount(), ARM_ATTACK_DEG,
                     ARM_ATTACK_HOLD_MS / 1000.0f);
      break;
    case ArmPhase::RETURNING:
      Console.printf("[ARM] Retour a %.0f deg\n", ARM_REST_DEG);
      break;
    case ArmPhase::READY:
      Console.println("[ARM] Pret pour une nouvelle attaque");
      break;
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
    Console.printf("[ARM] Servo coupe (bras %s).\n", attack.phase() == ArmPhase::ATTACKING ? "en attaque" : "au repos");
  }
}

bool isAttacking() {
  return attack.phase() == ArmPhase::ATTACKING;
}

}  // namespace arm

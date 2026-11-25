// Tests de la séquence d'attaque du bras (D021) et du calcul de l'impulsion du servo.
// Sur PC : pio test -e native. Sur l'ESP32 branché en USB : pio test -e logic_esp32.
#include <unity.h>

#include "logic/arm_attack.h"
#include "logic/servo_pulse.h"

void setUp() {}
void tearDown() {}

// ---------- Séquence d'attaque ----------

// Personne en face : le bras reste au repos.
void test_rest_without_adversary() {
  ArmAttack attack;
  TEST_ASSERT_FALSE(attack.update(false, 1000));
  TEST_ASSERT_EQUAL(ArmPhase::READY, attack.phase());
  TEST_ASSERT_EQUAL_FLOAT(ARM_REST_DEG, attack.angleDeg());
}

// Adversaire à portée : attaque à 120°, tenue 3 s, retour, puis prêt.
void test_attack_hold_and_return() {
  ArmAttack attack;
  TEST_ASSERT_TRUE(attack.update(true, 1000));
  TEST_ASSERT_EQUAL(ArmPhase::ATTACKING, attack.phase());
  TEST_ASSERT_EQUAL_FLOAT(ARM_ATTACK_DEG, attack.angleDeg());
  TEST_ASSERT_EQUAL_UINT16(1, attack.attackCount());

  // Juste avant la fin de la tenue : toujours en attaque.
  TEST_ASSERT_FALSE(attack.update(true, 1000 + ARM_ATTACK_HOLD_MS - 1));
  TEST_ASSERT_EQUAL_FLOAT(ARM_ATTACK_DEG, attack.angleDeg());

  // Fin de la tenue : retour au repos.
  TEST_ASSERT_TRUE(attack.update(true, 1000 + ARM_ATTACK_HOLD_MS));
  TEST_ASSERT_EQUAL(ArmPhase::RETURNING, attack.phase());
  TEST_ASSERT_EQUAL_FLOAT(ARM_REST_DEG, attack.angleDeg());

  // Le servo a eu le temps de revenir : prêt.
  TEST_ASSERT_TRUE(attack.update(false, 1000 + ARM_ATTACK_HOLD_MS + ARM_RETURN_MS));
  TEST_ASSERT_EQUAL(ArmPhase::READY, attack.phase());
}

// L'adversaire qui part pendant l'attaque ne l'écourte pas : la séquence va au bout.
void test_attack_goes_to_the_end() {
  ArmAttack attack;
  attack.update(true, 0);
  TEST_ASSERT_FALSE(attack.update(false, ARM_ATTACK_HOLD_MS / 2));
  TEST_ASSERT_EQUAL(ArmPhase::ATTACKING, attack.phase());
}

// Pendant le retour, un adversaire vu ne relance pas l'attaque : il faut attendre d'être prêt.
void test_no_new_attack_while_returning() {
  ArmAttack attack;
  attack.update(true, 0);
  attack.update(true, ARM_ATTACK_HOLD_MS);  // retour
  TEST_ASSERT_FALSE(attack.update(true, ARM_ATTACK_HOLD_MS + ARM_RETURN_MS - 1));
  TEST_ASSERT_EQUAL(ArmPhase::RETURNING, attack.phase());
  TEST_ASSERT_EQUAL_UINT16(1, attack.attackCount());
}

// Adversaire toujours là une fois le bras revenu : nouvelle attaque.
void test_new_attack_when_ready_again() {
  ArmAttack attack;
  attack.update(true, 0);
  attack.update(true, ARM_ATTACK_HOLD_MS);
  attack.update(true, ARM_ATTACK_HOLD_MS + ARM_RETURN_MS);  // prêt
  TEST_ASSERT_TRUE(attack.update(true, ARM_ATTACK_HOLD_MS + ARM_RETURN_MS + 20));
  TEST_ASSERT_EQUAL(ArmPhase::ATTACKING, attack.phase());
  TEST_ASSERT_EQUAL_UINT16(2, attack.attackCount());
}

// Le temps passe par zéro (millis() déborde après 49 jours) : la tenue dure toujours 3 s.
void test_hold_across_millis_overflow() {
  ArmAttack attack;
  const uint32_t start = 0xFFFFFFFFUL - 1000UL;
  attack.update(true, start);
  TEST_ASSERT_FALSE(attack.update(true, start + ARM_ATTACK_HOLD_MS - 1));
  TEST_ASSERT_TRUE(attack.update(true, start + ARM_ATTACK_HOLD_MS));
}

// ---------- Impulsion du servo ----------

// 0° -> SERVO_MIN_PULSE_US, 180° -> SERVO_MAX_PULSE_US, sur une période de 20 ms.
void test_servo_duty_at_limits() {
  const float maxDuty = (float)((1UL << SERVO_PWM_RESOLUTION_BITS) - 1);
  const float periodUs = 1000000.0f / SERVO_PWM_FREQ_HZ;
  TEST_ASSERT_UINT32_WITHIN(1, (uint32_t)(SERVO_MIN_PULSE_US / periodUs * maxDuty + 0.5f), servoDutyForAngle(0.0f));
  TEST_ASSERT_UINT32_WITHIN(1, (uint32_t)(SERVO_MAX_PULSE_US / periodUs * maxDuty + 0.5f), servoDutyForAngle(180.0f));
}

// Plus l'angle est grand, plus l'impulsion est longue ; hors limites, on reste en butée.
void test_servo_duty_order_and_clamp() {
  TEST_ASSERT_TRUE(servoDutyForAngle(ARM_ATTACK_DEG) > servoDutyForAngle(ARM_REST_DEG));
  TEST_ASSERT_EQUAL_UINT32(servoDutyForAngle(0.0f), servoDutyForAngle(-30.0f));
  TEST_ASSERT_EQUAL_UINT32(servoDutyForAngle(180.0f), servoDutyForAngle(250.0f));
}

int runAllTests() {
  UNITY_BEGIN();
  RUN_TEST(test_rest_without_adversary);
  RUN_TEST(test_attack_hold_and_return);
  RUN_TEST(test_attack_goes_to_the_end);
  RUN_TEST(test_no_new_attack_while_returning);
  RUN_TEST(test_new_attack_when_ready_again);
  RUN_TEST(test_hold_across_millis_overflow);
  RUN_TEST(test_servo_duty_at_limits);
  RUN_TEST(test_servo_duty_order_and_clamp);
  return UNITY_END();
}

#ifdef ARDUINO
#include <Arduino.h>
void setup() {
  delay(2000);  // programme de test uniquement : laisse le temps au moniteur série de se connecter
  runAllTests();
}
void loop() {}
#else
int main() {
  return runAllTests();
}
#endif

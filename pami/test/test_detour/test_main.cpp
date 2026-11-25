// Tests du contournement par le centre (logic/detour.h, D016).
// Sur PC : pio test -e native. Sur l'ESP32 branché en USB : pio test -e logic_esp32.
// Les valeurs attendues viennent de config.h : les tests restent justes si on règle les constantes.
#include <unity.h>

#include "logic/detour.h"

void setUp() {}
void tearDown() {}

constexpr uint16_t CLEAR_MM = 2000;    // rien devant
constexpr uint16_t BLOCKED_MM = 200;   // obstacle encore devant, à moins de DETOUR_PASS_MM
constexpr float RIGHT = -1.0f;          // départ à gauche : contournement par la droite

void assertOrder(DetourOrderType type, float value, const DetourOrder& order) {
  TEST_ASSERT_EQUAL(type, order.type);
  TEST_ASSERT_EQUAL_FLOAT(value, order.value);
}

// Fait les 3 premiers mouvements d'un pas de côté (quart de tour, pas, quart de tour retour).
// Renvoie l'instant où la pause de regard commence.
uint32_t doSidestep(Detour& detour, float side, uint32_t nowMs) {
  assertOrder(DetourOrderType::TURN, side * DETOUR_TURN_DEG, detour.next(CLEAR_MM, nowMs));
  assertOrder(DetourOrderType::DRIVE, DETOUR_SIDE_MM, detour.next(CLEAR_MM, nowMs + 1000));
  assertOrder(DetourOrderType::TURN, -side * DETOUR_TURN_DEG, detour.next(CLEAR_MM, nowMs + 2500));
  return nowMs + 2500;
}

// Cas normal : un pas de côté suffit, le PAMI dépasse l'obstacle, puis reprend sa mission.
void test_normal_sequence() {
  Detour detour;
  detour.start(RIGHT);
  const uint32_t lookStart = doSidestep(detour, RIGHT, 0);
  TEST_ASSERT_TRUE(detour.isLooking());
  assertOrder(DetourOrderType::DRIVE, DETOUR_PASS_MM, detour.next(CLEAR_MM, lookStart + DETOUR_LOOK_MS));
  assertOrder(DetourOrderType::DONE, 0.0f, detour.next(CLEAR_MM, lookStart + 5000));
  TEST_ASSERT_EQUAL_UINT8(1, detour.sidesteps());
}

// Le PAMI ne regarde pas avant la fin de la pause, même si la voie est libre.
void test_look_waits_for_pause() {
  Detour detour;
  detour.start(RIGHT);
  const uint32_t lookStart = doSidestep(detour, RIGHT, 0);
  assertOrder(DetourOrderType::WAIT, 0.0f, detour.next(CLEAR_MM, lookStart));
  assertOrder(DetourOrderType::WAIT, 0.0f, detour.next(CLEAR_MM, lookStart + DETOUR_LOOK_MS - 1));
  assertOrder(DetourOrderType::DRIVE, DETOUR_PASS_MM, detour.next(CLEAR_MM, lookStart + DETOUR_LOOK_MS));
}

// Voie encore bloquée au regard : un deuxième pas de côté, toujours vers le centre, puis dépassement.
void test_second_sidestep_when_still_blocked() {
  Detour detour;
  detour.start(RIGHT);
  uint32_t lookStart = doSidestep(detour, RIGHT, 0);
  // Encore bloqué : nouveau quart de tour vers le centre (pas de dépassement).
  assertOrder(DetourOrderType::TURN, RIGHT * DETOUR_TURN_DEG, detour.next(BLOCKED_MM, lookStart + DETOUR_LOOK_MS));
  assertOrder(DetourOrderType::DRIVE, DETOUR_SIDE_MM, detour.next(CLEAR_MM, lookStart + 2000));
  assertOrder(DetourOrderType::TURN, -RIGHT * DETOUR_TURN_DEG, detour.next(CLEAR_MM, lookStart + 3500));
  lookStart += 3500;
  assertOrder(DetourOrderType::DRIVE, DETOUR_PASS_MM, detour.next(CLEAR_MM, lookStart + DETOUR_LOOK_MS));
  assertOrder(DetourOrderType::DONE, 0.0f, detour.next(CLEAR_MM, lookStart + 5000));
  TEST_ASSERT_EQUAL_UINT8(2, detour.sidesteps());
}

// Toujours bloqué après DETOUR_MAX_SIDESTEPS pas de côté : échec, et l'échec reste.
void test_fails_after_max_sidesteps() {
  Detour detour;
  detour.start(RIGHT);
  uint32_t nowMs = 0;
  for (uint8_t i = 0; i < DETOUR_MAX_SIDESTEPS; i++) {
    if (i == 0) {
      nowMs = doSidestep(detour, RIGHT, nowMs);
    } else {
      // Le quart de tour vers le centre a déjà été donné au regard précédent.
      assertOrder(DetourOrderType::DRIVE, DETOUR_SIDE_MM, detour.next(CLEAR_MM, nowMs + 1000));
      assertOrder(DetourOrderType::TURN, -RIGHT * DETOUR_TURN_DEG, detour.next(CLEAR_MM, nowMs + 2500));
      nowMs += 2500;
    }
    const DetourOrder order = detour.next(BLOCKED_MM, nowMs + DETOUR_LOOK_MS);
    if (i + 1 < DETOUR_MAX_SIDESTEPS) {
      assertOrder(DetourOrderType::TURN, RIGHT * DETOUR_TURN_DEG, order);
    } else {
      assertOrder(DetourOrderType::FAILED, 0.0f, order);
    }
  }
  TEST_ASSERT_EQUAL_UINT8(DETOUR_MAX_SIDESTEPS, detour.sidesteps());
  assertOrder(DetourOrderType::FAILED, 0.0f, detour.next(CLEAR_MM, nowMs + 10000));
}

// Voie libre pile à DETOUR_PASS_MM : pas assez, il faut PLUS que la distance de dépassement.
void test_pass_needs_more_than_pass_distance() {
  Detour detour;
  detour.start(RIGHT);
  const uint32_t lookStart = doSidestep(detour, RIGHT, 0);
  const DetourOrder order = detour.next((uint16_t)DETOUR_PASS_MM, lookStart + DETOUR_LOOK_MS);
  TEST_ASSERT_EQUAL(DetourOrderType::TURN, order.type);
}

// Départ à droite de la table : le contournement se fait par la gauche (angles positifs d'abord).
void test_left_side_mirror() {
  Detour detour;
  detour.start(1.0f);
  const uint32_t lookStart = doSidestep(detour, 1.0f, 0);
  assertOrder(DetourOrderType::DRIVE, DETOUR_PASS_MM, detour.next(CLEAR_MM, lookStart + DETOUR_LOOK_MS));
}

// La pause marche aussi quand millis() repasse par zéro.
void test_look_pause_across_millis_overflow() {
  Detour detour;
  detour.start(RIGHT);
  const uint32_t lookStart = doSidestep(detour, RIGHT, 0xFFFFF000UL);  // regard vers 0xFFFFF9C4
  assertOrder(DetourOrderType::WAIT, 0.0f, detour.next(CLEAR_MM, lookStart + DETOUR_LOOK_MS - 1));
  assertOrder(DetourOrderType::DRIVE, DETOUR_PASS_MM, detour.next(CLEAR_MM, lookStart + DETOUR_LOOK_MS + 5000));
}

// Sans start(), un contournement ne fait rien bouger.
void test_not_started_does_nothing() {
  Detour detour;
  TEST_ASSERT_EQUAL(DetourOrderType::FAILED, detour.next(CLEAR_MM, 0).type);
}

int runAllTests() {
  UNITY_BEGIN();
  RUN_TEST(test_normal_sequence);
  RUN_TEST(test_look_waits_for_pause);
  RUN_TEST(test_second_sidestep_when_still_blocked);
  RUN_TEST(test_fails_after_max_sidesteps);
  RUN_TEST(test_pass_needs_more_than_pass_distance);
  RUN_TEST(test_left_side_mirror);
  RUN_TEST(test_look_pause_across_millis_overflow);
  RUN_TEST(test_not_started_does_nothing);
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

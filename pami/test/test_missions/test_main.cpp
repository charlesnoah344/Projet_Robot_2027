// Tests de la logique de mission (D014) : douve la plus proche, miroir gauche / droite,
// côté du détour.
// Sur PC : pio test -e native. Sur l'ESP32 branché en USB : pio test -e logic_esp32.
#include <unity.h>

#include "logic/avoidance.h"
#include "logic/mission_step.h"
#include "logic/missions.h"
#include "logic/nearest_moat.h"

void setUp() {}
void tearDown() {}

// ---------- Douve la plus proche ----------

void test_nearest_moat_is_not_always_the_first() {
  const MoatPoint moats[] = {{1200.0f, 300.0f}, {600.0f, -200.0f}, {900.0f, 0.0f}};
  TEST_ASSERT_EQUAL_UINT8(1, nearestMoatIndex(moats, 3, 0.0f, 0.0f));
}

// Le côté (y positif ou négatif) ne change pas la distance : c'est ce qui permet de choisir
// sur les positions écrites pour la gauche, même pour un départ à droite.
void test_nearest_moat_same_for_both_sides() {
  const MoatPoint left[] = {{800.0f, 500.0f}, {700.0f, -100.0f}, {1000.0f, 0.0f}};
  const MoatPoint right[] = {{800.0f, -500.0f}, {700.0f, 100.0f}, {1000.0f, 0.0f}};
  TEST_ASSERT_EQUAL_UINT8(nearestMoatIndex(left, 3, 0.0f, 0.0f),
                          nearestMoatIndex(right, 3, 0.0f, 0.0f));
}

// Égalité : la première de la liste.
void test_nearest_moat_tie_keeps_first() {
  const MoatPoint moats[] = {{500.0f, 300.0f}, {500.0f, -300.0f}};
  TEST_ASSERT_EQUAL_UINT8(0, nearestMoatIndex(moats, 2, 0.0f, 0.0f));
}

// Depuis un autre point que le départ, la plus proche change.
void test_nearest_moat_from_another_point() {
  const MoatPoint moats[] = {{0.0f, 1000.0f}, {1000.0f, 0.0f}};
  TEST_ASSERT_EQUAL_UINT8(0, nearestMoatIndex(moats, 2, 0.0f, 900.0f));
  TEST_ASSERT_EQUAL_UINT8(1, nearestMoatIndex(moats, 2, 900.0f, 0.0f));
}

// Les positions de config.h : il y en a bien 3, et le choix tombe dans la liste.
void test_config_moats() {
  TEST_ASSERT_EQUAL_UINT8(3, MOAT_COUNT);
  TEST_ASSERT_TRUE(nearestMoatIndex(MOATS_MM, MOAT_COUNT, 0.0f, 0.0f) < MOAT_COUNT);
}

void test_fill_moat_steps() {
  MissionStep steps[MOAT_STEP_COUNT];
  fillMoatSteps(MoatPoint{600.0f, -200.0f}, steps);
  TEST_ASSERT_EQUAL(StepType::GOTO, steps[0].type);
  TEST_ASSERT_EQUAL_FLOAT(600.0f, steps[0].xMm);
  TEST_ASSERT_EQUAL_FLOAT(-200.0f, steps[0].yMm);
  TEST_ASSERT_EQUAL(StepType::DEFEND, steps[1].type);
}

// ---------- Miroir gauche / droite ----------

void test_left_side_keeps_step() {
  const MissionStep step = forSide(gotoPoint(600.0f, -200.0f), StartSide::LEFT);
  TEST_ASSERT_EQUAL_FLOAT(600.0f, step.xMm);
  TEST_ASSERT_EQUAL_FLOAT(-200.0f, step.yMm);
}

void test_right_side_mirrors_step() {
  const MissionStep point = forSide(gotoPoint(600.0f, -200.0f), StartSide::RIGHT);
  TEST_ASSERT_EQUAL_FLOAT(600.0f, point.xMm);  // devant reste devant
  TEST_ASSERT_EQUAL_FLOAT(200.0f, point.yMm);  // droite devient gauche
  const MissionStep heading = forSide(turnTo(45.0f), StartSide::RIGHT);
  TEST_ASSERT_EQUAL_FLOAT(-45.0f, heading.headingDeg);
}

// ---------- Détour vers le centre ----------

// Parti de la gauche de la table, le centre est à droite du PAMI (-1) ; parti de la droite, à gauche (+1).
void test_detour_goes_toward_center() {
  TEST_ASSERT_EQUAL_FLOAT(-1.0f, chooseDetourSide(StartSide::LEFT));
  TEST_ASSERT_EQUAL_FLOAT(1.0f, chooseDetourSide(StartSide::RIGHT));
}

int runAllTests() {
  UNITY_BEGIN();
  RUN_TEST(test_nearest_moat_is_not_always_the_first);
  RUN_TEST(test_nearest_moat_same_for_both_sides);
  RUN_TEST(test_nearest_moat_tie_keeps_first);
  RUN_TEST(test_nearest_moat_from_another_point);
  RUN_TEST(test_config_moats);
  RUN_TEST(test_fill_moat_steps);
  RUN_TEST(test_left_side_keeps_step);
  RUN_TEST(test_right_side_mirrors_step);
  RUN_TEST(test_detour_goes_toward_center);
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

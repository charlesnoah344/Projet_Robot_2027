// Tests de la logique de mission (D014, D017) : trajet de la douve assignée, miroir gauche / droite,
// côté du détour.
// Sur PC : pio test -e native. Sur l'ESP32 branché en USB : pio test -e logic_esp32.
#include <unity.h>

#include "logic/avoidance.h"
#include "logic/mission_step.h"
#include "logic/missions.h"
#include "logic/next_move.h"

void setUp() {}
void tearDown() {}

// ---------- Douve assignée (D017) ----------

// Conversion du repère de la table : le point de départ devient (0, 0).
void test_table_point_start_is_origin() {
  const MissionStep step = gotoTablePoint(START_POINT_MM);
  TEST_ASSERT_EQUAL_FLOAT(0.0f, step.xMm);
  TEST_ASSERT_EQUAL_FLOAT(0.0f, step.yMm);
}

// Pour GAUCHE, le centre de la table (y de la table qui grandit) est à droite du PAMI : y négatif.
void test_table_point_toward_center_is_right_for_left() {
  const MissionStep step = gotoTablePoint(TablePoint{START_POINT_MM.xMm + 500.0f, START_POINT_MM.yMm + 100.0f});
  TEST_ASSERT_EQUAL_FLOAT(500.0f, step.xMm);
  TEST_ASSERT_EQUAL_FLOAT(-100.0f, step.yMm);
}

// Avec le départ provisoire (100, 210) : point de virage (1900, 0), douve (2079, -219), puis DEFEND.
void test_moat_steps_for_left() {
  uint8_t count = 0;
  const MissionStep* steps = missionSteps(MissionId::MOAT, count);
  TEST_ASSERT_EQUAL_UINT8(3, count);
  TEST_ASSERT_EQUAL(StepType::GOTO, steps[0].type);
  TEST_ASSERT_EQUAL_FLOAT(MOAT_WAYPOINT_MM.xMm - START_POINT_MM.xMm, steps[0].xMm);
  TEST_ASSERT_EQUAL_FLOAT(-(MOAT_WAYPOINT_MM.yMm - START_POINT_MM.yMm), steps[0].yMm);
  TEST_ASSERT_EQUAL(StepType::GOTO, steps[1].type);
  TEST_ASSERT_EQUAL_FLOAT(MOAT_NEAR_MM.xMm - START_POINT_MM.xMm, steps[1].xMm);
  TEST_ASSERT_EQUAL_FLOAT(-(MOAT_NEAR_MM.yMm - START_POINT_MM.yMm), steps[1].yMm);
  TEST_ASSERT_EQUAL(StepType::DEFEND, steps[2].type);
}

// Pour DROITE, la même douve de son côté, en miroir : le virage se fait vers la gauche.
void test_moat_steps_for_right() {
  const MissionStep waypoint = forSide(MOAT_STEPS[0], StartSide::RIGHT);
  const MissionStep moat = forSide(MOAT_STEPS[1], StartSide::RIGHT);
  TEST_ASSERT_EQUAL_FLOAT(MOAT_STEPS[0].xMm, waypoint.xMm);
  TEST_ASSERT_EQUAL_FLOAT(-MOAT_STEPS[0].yMm, waypoint.yMm);
  TEST_ASSERT_EQUAL_FLOAT(MOAT_STEPS[1].xMm, moat.xMm);
  TEST_ASSERT_TRUE(moat.yMm > 0.0f);  // vers la gauche du PAMI, donc vers le centre
}

// Depuis le départ, cap 0 : la première décision est d'avancer tout droit jusqu'au point de virage.
void test_moat_first_leg_is_straight() {
  const Pose start = Pose();
  const MoveDecision next = decideNextMove(MOAT_STEPS[0], start);
  TEST_ASSERT_EQUAL(MoveAction::DRIVE, next.action);
  TEST_ASSERT_FLOAT_WITHIN(0.5f, MOAT_WAYPOINT_MM.xMm - START_POINT_MM.xMm, next.value);
}

// Au point de virage, cap 0 : tourner vers le centre (à droite pour GAUCHE), puis environ 283 mm.
void test_moat_second_leg_turns_toward_center() {
  Pose atWaypoint;
  atWaypoint.xMm = MOAT_STEPS[0].xMm;
  atWaypoint.yMm = MOAT_STEPS[0].yMm;
  const MoveDecision turn = decideNextMove(MOAT_STEPS[1], atWaypoint);
  TEST_ASSERT_EQUAL(MoveAction::TURN, turn.action);
  TEST_ASSERT_FLOAT_WITHIN(0.5f, -50.7f, turn.value);

  Pose turned = atWaypoint;
  turned.headingDeg = turn.value;
  const MoveDecision drive = decideNextMove(MOAT_STEPS[1], turned);
  TEST_ASSERT_EQUAL(MoveAction::DRIVE, drive.action);
  TEST_ASSERT_FLOAT_WITHIN(1.0f, 282.8f, drive.value);
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

// PAMI GAUCHE (bord à sa gauche) : le centre est à sa droite (-1) ; PAMI DROITE : à sa gauche (+1).
void test_detour_goes_toward_center() {
  TEST_ASSERT_EQUAL_FLOAT(-1.0f, chooseDetourSide(StartSide::LEFT));
  TEST_ASSERT_EQUAL_FLOAT(1.0f, chooseDetourSide(StartSide::RIGHT));
}

// Le détour et le virage vers la douve vont du même côté : celui du centre.
void test_detour_and_moat_on_same_side() {
  const StartSide sides[] = {StartSide::LEFT, StartSide::RIGHT};
  for (const StartSide side : sides) {
    const MissionStep moat = forSide(MOAT_STEPS[1], side);
    TEST_ASSERT_TRUE(moat.yMm * chooseDetourSide(side) > 0.0f);
  }
}

int runAllTests() {
  UNITY_BEGIN();
  RUN_TEST(test_table_point_start_is_origin);
  RUN_TEST(test_table_point_toward_center_is_right_for_left);
  RUN_TEST(test_moat_steps_for_left);
  RUN_TEST(test_moat_steps_for_right);
  RUN_TEST(test_moat_first_leg_is_straight);
  RUN_TEST(test_moat_second_leg_turns_toward_center);
  RUN_TEST(test_left_side_keeps_step);
  RUN_TEST(test_right_side_mirrors_step);
  RUN_TEST(test_detour_goes_toward_center);
  RUN_TEST(test_detour_and_moat_on_same_side);
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

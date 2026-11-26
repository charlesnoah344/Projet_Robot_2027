// Tests du calcul du rapport des diamètres droite / gauche (D023) et des diamètres de config.h.
// Sur PC : pio test -e native. Sur l'ESP32 branché en USB : pio test -e logic_esp32.
#include <unity.h>

#include "config.h"
#include "logic/wheel_ratio.h"

void setUp() {}
void tearDown() {}

// La mesure de T022 : 25 mm à gauche sur 800 mm, entraxe 101,6 mm -> 0,79 %.
void test_error_from_t022() {
  TEST_ASSERT_FLOAT_WITHIN(0.00001f, 0.0079375f, wheelRatioError(25.0f, 800.0f, 101.6f));
}

// À droite : erreur négative ; pile droit : aucune erreur.
void test_error_sign() {
  TEST_ASSERT_TRUE(wheelRatioError(-25.0f, 800.0f, 101.6f) < 0.0f);
  TEST_ASSERT_EQUAL_FLOAT(0.0f, wheelRatioError(0.0f, 800.0f, 101.6f));
}

// Le même écart sur une ligne deux fois plus longue veut dire une erreur 4 fois plus petite.
void test_error_scales_with_length_squared() {
  const float shortLine = wheelRatioError(20.0f, 500.0f, 101.6f);
  const float longLine = wheelRatioError(20.0f, 1000.0f, 101.6f);
  TEST_ASSERT_FLOAT_WITHIN(0.000001f, shortLine / 4.0f, longLine);
}

// Fini à gauche : on augmente le rapport ; à droite : on le diminue.
void test_corrected_ratio_direction() {
  TEST_ASSERT_TRUE(correctedWheelRatio(1.0f, 0.005f) > 1.0f);
  TEST_ASSERT_TRUE(correctedWheelRatio(1.0f, -0.005f) < 1.0f);
  TEST_ASSERT_FLOAT_WITHIN(0.00001f, 1.0079375f, correctedWheelRatio(1.0f, 0.0079375f));
}

// config.h : la moyenne des deux diamètres reste WHEEL_DIAMETER_MM, et leur rapport est celui demandé.
void test_config_diameters() {
  TEST_ASSERT_FLOAT_WITHIN(0.001f, WHEEL_DIAMETER_MM, (LEFT_WHEEL_DIAMETER_MM + RIGHT_WHEEL_DIAMETER_MM) / 2.0f);
  TEST_ASSERT_FLOAT_WITHIN(0.00001f, WHEEL_DIAMETER_RIGHT_TO_LEFT_RATIO,
                           RIGHT_WHEEL_DIAMETER_MM / LEFT_WHEEL_DIAMETER_MM);
  TEST_ASSERT_FLOAT_WITHIN(0.00001f, WHEEL_DIAMETER_RIGHT_TO_LEFT_RATIO, RIGHT_MM_PER_TICK / LEFT_MM_PER_TICK);
}

int runAllTests() {
  UNITY_BEGIN();
  RUN_TEST(test_error_from_t022);
  RUN_TEST(test_error_sign);
  RUN_TEST(test_error_scales_with_length_squared);
  RUN_TEST(test_corrected_ratio_direction);
  RUN_TEST(test_config_diameters);
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

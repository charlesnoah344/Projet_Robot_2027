// Tests de la logique du minuteur : bouton de départ et calcul du temps de match.
// Sur PC : pio test -e native. Sur l'ESP32 branché en USB : pio test -e logic_esp32.
#include <unity.h>

#include "logic/match_clock.h"
#include "logic/start_button.h"

void setUp() {}
void tearDown() {}

// ---------- Bouton de départ ----------

// Cas normal : bouton relâché à l'allumage, puis appui franc.
void test_start_after_release_then_press() {
  StartButtonDetector button(50);
  TEST_ASSERT_FALSE(button.update(false, 0));
  TEST_ASSERT_FALSE(button.update(false, 60));  // relâché depuis 60 ms : armé
  TEST_ASSERT_TRUE(button.isArmed());
  TEST_ASSERT_FALSE(button.update(true, 100));  // appui, pas encore stable
  TEST_ASSERT_TRUE(button.update(true, 150));   // appui stable depuis 50 ms : départ
}

// Bouton maintenu dès l'allumage : aucun départ tant qu'il n'a pas été relâché.
void test_no_start_when_held_at_power_on() {
  StartButtonDetector button(50);
  for (uint32_t t = 0; t <= 5000; t += 10) {
    TEST_ASSERT_FALSE(button.update(true, t));
  }
  TEST_ASSERT_FALSE(button.isArmed());
  TEST_ASSERT_FALSE(button.update(false, 5010));  // relâché
  TEST_ASSERT_FALSE(button.update(false, 5060));  // armé
  TEST_ASSERT_FALSE(button.update(true, 5100));
  TEST_ASSERT_TRUE(button.update(true, 5150));
}

// Un rebond plus court que l'anti-rebond ne lance pas le départ.
void test_short_bounce_is_ignored() {
  StartButtonDetector button(50);
  button.update(false, 0);
  button.update(false, 60);
  TEST_ASSERT_FALSE(button.update(true, 100));
  TEST_ASSERT_FALSE(button.update(true, 120));
  TEST_ASSERT_FALSE(button.update(false, 130));  // relâché après 30 ms seulement
  TEST_ASSERT_FALSE(button.update(false, 300));
}

// Un seul départ par allumage : un deuxième appui est ignoré.
void test_second_press_is_ignored() {
  StartButtonDetector button(50);
  button.update(false, 0);
  button.update(false, 60);
  button.update(true, 100);
  TEST_ASSERT_TRUE(button.update(true, 150));
  TEST_ASSERT_FALSE(button.update(true, 500));   // toujours appuyé
  TEST_ASSERT_FALSE(button.update(false, 600));  // relâché
  TEST_ASSERT_FALSE(button.update(false, 700));
  TEST_ASSERT_FALSE(button.update(true, 800));   // nouvel appui
  TEST_ASSERT_FALSE(button.update(true, 900));
}

// Le bouton marche aussi quand millis() repasse par zéro pendant l'appui.
void test_start_across_millis_overflow() {
  StartButtonDetector button(50);
  const uint32_t t0 = 0xFFFFFF00UL;
  button.update(false, t0);
  button.update(false, t0 + 60);
  TEST_ASSERT_FALSE(button.update(true, 0xFFFFFFF0UL));
  TEST_ASSERT_TRUE(button.update(true, 0x00000030UL));  // 64 ms plus tard, après le passage à zéro
}

// ---------- Temps de match ----------

void test_match_over_at_99_5_s() {
  TEST_ASSERT_FALSE(isMatchOver(1000, 1000));
  TEST_ASSERT_FALSE(isMatchOver(1000, 1000 + 99499));
  TEST_ASSERT_TRUE(isMatchOver(1000, 1000 + 99500));
  TEST_ASSERT_TRUE(isMatchOver(1000, 1000 + 200000));
}

void test_match_clock_across_millis_overflow() {
  const uint32_t start = 0xFFFFF000UL;  // départ juste avant le passage à zéro de millis()
  TEST_ASSERT_EQUAL_UINT32(0x2000UL, elapsedSinceMs(start, 0x00001000UL));
  TEST_ASSERT_FALSE(isMatchOver(start, start + 99499));
  TEST_ASSERT_TRUE(isMatchOver(start, start + 99500));
}

int runAllTests() {
  UNITY_BEGIN();
  RUN_TEST(test_start_after_release_then_press);
  RUN_TEST(test_no_start_when_held_at_power_on);
  RUN_TEST(test_short_bounce_is_ignored);
  RUN_TEST(test_second_press_is_ignored);
  RUN_TEST(test_start_across_millis_overflow);
  RUN_TEST(test_match_over_at_99_5_s);
  RUN_TEST(test_match_clock_across_millis_overflow);
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

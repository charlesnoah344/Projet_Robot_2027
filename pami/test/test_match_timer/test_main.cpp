// Tests de la logique du minuteur : commande de départ et côté (D013, D020), calcul du temps de match.
// Sur PC : pio test -e native. Sur l'ESP32 branché en USB : pio test -e logic_esp32.
#include <unity.h>

#include "logic/match_clock.h"
#include "logic/start_command.h"

void setUp() {}
void tearDown() {}

// Envoie une ligne entière au détecteur, caractère par caractère, et renvoie le dernier résultat
// différent de NONE (NONE si aucun).
CommandResult typeText(StartCommandDetector& detector, const char* text) {
  CommandResult last = CommandResult::NONE;
  for (const char* c = text; *c != '\0'; c++) {
    const CommandResult result = detector.feed(*c);
    if (result != CommandResult::NONE) {
      last = result;
    }
  }
  return last;
}

// ---------- Commande de départ (D013) et côté (D020) ----------

// Cas normal : « go g » puis Entrée (le moniteur envoie \r\n) : départ, PAMI GAUCHE.
void test_go_left_then_enter_starts() {
  StartCommandDetector detector;
  TEST_ASSERT_EQUAL(CommandResult::NONE, typeText(detector, "go g"));  // pas encore Entrée
  TEST_ASSERT_FALSE(detector.hasStarted());
  TEST_ASSERT_EQUAL(CommandResult::START, typeText(detector, "\r\n"));
  TEST_ASSERT_TRUE(detector.hasStarted());
  TEST_ASSERT_EQUAL(StartSide::LEFT, detector.startSide());
}

// « go d » : départ, PAMI DROITE.
void test_go_right_starts_on_right() {
  StartCommandDetector detector;
  TEST_ASSERT_EQUAL(CommandResult::START, typeText(detector, "go d\n"));
  TEST_ASSERT_EQUAL(StartSide::RIGHT, detector.startSide());
}

// « go » seul : refusé, il manque le côté. Ensuite, la bonne commande marche toujours.
void test_go_without_side_is_refused() {
  StartCommandDetector detector;
  TEST_ASSERT_EQUAL(CommandResult::MISSING_SIDE, typeText(detector, "go\n"));
  TEST_ASSERT_FALSE(detector.hasStarted());
  TEST_ASSERT_EQUAL(CommandResult::START, typeText(detector, "go d\n"));
  TEST_ASSERT_EQUAL(StartSide::RIGHT, detector.startSide());
}

// Majuscules et espaces ne comptent pas.
void test_case_and_spaces_are_ignored() {
  StartCommandDetector upper;
  TEST_ASSERT_EQUAL(CommandResult::START, typeText(upper, "GO G\n"));
  TEST_ASSERT_EQUAL(StartSide::LEFT, upper.startSide());
  StartCommandDetector spaced;
  TEST_ASSERT_EQUAL(CommandResult::START, typeText(spaced, "  go   d  \r"));
  TEST_ASSERT_EQUAL(StartSide::RIGHT, spaced.startSide());
  StartCommandDetector glued;
  TEST_ASSERT_EQUAL(CommandResult::START, typeText(glued, "gog\n"));
  TEST_ASSERT_EQUAL(StartSide::LEFT, glued.startSide());
}

// Les caractères arrivent en plusieurs fois (un tour de loop() par touche) : ça marche quand même.
void test_command_split_over_several_calls() {
  StartCommandDetector detector;
  TEST_ASSERT_EQUAL(CommandResult::NONE, typeText(detector, "g"));
  TEST_ASSERT_EQUAL(CommandResult::NONE, typeText(detector, "o "));
  TEST_ASSERT_EQUAL(CommandResult::NONE, typeText(detector, "d"));
  TEST_ASSERT_EQUAL(CommandResult::START, typeText(detector, "\n"));
  TEST_ASSERT_EQUAL(StartSide::RIGHT, detector.startSide());
}

// Une touche seule, un autre mot, un mauvais côté ou un mot trop long ne lancent pas le PAMI.
void test_other_lines_do_not_start() {
  StartCommandDetector detector;
  TEST_ASSERT_EQUAL(CommandResult::UNKNOWN, typeText(detector, "g\n"));
  TEST_ASSERT_EQUAL(CommandResult::UNKNOWN, typeText(detector, "goo\n"));
  TEST_ASSERT_EQUAL(CommandResult::UNKNOWN, typeText(detector, "go x\n"));
  TEST_ASSERT_EQUAL(CommandResult::UNKNOWN, typeText(detector, "go gd\n"));
  TEST_ASSERT_EQUAL(CommandResult::UNKNOWN, typeText(detector, "stop\n"));
  TEST_ASSERT_EQUAL(CommandResult::UNKNOWN, typeText(detector, "gogogogogogogogogogogogogo\n"));
  TEST_ASSERT_FALSE(detector.hasStarted());
  // Après des lignes refusées, la bonne commande marche toujours.
  TEST_ASSERT_EQUAL(CommandResult::START, typeText(detector, "go g\n"));
}

// Entrée seule : rien ne se passe.
void test_empty_line_does_nothing() {
  StartCommandDetector detector;
  TEST_ASSERT_EQUAL(CommandResult::NONE, typeText(detector, "\r\n\r\n"));
  TEST_ASSERT_FALSE(detector.hasStarted());
}

// Un seul départ par allumage : un deuxième départ est ignoré, et le côté ne change plus.
void test_second_go_is_ignored() {
  StartCommandDetector detector;
  TEST_ASSERT_EQUAL(CommandResult::START, typeText(detector, "go g\n"));
  TEST_ASSERT_EQUAL(CommandResult::IGNORED, typeText(detector, "go d\n"));
  TEST_ASSERT_EQUAL(CommandResult::IGNORED, typeText(detector, "stop\n"));
  TEST_ASSERT_TRUE(detector.hasStarted());
  TEST_ASSERT_EQUAL(StartSide::LEFT, detector.startSide());
}

// ---------- Temps de match ----------

// MATCH_STOP_MS plutôt qu'une valeur en dur : le test reste juste avec le profil labo (D007)
// comme avec le profil Eurobot.
void test_match_over_at_stop_time() {
  TEST_ASSERT_FALSE(isMatchOver(1000, 1000));
  TEST_ASSERT_FALSE(isMatchOver(1000, 1000 + MATCH_STOP_MS - 1));
  TEST_ASSERT_TRUE(isMatchOver(1000, 1000 + MATCH_STOP_MS));
  TEST_ASSERT_TRUE(isMatchOver(1000, 1000 + MATCH_STOP_MS + 100000));
}

void test_match_clock_across_millis_overflow() {
  const uint32_t start = 0xFFFFF000UL;  // départ juste avant le passage à zéro de millis()
  TEST_ASSERT_EQUAL_UINT32(0x2000UL, elapsedSinceMs(start, 0x00001000UL));
  TEST_ASSERT_FALSE(isMatchOver(start, start + MATCH_STOP_MS - 1));
  TEST_ASSERT_TRUE(isMatchOver(start, start + MATCH_STOP_MS));
}

int runAllTests() {
  UNITY_BEGIN();
  RUN_TEST(test_go_left_then_enter_starts);
  RUN_TEST(test_go_right_starts_on_right);
  RUN_TEST(test_go_without_side_is_refused);
  RUN_TEST(test_case_and_spaces_are_ignored);
  RUN_TEST(test_command_split_over_several_calls);
  RUN_TEST(test_other_lines_do_not_start);
  RUN_TEST(test_empty_line_does_nothing);
  RUN_TEST(test_second_go_is_ignored);
  RUN_TEST(test_match_over_at_stop_time);
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

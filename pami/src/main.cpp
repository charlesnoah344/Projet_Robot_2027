// Firmware du PAMI — phase d'exploration (voir pami/CLAUDE.md).
//
// loop() ne bloque jamais : chaque couche fait un petit travail puis rend la main.
// Ordre des couches à chaque tour :
//   1. minuteur (prioritaire sur tout) ;
//   2. capteurs ;
//   3. machine à états ;
//   4. motion ;
//   5. safety, qui pilote le driver.
#include <Arduino.h>

#include "config.h"
#include "encoders.h"
#include "match_timer.h"
#include "motion.h"
#include "motor_driver.h"
#include "safety.h"
#include "sensors.h"
#include "state_machine.h"

namespace {

// Affiche une fois par seconde le nombre de tours de loop(), l'état et le temps de match.
// Le nombre de tours doit rester élevé (au moins 1000 par seconde), sinon l'évitement
// réagirait en retard.
void logStatus(uint32_t nowMs) {
  static uint32_t loopCount = 0;
  static uint32_t lastLogMs = 0;

  loopCount++;
  if (nowMs - lastLogMs < STATUS_LOG_PERIOD_MS) {
    return;
  }
  if (DEBUG_LOGS) {
    const uint32_t loopsPerSecond = loopCount * 1000UL / (nowMs - lastLogMs);
    Serial.printf("[MAIN] %lu tours/s | etat %s | t = %.1f s\n", (unsigned long)loopsPerSecond,
                  stateName(stateMachine::state()), matchTimer::elapsedMs(nowMs) / 1000.0f);
  }
  loopCount = 0;
  lastLogMs = nowMs;
}

}  // namespace

void setup() {
  // En tout premier, avant même le port série : moteurs freinés.
  motorDriver::begin();

  Serial.begin(SERIAL_BAUD);
  Serial.println();
  Serial.println("[MAIN] PAMI ECAMlibur - firmware d'exploration (etape 1 : squelette)");
  // Rappel à chaque démarrage : l'exception D005 ne doit pas être oubliée.
  Serial.println("[TIME] ATTENTION : attente de 85 s DESACTIVEE (exception D005).");
  Serial.println("[TIME] A implementer avant tout match. Arret total a 99,5 s : actif.");

  encoders::begin();
  sensors::begin();
  motion::begin();
  matchTimer::begin();
  stateMachine::begin();
}

void loop() {
  const uint32_t nowMs = millis();

  matchTimer::update(nowMs);
  sensors::update(nowMs);
  stateMachine::update(nowMs);
  motion::update(nowMs);
  safety::apply(motion::command(), nowMs);

  logStatus(nowMs);
}

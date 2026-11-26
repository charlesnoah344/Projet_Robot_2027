// Firmware du PAMI, profil labo (voir docs/plan-logiciel-pami.md et D012 à D014).
//
// loop() ne bloque jamais : chaque couche fait un petit travail puis rend la main.
// Ordre des couches à chaque tour :
//   1. minuteur (prioritaire sur tout), qui lit aussi la commande de départ et le côté (D013, D020) ;
//   2. capteurs (2 ultrasons) ;
//   3. machine à états ;
//   4. motion ;
//   5. safety, qui pilote le driver ;
//   6. bras (coupé dès la fin du temps) ;
//   7. Wi-Fi (D010) : envoie les messages, accepte un téléversement PAMI immobile. La seule
//      commande lue est le départ : on ne peut pas piloter le PAMI à distance.
#include <Arduino.h>

#include "arm.h"
#include "config.h"
#include "console.h"
#include "encoders.h"
#include "match_timer.h"
#include "motion.h"
#include "motor_driver.h"
#include "safety.h"
#include "sensors.h"
#include "side.h"
#include "state_machine.h"
#include "wireless.h"

namespace {

// Affiche une fois par seconde le nombre de tours de loop(), l'état et le temps de match.
// Le nombre de tours doit rester élevé (au moins 1000 par seconde), sinon l'évitement
// réagirait en retard.
void logStatus(uint32_t nowMs) {
  static uint32_t loopCount = 0;
  static uint32_t lastLogMs = 0;

  // Rien avant le départ : on peut taper « go g » ou « go d » dans le moniteur sans que des lignes
  // défilent pendant qu'on écrit. Le comptage repart au départ : la première ligne est juste.
  if (!matchTimer::isStarted()) {
    loopCount = 0;
    lastLogMs = nowMs;
    return;
  }

  loopCount++;
  if (nowMs - lastLogMs < STATUS_LOG_PERIOD_MS) {
    return;
  }
  if (DEBUG_LOGS) {
    const uint32_t loopsPerSecond = loopCount * 1000UL / (nowMs - lastLogMs);
    int32_t leftTicks = 0;
    int32_t rightTicks = 0;
    encoders::read(leftTicks, rightTicks);
    Console.printf(
        "[MAIN] %lu tours/s | etat %s | t = %.1f s | cote %s | US G %u D %u mm | ticks G %ld D %ld\n",
        (unsigned long)loopsPerSecond, stateName(stateMachine::state()),
        matchTimer::elapsedMs(nowMs) / 1000.0f, side::isChosen() ? sideName(side::current()) : "?",
        sensors::distanceMm(sensors::UltrasoundId::LEFT),
        sensors::distanceMm(sensors::UltrasoundId::RIGHT), (long)leftTicks, (long)rightTicks);
  }
  loopCount = 0;
  lastLogMs = nowMs;
}

}  // namespace

void setup() {
  // En tout premier, avant même le port série : moteurs freinés.
  motorDriver::begin();

  Serial.begin(SERIAL_BAUD);
  Console.println();
  Console.println("[MAIN] PAMI ECAMlibur - firmware (PCB)");
  // Rappel à chaque démarrage : ce profil ne convient pas à un match Eurobot (D007, D013).
  Console.printf("[TIME] Profil LABO (D007) : mouvement a %.1f s, arret total a %.1f s.\n",
                 MATCH_MOVE_ALLOWED_MS / 1000.0f, MATCH_STOP_MS / 1000.0f);
  Console.println("[TIME] Pour Eurobot : 85,3 s et 99,5 s, depart par cordon ou par le robot"
                  " (depart Wi-Fi interdit, F.6).");

  arm::begin();  // servo sans PWM : il ne bouge pas
  encoders::begin();
  sensors::begin();
  motion::begin();
  matchTimer::begin();
  stateMachine::begin();
  // En dernier : créer le réseau prend une fraction de seconde, le PAMI est encore immobile.
  wireless::begin();
}

void loop() {
  const uint32_t nowMs = millis();

  matchTimer::update(nowMs);
  sensors::update(nowMs);
  stateMachine::update(nowMs);
  motion::update(nowMs, safety::isBlocking());
  safety::apply(motion::command(), nowMs);
  arm::update();

  logStatus(nowMs);

  // PAMI immobile : avant le départ, ou après la fin du temps.
  const bool idle = !matchTimer::isStarted() || matchTimer::isOver();
  // Profil Eurobot (WIFI_ON_DURING_MATCH = false) : radio coupée pendant le match (F.6).
  wireless::setRadioOn(WIFI_ON_DURING_MATCH || idle);
  wireless::update(nowMs, idle);  // téléversement accepté seulement PAMI immobile
}

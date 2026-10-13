// Banc de test de l'odométrie : ligne droite, rotation, carré, sans ultrason.
//
// !!! PROGRAMME DE TEST : AUCUN ÉVITEMENT. NE JAMAIS L'UTILISER EN MATCH. !!!
// Après le test, remettre le firmware : pio run -t upload
//
// Téléverser : pio run -e test_odometrie -t upload (maintenir BOOT pendant « Connecting »)
// Moniteur   : pio device monitor
//
// Ce banc utilise le VRAI code de déplacement du match (encoders, motion, mission, logic/) :
// les réglages trouvés ici servent tels quels au firmware de match.
// Différences avec le match : pas d'ultrason, pas de minuteur de match, pas de safety.
// Les commandes de motion vont directement au driver, toujours limitées par MOTOR_MAX_PWM_PCT.
// sensors.cpp est compilé seulement parce que mission.cpp en a besoin pour l'étape REACH_ZONE,
// que ce banc n'utilise pas : l'ultrason n'est jamais lu.
//
// MATÉRIEL ET MONTAGE
// - ESP32 en USB. MDD3A : M1A/M1B sur GPIO 25/26, M2A/M2B sur GPIO 33/32, GND commun.
// - 2 moteurs FIT0450 sur le bornier du MDD3A.
// - Encodeurs : + sur 5V, GND, gauche A/B sur GPIO 27/14, droit A/B sur GPIO 13/4.
// - Alimentation de labo à 6 V (environ 2 A) vers VB+ à travers le BAU.
// - Ultrason, IR, servo, bouton de départ : pas nécessaires.
//
// SÉCURITÉ
// - Premier essai de chaque commande SUR CALES, roues en l'air.
// - Au sol : 1,5 m dégagé devant et sur les côtés, quelqu'un à côté du BAU.
// - Au démarrage, rien ne bouge avant une commande.
// - Pendant un essai, N'IMPORTE QUELLE touche = STOP.
// - Arrêt automatique au bout de 15 s.
// - Roue bloquée ou encodeur inversé : motion passe en panne et tout s'arrête (redémarrer avec EN).
//
// PROTOCOLE
//  1. Encodeurs, roues à la main, rien ne bouge : faire faire 1 tour exact vers l'avant à chaque
//     roue (marque sur le pneu). Seul le compteur de cette roue augmente, d'environ 1920.
//  2. Sur cales : « 1 ». Les deux roues accélèrent doucement, tournent environ 4 s, ralentissent,
//     s'arrêtent. Pas de panne.
//  3. Au sol, ligne droite : marquer le départ, « 1 », mesurer la distance réelle D et l'écart
//     latéral. WHEEL_DIAMETER_MM = valeur actuelle × D / x affiché. Refaire 3 fois.
//  4. Au sol, rotation : repère au sol dans l'axe du PAMI, « 2 » (4 quarts de tour vers la gauche).
//     Mesurer l'angle réellement tourné A (360° attendus).
//     WHEEL_BASE_MM = valeur actuelle × 360 / A.
//  5. Au sol, carré : « 3 ». Mesurer l'écart entre l'arrivée et le départ.
//     « j » puis « 3 » : le même carré en jaune, dans l'autre sens (miroir).
//
// CRITÈRES DE RÉUSSITE
// - Encodeurs : 1920 ± 20 ticks par tour de roue, compteurs croissants vers l'avant.
// - Ligne droite : distance réelle à ± 5 mm de STRAIGHT_LINE_LENGTH_MM (500 mm), écart latéral < 10 mm.
// - Rotation : 360° ± 5° après réglage de WHEEL_BASE_MM.
// - Carré (SQUARE_SIDE_MM = 200 mm de côté) : retour à moins de 30 mm du départ.
#include <Arduino.h>
#include <esp_system.h>

#include "config.h"
#include "encoders.h"
#include "mission.h"
#include "motion.h"
#include "motor_driver.h"

namespace {

bool running = false;
uint32_t runStartMs = 0;
TeamColor color = TeamColor::BLUE;
bool faultReported = false;

// Pour calculer la vitesse de chaque roue entre deux lignes de mesures.
uint32_t lastLogMs = 0;
int32_t lastLogLeftTicks = 0;
int32_t lastLogRightTicks = 0;

const char* colorName() {
  return color == TeamColor::BLUE ? "BLEU" : "JAUNE";
}

// Affiche la cause du dernier redémarrage. « BROWNOUT » = chute de tension quand les moteurs démarrent.
void printResetReason() {
  const char* reason = "autre";
  switch (esp_reset_reason()) {
    case ESP_RST_POWERON: reason = "mise sous tension ou bouton EN"; break;
    case ESP_RST_SW: reason = "logiciel"; break;
    case ESP_RST_PANIC: reason = "plantage du programme"; break;
    case ESP_RST_BROWNOUT: reason = "BROWNOUT : CHUTE DE TENSION !"; break;
    case ESP_RST_INT_WDT:
    case ESP_RST_TASK_WDT:
    case ESP_RST_WDT: reason = "chien de garde (programme bloque)"; break;
    default: break;
  }
  Serial.printf("[BANC] Cause du redemarrage : %s\n", reason);
}

void printSettings() {
  Serial.printf("[BANC] Reglages : roue %.1f mm, entraxe %.1f mm, %ld ticks/tour, %.4f mm/tick\n",
                WHEEL_DIAMETER_MM, WHEEL_BASE_MM, (long)ENCODER_TICKS_PER_WHEEL_TURN, MM_PER_TICK);
  Serial.printf("[BANC] Vitesse : %.0f mm/s en ligne droite, %.0f mm/s par roue en rotation\n",
                DRIVE_SPEED_MM_S, TURN_SPEED_MM_S);
}

void printHelp() {
  Serial.println("[BANC] Commandes (une touche, sans Entree) :");
  Serial.printf("[BANC]   1 = ligne droite (%.0f mm)\n", STRAIGHT_LINE_LENGTH_MM);
  Serial.println("[BANC]   2 = rotation : 4 quarts de tour (un tour complet)");
  Serial.printf("[BANC]   3 = carre (%.0f mm de cote)\n", SQUARE_SIDE_MM);
  Serial.println("[BANC]   j = changer de couleur (bleu / jaune)");
  Serial.println("[BANC]   z = remettre la position a zero");
  Serial.println("[BANC]   h = cette aide");
  Serial.println("[BANC] Pendant un essai : N'IMPORTE QUELLE touche = STOP.");
}

// Une ligne de mesures : ticks, vitesse et PWM de chaque roue, position calculée.
void logMeasures(uint32_t nowMs) {
  int32_t leftTicks = 0;
  int32_t rightTicks = 0;
  encoders::read(leftTicks, rightTicks);
  const float dtS = (nowMs - lastLogMs) / 1000.0f;
  float leftMmS = 0.0f;
  float rightMmS = 0.0f;
  if (dtS > 0.0f) {
    leftMmS = (leftTicks - lastLogLeftTicks) * MM_PER_TICK / dtS;
    rightMmS = (rightTicks - lastLogRightTicks) * MM_PER_TICK / dtS;
  }
  lastLogMs = nowMs;
  lastLogLeftTicks = leftTicks;
  lastLogRightTicks = rightTicks;

  const Pose pose = motion::pose();
  const WheelCommand command = running ? motion::command() : WheelCommand();
  const float elapsedS = running ? (nowMs - runStartMs) / 1000.0f : 0.0f;
  Serial.printf("[ODO] t = %4.1f s | ticks G %6ld D %6ld | vitesse G %4.0f D %4.0f mm/s"
                " | PWM G %4.0f D %4.0f %% | x %5.0f mm  y %5.0f mm  cap %6.1f deg\n",
                elapsedS, (long)leftTicks, (long)rightTicks, leftMmS, rightMmS, command.leftPct,
                command.rightPct, pose.xMm, pose.yMm, pose.headingDeg);
}

void printResult() {
  const Pose pose = motion::pose();
  Serial.printf("[ODO] Position calculee : x = %.0f mm, y = %.0f mm, cap = %.1f deg\n", pose.xMm,
                pose.yMm, pose.headingDeg);
}

void startRun(MissionId id, const char* name, uint32_t nowMs) {
  if (motion::isFaulty()) {
    Serial.println("[BANC] Panne en cours : verifiez les encodeurs, puis redemarrez avec EN.");
    return;
  }
  mission::start(color, id);  // remet aussi la position à (0, 0), cap 0
  running = true;
  runStartMs = nowMs;
  Serial.printf("[BANC] Essai : %s, couleur %s. Une touche = STOP.\n", name, colorName());
}

void stopRun(const char* reason) {
  motion::stop();
  running = false;
  motorDriver::brake();
  Serial.printf("[BANC] STOP (%s)\n", reason);
  printResult();
}

void handleKey(char key, uint32_t nowMs) {
  if (key == '\r' || key == '\n') {
    return;  // touche Entrée : ignorée
  }
  // Sécurité : pendant un essai, n'importe quelle touche arrête tout.
  if (running) {
    stopRun("touche");
    return;
  }
  switch (tolower(key)) {
    case '1':
      startRun(MissionId::STRAIGHT_LINE, "ligne droite", nowMs);
      break;
    case '2':
      startRun(MissionId::ROTATION, "rotation (4 quarts de tour)", nowMs);
      break;
    case '3':
      startRun(MissionId::SQUARE, "carre", nowMs);
      break;
    case 'j':
      color = (color == TeamColor::BLUE) ? TeamColor::YELLOW : TeamColor::BLUE;
      Serial.printf("[BANC] Couleur : %s\n", colorName());
      break;
    case 'z':
      motion::resetPose();
      Serial.println("[BANC] Position remise a zero.");
      break;
    case 'h':
    case '?':
      printHelp();
      break;
    default:
      Serial.println("[BANC] Touche inconnue.");
      printHelp();
      break;
  }
}

}  // namespace

void setup() {
  // En tout premier : moteurs freinés.
  motorDriver::begin();

  Serial.begin(SERIAL_BAUD);
  Serial.println();
  Serial.println("[BANC] ===== BANC DE TEST ODOMETRIE =====");
  Serial.println("[BANC] PAS D'EVITEMENT : NE JAMAIS UTILISER EN MATCH.");
  Serial.println("[BANC] Apres le test : pio run -t upload (remet le firmware).");
  printResetReason();

  encoders::begin();
  motion::begin();
  encoders::read(lastLogLeftTicks, lastLogRightTicks);
  lastLogMs = millis();

  printSettings();
  printHelp();
  Serial.println("[BANC] Moteurs arretes. Tournez les roues a la main pour verifier les encodeurs.");
}

void loop() {
  const uint32_t nowMs = millis();

  while (Serial.available() > 0) {
    handleKey((char)Serial.read(), nowMs);
  }

  if (running) {
    mission::update();
    if (mission::isFinished() && motion::isDone()) {
      running = false;
      Serial.printf("[BANC] Essai termine en %.1f s.\n", (nowMs - runStartMs) / 1000.0f);
      printResult();
    } else if (nowMs - runStartMs >= TEST_BENCH_ODOMETRY_MAX_RUN_MS) {
      stopRun("arret automatique au bout de 15 s");
    }
  }

  if (motion::isFaulty() && !faultReported) {
    faultReported = true;
    running = false;
    Serial.println("[BANC] PANNE : roue bloquee ou encodeur inverse. Tout est arrete.");
    Serial.println("[BANC] Verifiez le sens des encodeurs (roues a la main), puis redemarrez avec EN.");
  }

  // Pas de safety dans ce banc : rien ne bloque la commande (false). Hors essai, freins serrés.
  motion::update(nowMs, false);
  const WheelCommand command = running ? motion::command() : WheelCommand();
  motorDriver::setWheelPercent(command.leftPct, command.rightPct);

  const uint32_t logPeriodMs =
      running ? TEST_BENCH_ODOMETRY_LOG_PERIOD_MS : TEST_BENCH_ODOMETRY_IDLE_LOG_PERIOD_MS;
  if (nowMs - lastLogMs >= logPeriodMs) {
    logMeasures(nowMs);
  }
}

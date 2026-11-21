// Banc de test « parcours » : aller à un point tapé au clavier, comme pendant un match.
//
// !!! PROGRAMME DE TEST : UN SEUL ULTRASON (GPIO 15). NE JAMAIS L'UTILISER EN MATCH. !!!
// Le firmware de match, lui, exige les deux ultrasons (règle d'or n° 1, D012). Ce banc existe pour
// essayer le comportement du match tant qu'un seul capteur est disponible.
// Après le test, remettre le firmware : pio run -t upload (sans fil : pio run -e esp32dev_wifi -t upload)
//
// Téléverser : pio run -e test_parcours -t upload -t monitor (maintenir BOOT pendant « Connecting »)
// Sans fil (D010) : pio run -e test_parcours_wifi -t upload, puis pio device monitor -e test_parcours_wifi.
//   Téléversement accepté seulement PAMI immobile. Les commandes marchent aussi depuis le moniteur Wi-Fi.
//
// CE QUE FAIT CE BANC
// On tape un point (x, y), le PAMI attend 10 s, puis y va tout seul, comme pendant un match :
// - aller au point : se tourner vers lui, avancer, viser de nouveau après un détour. C'est le même
//   calcul que la mission du match (logic/next_move.h) ;
// - obstacle devant (ultrason de GPIO 15, à moins de 150 mm) pendant une ligne droite : arrêt.
//   Toujours là après 1 s : détour, quart de tour VERS LE CENTRE de la table puis 200 mm de côté, puis
//   le PAMI vise de nouveau son point. 3 détours au plus, ensuite il attend que la voie se libère.
//   Mêmes règles et mêmes réglages que state_machine.cpp (AVOID_WAIT_MS, DETOUR_*, AVOID_MAX_DETOURS) ;
// - le filtre de sécurité est celui du match (logic/safety_filter.h) : pas de recul, pas d'avance vers un
//   obstacle, panne de l'ultrason = arrêt.
// Le point se donne dans le repère du PAMI posé au départ : x devant lui, y à sa gauche (négatif = à
// droite). Le côté de départ (gauche ou droite de la table) est écrit en dur dans config.h
// (TEST_BENCH_PARCOURS_START_ON_LEFT) : il ne sert qu'à choisir le sens du détour, vers le centre.
//
// MATÉRIEL ET MONTAGE
// - PCB 2026 5EO (D015), moteurs, encodeurs, alimentation des moteurs, BAU.
// - UN ultrason, sur le connecteur de GPIO 15, tourné vers l'avant. Le connecteur de GPIO 2 est ignoré :
//   le message « [SENS] PANNE ultrason gauche » au démarrage est donc normal.
//
// SÉCURITÉ
// - Rien ne bouge avant « p x y » puis les 10 s d'attente.
// - Pendant l'attente ou le trajet, N'IMPORTE QUELLE touche = STOP.
// - Un trajet s'arrête seul après 30 s.
// - Ultrason de GPIO 15 sans mesure pendant 1 s, ou roue bloquée : tout s'arrête (redémarrer avec EN).
// - Au sol : place dégagée, BAU à portée de main.
//
// COMMANDES : une lettre, des nombres si besoin, puis Entrée.
// Chiffres : pavé numérique, ou Maj + touche sur un clavier AZERTY.
//   p 600 -200   aller au point x = 600 mm (devant), y = -200 mm (à droite)
//   Entrée seule refaire le dernier point (replacer d'abord le PAMI au départ)
//   i            réglages       h  aide
//
// CE QU'ON REGARDE
// - les lignes [FSM] : ce que le PAMI décide (tourner, avancer, s'arrêter, contourner), dans l'ordre ;
// - les lignes [PARC] toutes les 0,5 s : position crue, distance vue par l'ultrason, distance à la cible ;
// - le bilan : position crue PAMI arrêté, écart avec la cible, durée, nombre de détours.
//   Mesurer ensuite la position RÉELLE au mètre ruban, depuis le même repère qu'au départ.
//
// PROTOCOLE PROPOSÉ
//  1. Sans obstacle : p 800 0, puis p 600 300, puis p 600 -300. Noter l'écart réel avec la cible.
//  2. Une boîte sur le trajet de p 800 0, à environ 400 mm du départ : arrêt devant, détour vers le
//     centre (vers la droite si départ à gauche), puis arrivée au point.
//  3. La même boîte, retirée pendant l'arrêt (avant 1 s) : le PAMI repart sans détour.
//  4. TEST_BENCH_PARCOURS_START_ON_LEFT = false, téléverser, refaire 2 : le détour change de côté.
//
// CRITÈRES DE RÉUSSITE
// - Sans obstacle : arrivée à moins de 30 mm du point (position crue), puis écart réel noté.
// - Avec obstacle : arrêt avant le contact, détour du côté du centre, arrivée au point.
// - Aucune marche arrière, aucune PANNE.
#include <Arduino.h>
#include <ctype.h>
#include <esp_system.h>
#include <math.h>
#include <stdlib.h>

#include "config.h"
#include "console.h"
#include "encoders.h"
#include "logic/avoidance.h"
#include "logic/mission_step.h"
#include "logic/next_move.h"
#include "logic/safety_filter.h"
#include "motion.h"
#include "motor_driver.h"
#include "sensors.h"
#include "wireless.h"

using sensors::UltrasoundId;

namespace {

// Le seul ultrason utilisé par ce banc : celui du connecteur de GPIO 15.
constexpr UltrasoundId BENCH_US = UltrasoundId::RIGHT;
const StartSide BENCH_SIDE = TEST_BENCH_PARCOURS_START_ON_LEFT ? StartSide::LEFT : StartSide::RIGHT;

// Les étapes du banc. GOTO, AVOID_STOP et DETOUR sont celles de la machine à états du match.
enum class Phase {
  IDLE,        // au repos : on attend une commande
  COUNTDOWN,   // « p x y » reçu : 10 s d'attente, immobile
  GOTO,        // va au point (comme l'état MISSION du match)
  AVOID_STOP,  // arrêté devant un obstacle
  DETOUR,      // contourne : quart de tour vers le centre, puis pas de côté
  SETTLING,    // trajet fini : moteurs freinés, on attend l'arrêt complet avant le bilan
  FAULT        // panne : plus rien ne bouge jusqu'au redémarrage
};
enum class DetourPhase { TURNING, SIDESTEP };

Phase phase = Phase::IDLE;
uint32_t phaseStartMs = 0;
DetourPhase detourPhase = DetourPhase::TURNING;
uint8_t detourCount = 0;

float targetXMm = 0.0f;
float targetYMm = 0.0f;
bool hasTarget = false;
uint32_t runStartMs = 0;
uint32_t runEndMs = 0;
uint32_t lastLogMs = 0;
uint32_t lastCountdownSecond = 0;
const char* endReason = "";

bool obstacleAhead = false;  // ultrason de GPIO 15, avec la marge de libération (comme safety)
bool blocking = false;       // le filtre de sécurité a modifié la dernière commande
bool sensorFaultReported = false;

// --- Ligne de commande tapée ---
char line[40];
size_t lineLength = 0;
bool lastWasCr = false;

const char* phaseName(Phase value) {
  switch (value) {
    case Phase::IDLE: return "REPOS";
    case Phase::COUNTDOWN: return "ATTENTE";
    case Phase::GOTO: return "VERS_POINT";
    case Phase::AVOID_STOP: return "ARRET_OBST";
    case Phase::DETOUR: return "DETOUR";
    case Phase::SETTLING: return "ARRET";
    case Phase::FAULT: return "PANNE";
  }
  return "?";
}

const char* detourSideName() {
  return chooseDetourSide(BENCH_SIDE) > 0.0f ? "GAUCHE" : "DROITE";
}

// Le PAMI peut bouger : trajet en cours.
bool isMoving() {
  return phase == Phase::GOTO || phase == Phase::AVOID_STOP || phase == Phase::DETOUR;
}

void enterPhase(Phase next, uint32_t nowMs) {
  Console.printf("[FSM] %s -> %s\n", phaseName(phase), phaseName(next));
  phase = next;
  phaseStartMs = nowMs;
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
  Console.printf("[BANC] Cause du redemarrage : %s\n", reason);
}

void printSettings() {
  Console.printf("[PARC] Cote de depart : %s (config.h) -> detour par la %s du PAMI, vers le centre\n",
                 sideName(BENCH_SIDE), detourSideName());
  Console.printf("[PARC] Ultrason : GPIO 15 seulement. Arret a %u mm, voie libre au-dela de %u mm.\n",
                 OBSTACLE_STOP_DISTANCE_MM, OBSTACLE_STOP_DISTANCE_MM + OBSTACLE_CLEAR_MARGIN_MM);
  Console.printf("[PARC] Detour : apres %.1f s d'arret, quart de tour de %.0f deg, %.0f mm de cote, %u detours au plus\n",
                 AVOID_WAIT_MS / 1000.0f, DETOUR_TURN_DEG, DETOUR_SIDE_MM, AVOID_MAX_DETOURS);
  Console.printf("[PARC] Vitesses : ligne %.0f mm/s, rotation %.0f mm/s par roue. Point atteint a %.0f mm pres.\n",
                 DRIVE_SPEED_MM_S, TURN_SPEED_MM_S, GOTO_REACHED_TOLERANCE_MM);
  Console.printf("[PARC] Etalonnage : roue %.2f mm, entraxe %.1f mm\n", WHEEL_DIAMETER_MM, WHEEL_BASE_MM);
}

void printHelp() {
  Console.println("[PARC] Commandes : une lettre, des nombres si besoin, puis Entree.");
  Console.println("[PARC]   p 600 -200   aller au point x = 600 mm (devant), y = -200 mm (a droite)");
  Console.println("[PARC]   Entree seule refaire le dernier point (replacer le PAMI au depart avant)");
  Console.println("[PARC]   i : reglages    h : aide");
  Console.printf("[PARC] Depart %lu s apres la commande. Pendant l'attente ou le trajet : N'IMPORTE QUELLE touche = STOP.\n",
                 (unsigned long)(TEST_BENCH_PARCOURS_COUNTDOWN_MS / 1000));
}

float distanceToTargetMm() {
  const Pose pose = motion::pose();
  return hypotf(targetXMm - pose.xMm, targetYMm - pose.yMm);
}

void printPose(const char* title) {
  const Pose pose = motion::pose();
  Console.printf("[PARC] %s : x = %.0f mm, y = %.0f mm, cap = %.1f deg | cible (%.0f, %.0f) a %.0f mm\n", title,
                 pose.xMm, pose.yMm, pose.headingDeg, targetXMm, targetYMm, distanceToTargetMm());
}

// --- Début, fin et pannes ---

void startCountdown(float xMm, float yMm, uint32_t nowMs) {
  if (phase == Phase::FAULT || motion::isFaulty()) {
    Console.println("[PARC] Panne en cours : redemarrez avec EN.");
    return;
  }
  if (sensors::isFaulty(BENCH_US)) {
    Console.println("[PARC] Ultrason GPIO 15 sans mesure : pas de depart (comme en match). Verifiez-le, puis EN.");
    return;
  }
  targetXMm = xMm;
  targetYMm = yMm;
  hasTarget = true;
  lastCountdownSecond = 0;
  Console.printf("[PARC] Point (%.0f, %.0f). Depart dans %lu s : placez le PAMI. Une touche = annuler.\n", xMm, yMm,
                 (unsigned long)(TEST_BENCH_PARCOURS_COUNTDOWN_MS / 1000));
  enterPhase(Phase::COUNTDOWN, nowMs);
}

void startRun(uint32_t nowMs) {
  motion::resetPose();  // comme au début d'une mission : le PAMI est posé en (0, 0), cap 0
  detourCount = 0;
  runStartMs = nowMs;
  lastLogMs = nowMs;
  Console.printf("[PARC] DEPART vers (%.0f, %.0f). Cote %s : en cas d'obstacle, detour par la %s.\n", targetXMm,
                 targetYMm, sideName(BENCH_SIDE), detourSideName());
  enterPhase(Phase::GOTO, nowMs);
}

// Fin du trajet (arrivé, temps dépassé ou touche) : freinage, puis bilan une fois le PAMI arrêté.
void finishRun(const char* reason, uint32_t nowMs) {
  motion::stop();
  endReason = reason;
  runEndMs = nowMs;
  enterPhase(Phase::SETTLING, nowMs);
}

void printReport() {
  Console.printf("[PARC] ===== %s en %.1f s, %u detour(s) =====\n", endReason,
                 (runEndMs - runStartMs) / 1000.0f, detourCount);
  printPose("Position crue, PAMI arrete");
  Console.println("[PARC] -> Mesurez la position REELLE (depuis le repere de depart) et comparez a la cible.");
  Console.println("[PARC]    Entree seule : refaire le meme point (replacer le PAMI au depart avant).");
}

void enterFault(const char* why, uint32_t nowMs) {
  motion::stop();
  Console.printf("[PARC] PANNE : %s. Tout est arrete. Verifiez, puis redemarrez avec EN.\n", why);
  printPose("Position crue");
  enterPhase(Phase::FAULT, nowMs);
}

// --- Le trajet : mêmes règles que state_machine.cpp et mission.cpp ---

// Le PAMI avance vers un obstacle : on arrête le mouvement et on passe en ARRET_OBST.
// Pendant une rotation sur place, l'obstacle ne compte pas (le filtre de sécurité la permet).
bool stopIfObstacle(uint32_t nowMs) {
  if (obstacleAhead && motion::isDriving()) {
    motion::stop();
    enterPhase(Phase::AVOID_STOP, nowMs);
    return true;
  }
  return false;
}

void startDetour(uint32_t nowMs) {
  detourCount++;
  // Toujours vers le centre de la table : à droite si on est parti de la gauche, et inversement (D014).
  const float detourSide = chooseDetourSide(BENCH_SIDE);
  Console.printf("[FSM] Detour %u par la %s (vers le centre)\n", detourCount, detourSideName());
  motion::turnAngle(detourSide * DETOUR_TURN_DEG);
  detourPhase = DetourPhase::TURNING;
  enterPhase(Phase::DETOUR, nowMs);
}

void updateGoto(uint32_t nowMs) {
  if (stopIfObstacle(nowMs)) {
    return;
  }
  if (!motion::isDone()) {
    return;  // un mouvement est en cours : on le laisse finir
  }
  // Même calcul que la mission du match : on regarde où on est et où aller, puis on décide.
  const MissionStep step = gotoPoint(targetXMm, targetYMm);
  const MoveDecision next = decideNextMove(step, motion::pose());
  switch (next.action) {
    case MoveAction::DONE:
      finishRun("ARRIVE", nowMs);
      break;
    case MoveAction::TURN:
      Console.printf("[FSM] Tourner de %+.1f deg vers le point\n", next.value);
      motion::turnAngle(next.value);
      break;
    case MoveAction::DRIVE:
      Console.printf("[FSM] Avancer de %.0f mm\n", next.value);
      motion::driveDistance(next.value);
      break;
  }
}

void updateAvoidStop(uint32_t nowMs) {
  // Niveau 1 : arrêté. Si l'obstacle s'en va, on reprend le trajet là où il en était.
  if (!obstacleAhead) {
    enterPhase(Phase::GOTO, nowMs);
    return;
  }
  // Niveau 2 : toujours là après AVOID_WAIT_MS -> détour (nombre limité).
  if (nowMs - phaseStartMs >= AVOID_WAIT_MS && detourCount < AVOID_MAX_DETOURS) {
    startDetour(nowMs);
  }
}

void updateDetour(uint32_t nowMs) {
  if (detourPhase == DetourPhase::SIDESTEP && stopIfObstacle(nowMs)) {
    return;
  }
  if (!motion::isDone()) {
    return;
  }
  if (detourPhase == DetourPhase::TURNING) {
    // Quart de tour fini : pas de côté.
    motion::driveDistance(DETOUR_SIDE_MM);
    detourPhase = DetourPhase::SIDESTEP;
  } else {
    // Niveau 3 : le PAMI vise de nouveau son point et contourne ainsi l'obstacle.
    enterPhase(Phase::GOTO, nowMs);
  }
}

void updatePhases(uint32_t nowMs) {
  // Priorités, comme dans le match : pannes et temps maximal avant tout le reste.
  if (isMoving()) {
    if (sensors::isFaulty(BENCH_US)) {
      enterFault("ultrason GPIO 15 sans mesure depuis 1 s", nowMs);
      return;
    }
    if (motion::isFaulty()) {
      enterFault("roue bloquee ou encodeur inverse", nowMs);
      return;
    }
    if (nowMs - runStartMs >= TEST_BENCH_PARCOURS_MAX_RUN_MS) {
      finishRun("TEMPS MAXIMAL DEPASSE", nowMs);
      return;
    }
  }

  switch (phase) {
    case Phase::COUNTDOWN: {
      const uint32_t elapsedMs = nowMs - phaseStartMs;
      if (elapsedMs >= TEST_BENCH_PARCOURS_COUNTDOWN_MS) {
        startRun(nowMs);
        break;
      }
      const uint32_t secondsLeft = (TEST_BENCH_PARCOURS_COUNTDOWN_MS - elapsedMs + 999) / 1000;
      if (secondsLeft != lastCountdownSecond) {
        lastCountdownSecond = secondsLeft;
        Console.printf("[PARC] Depart dans %lu s\n", (unsigned long)secondsLeft);
      }
      break;
    }
    case Phase::GOTO:
      updateGoto(nowMs);
      break;
    case Phase::AVOID_STOP:
      updateAvoidStop(nowMs);
      break;
    case Phase::DETOUR:
      updateDetour(nowMs);
      break;
    case Phase::SETTLING:
      if (nowMs - phaseStartMs >= TEST_BENCH_PARCOURS_SETTLE_MS) {
        printReport();
        enterPhase(Phase::IDLE, nowMs);
      }
      break;
    default:
      break;  // REPOS et PANNE : rien à faire
  }
}

// Obstacle devant l'ultrason de GPIO 15, avec la marge de libération : la même règle que safety.
void updateObstacle() {
  const uint16_t distanceMm = sensors::distanceMm(BENCH_US);
  const bool wasBlocked = obstacleAhead;
  obstacleAhead = isObstacleAhead(obstacleAhead, distanceMm);
  if (obstacleAhead != wasBlocked && isMoving()) {
    Console.printf("[SAFE] %s (%u mm)\n", obstacleAhead ? "Obstacle devant" : "Voie libre", distanceMm);
  }
}

// Le filtre de sécurité du match (logic/safety_filter.h), avec le seul ultrason de GPIO 15.
// Seul endroit de ce banc qui commande les moteurs.
void applyCommand() {
  const bool moveAllowed = isMoving();
  const WheelCommand requested = moveAllowed ? motion::command() : WheelCommand();
  const WheelCommand sent = filterCommand(requested, moveAllowed, sensors::isFaulty(BENCH_US), obstacleAhead);
  blocking = (sent.leftPct != requested.leftPct || sent.rightPct != requested.rightPct);
  motorDriver::setWheelPercent(sent.leftPct, sent.rightPct);
}

void logProgress(uint32_t nowMs) {
  if (!isMoving() || nowMs - lastLogMs < TEST_BENCH_PARCOURS_LOG_PERIOD_MS) {
    return;
  }
  lastLogMs = nowMs;
  const Pose pose = motion::pose();
  Console.printf("[PARC] t = %4.1f s | %-10s | x %5.0f y %5.0f mm cap %6.1f deg | US %4u mm | cible a %4.0f mm\n",
                 (nowMs - runStartMs) / 1000.0f, phaseName(phase), pose.xMm, pose.yMm, pose.headingDeg,
                 sensors::distanceMm(BENCH_US), distanceToTargetMm());
}

// Au repos, un seul message si l'ultrason de GPIO 15 ne mesure rien.
void reportSensorFaultOnce() {
  if (sensors::isFaulty(BENCH_US) && !sensorFaultReported) {
    sensorFaultReported = true;
    Console.println("[PARC] Ultrason GPIO 15 : aucune mesure depuis 1 s. Aucun depart possible.");
    Console.println("[PARC] Verifiez le capteur (banc test_ultrason), puis redemarrez avec EN.");
  }
}

// --- Lecture de la ligne tapée ---

char* skipSpaces(char* text) {
  while (*text == ' ') {
    text++;
  }
  return text;
}

// Lit deux nombres (x puis y). Virgules, points-virgules, parenthèses et chevrons servent de séparateurs :
// « p 600 -200 », « p 600, -200 » et « p (600; -200) » marchent. Faux s'il n'y a pas exactement deux nombres.
bool parseTwoNumbers(char* text, float& first, float& second) {
  for (char* c = text; *c != '\0'; c++) {
    if (*c == ',' || *c == ';' || *c == '(' || *c == ')' || *c == '<' || *c == '>') {
      *c = ' ';
    }
  }
  char* end = nullptr;
  first = strtof(text, &end);
  if (end == text) {
    return false;
  }
  char* secondStart = end;
  second = strtof(secondStart, &end);
  if (end == secondStart) {
    return false;
  }
  return *skipSpaces(end) == '\0';
}

void executeLine(uint32_t nowMs) {
  line[lineLength] = '\0';
  char* text = skipSpaces(line);
  if (*text == '\0') {  // Entrée seule : refaire le dernier point
    if (hasTarget) {
      startCountdown(targetXMm, targetYMm, nowMs);
    } else {
      printHelp();
    }
    return;
  }
  const char command = (char)tolower(*text);
  char* arguments = skipSpaces(text + 1);
  switch (command) {
    case 'p': {
      float xMm = 0.0f;
      float yMm = 0.0f;
      if (!parseTwoNumbers(arguments, xMm, yMm)) {
        Console.println("[PARC] Tapez x puis y en mm, par exemple : p 600 -200");
        return;
      }
      if (fabsf(xMm) > TEST_BENCH_PARCOURS_MAX_COORD_MM || fabsf(yMm) > TEST_BENCH_PARCOURS_MAX_COORD_MM) {
        Console.printf("[PARC] x et y : entre -%.0f et %.0f mm.\n", TEST_BENCH_PARCOURS_MAX_COORD_MM,
                       TEST_BENCH_PARCOURS_MAX_COORD_MM);
        return;
      }
      if (hypotf(xMm, yMm) <= GOTO_REACHED_TOLERANCE_MM) {
        Console.printf("[PARC] Point a moins de %.0f mm du depart : il est deja atteint.\n",
                       GOTO_REACHED_TOLERANCE_MM);
        return;
      }
      startCountdown(xMm, yMm, nowMs);
      break;
    }
    case 'i':
      printSettings();
      break;
    case 'h':
    case '?':
      printHelp();
      break;
    default:
      Console.println("[PARC] Commande inconnue.");
      printHelp();
      break;
  }
}

void handleChar(char c, uint32_t nowMs) {
  // Beaucoup de moniteurs envoient \r puis \n pour une seule touche Entrée : on n'en compte qu'une.
  // Sinon, le \n arriverait juste après la commande et annulerait aussitôt le départ.
  if (c == '\n' && lastWasCr) {
    lastWasCr = false;
    return;
  }
  lastWasCr = (c == '\r');

  // Sécurité : pendant l'attente ou le trajet, n'importe quelle touche arrête tout.
  if (phase == Phase::COUNTDOWN) {
    Console.println("[PARC] Depart annule (touche).");
    enterPhase(Phase::IDLE, nowMs);
    lineLength = 0;
    return;
  }
  if (isMoving()) {
    finishRun("STOP (touche)", nowMs);
    lineLength = 0;
    return;
  }
  if (phase == Phase::SETTLING) {
    return;  // le PAMI s'arrête : le bilan arrive dans un instant
  }
  if (c == '\r' || c == '\n') {
    Console.println();
    executeLine(nowMs);
    lineLength = 0;
    return;
  }
  if (c == '\b' || c == 127) {  // effacer le dernier caractère
    if (lineLength > 0) {
      lineLength--;
      Console.print("\b \b");
    }
    return;
  }
  if (c < 32 || c > 126 || lineLength >= sizeof(line) - 1) {
    return;
  }
  line[lineLength++] = c;
  Console.print(c);  // écho : le moniteur n'affiche pas ce qu'on tape
}

}  // namespace

void setup() {
  // En tout premier : moteurs freinés. Rien ne bouge avant une commande.
  motorDriver::begin();

  Serial.begin(SERIAL_BAUD);
  Console.println();
  Console.println("[BANC] ===== BANC DE TEST PARCOURS (aller a un point, comme en match) =====");
  Console.println("[BANC] UN SEUL ULTRASON (GPIO 15) : NE JAMAIS UTILISER EN MATCH.");
  Console.println("[BANC] Apres le test : remettre le firmware (pio run -t upload, ou -e esp32dev_wifi).");
  Console.println("[BANC] Le message \"PANNE ultrason gauche\" est normal : ce banc ignore le connecteur de GPIO 2.");
  printResetReason();

  encoders::begin();
  sensors::begin();
  motion::begin();

  printSettings();
  printHelp();
  wireless::begin();
}

void loop() {
  const uint32_t nowMs = millis();

  while (Console.available() > 0) {
    handleChar((char)Console.read(), nowMs);
  }

  sensors::update(nowMs);
  updateObstacle();
  updatePhases(nowMs);
  motion::update(nowMs, blocking);
  applyCommand();

  logProgress(nowMs);
  if (phase == Phase::IDLE) {
    reportSensorFaultOnce();
  }
  // Téléversement sans fil accepté seulement PAMI immobile.
  wireless::update(nowMs, phase == Phase::IDLE || phase == Phase::FAULT);
}

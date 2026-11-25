// Banc de test « parcours » : aller à un point tapé au clavier, comme pendant un match.
//
// !!! PROGRAMME DE TEST : UN SEUL ULTRASON. NE JAMAIS L'UTILISER EN MATCH. !!!
// Le firmware de match, lui, exige les deux ultrasons (règle d'or n° 1, D012). Ce banc existe pour
// essayer le comportement du match tant qu'un seul capteur est disponible.
// Après le test, remettre le firmware : pio run -t upload (sans fil : pio run -e esp32dev_wifi -t upload)
//
// Téléverser : pio run -e test_parcours -t upload -t monitor (maintenir BOOT pendant « Connecting »)
// Sans fil (D010) : pio run -e test_parcours_wifi -t upload, puis pio device monitor -e test_parcours_wifi.
//   Téléversement accepté seulement PAMI immobile. Les commandes marchent aussi depuis le moniteur Wi-Fi.
//
// CE QUE FAIT CE BANC
// On tape un point (x, y), ou « d » pour le trajet de la douve du match (D017). Le PAMI attend 10 s,
// puis y va tout seul, comme pendant un match :
// - aller au point : se tourner vers lui, avancer, viser de nouveau après un détour. C'est le même
//   calcul que la mission du match (logic/next_move.h) ;
// - obstacle devant (l'ultrason du banc, à moins de OBSTACLE_STOP_DISTANCE_MM) pendant une ligne droite : arrêt.
//   Toujours là après 1,5 s : contournement par le centre (D016, logic/detour.h, le même code que le
//   match) : quart de tour VERS LE CENTRE, 200 mm de côté, quart de tour retour (parallèle à la route),
//   pause de 0,3 s, regard. Voie libre sur plus de 300 mm : il avance de 300 mm pour dépasser
//   l'obstacle, puis vise de nouveau son point. Sinon, un pas de côté de plus (2 au plus).
//   2 contournements au plus ; ensuite il reste arrêté et repart dès que la voie se libère.
//   Mêmes réglages que le match (AVOID_WAIT_MS, DETOUR_*, AVOID_MAX_DETOURS) ;
// - le filtre de sécurité est celui du match (logic/safety_filter.h) : pas de recul, pas d'avance vers un
//   obstacle, panne de l'ultrason = arrêt.
// Le point se donne dans le repère du PAMI posé au départ : x devant lui, y à sa gauche (négatif = à
// droite). Le côté (GAUCHE = bord de table à gauche du PAMI, centre à sa droite, D017) se choisit avec
// « c g » ou « c d » (au démarrage : TEST_BENCH_PARCOURS_START_ON_LEFT). Il choisit le sens du détour, vers le centre, et
// le miroir du trajet « d ». Il ne change pas un point tapé avec « p ».
// Trajet « d » : les étapes MOAT_STEPS du match (logic/missions.h), mises en miroir par forSide().
// Pour GAUCHE : tout droit jusqu'à (1900, 0), virage d'environ 51° vers la droite, douve en (2079, -219).
// Pour DROITE : la même chose vers la gauche, douve en (2079, 219). Il faut environ 2,2 m de long.
//
// MATÉRIEL ET MONTAGE
// - PCB 2026 5EO (D015), moteurs, encodeurs, alimentation des moteurs, BAU.
// - UN ultrason, tourné vers l'avant, sur le connecteur choisi dans config.h
//   (TEST_BENCH_PARCOURS_USE_LEFT_US : false = connecteur droit, true = connecteur gauche).
//   L'autre connecteur est ignoré : le message « [SENS] PANNE ultrason ... » à son sujet est donc normal.
//
// SÉCURITÉ
// - Rien ne bouge avant « p x y » ou « d », puis les 10 s d'attente.
// - Pendant l'attente ou le trajet, N'IMPORTE QUELLE touche = STOP.
// - Un trajet s'arrête seul après 30 s.
// - Ultrason du banc sans mesure pendant 1 s, ou roue bloquée : tout s'arrête (redémarrer avec EN).
// - Au sol : place dégagée, BAU à portée de main.
//
// COMMANDES : une lettre, des nombres si besoin, puis Entrée.
// Chiffres : pavé numérique, ou Maj + touche sur un clavier AZERTY.
//   p 600 -200   aller au point x = 600 mm (devant), y = -200 mm (à droite)
//   d            trajet de la douve du match (D017), selon le côté choisi
//   c g / c d    côté GAUCHE / DROITE (sens du détour et miroir du trajet « d »)
//   Entrée seule refaire le dernier trajet (replacer d'abord le PAMI au départ)
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
//  2. Une petite boîte (environ 10 cm) sur le trajet de p 800 0, à environ 400 mm du départ : arrêt,
//     1,5 s, contournement par le centre (par la droite si départ à gauche), dépassement, ARRIVE.
//  3. Une grande boîte (environ 30 cm) : 2 pas de côté avant de dépasser, puis ARRIVE.
//  4. La petite boîte, retirée pendant l'arrêt (avant 1,5 s) : le PAMI repart sans contourner.
//  5. Une main devant le PAMI pendant le pas de côté : arrêt, attente, nouveau contournement.
//  6. « c d », puis refaire 2 : le contournement change de côté.
//  7. Trajet douve : marquer au sol le départ, le point de virage (1900 mm devant) et le centre de la
//     douve (2079 mm devant, 219 mm vers le centre). Taper d, côté GAUCHE puis DROITE. Noter la durée
//     et l'écart réel avec le centre de la douve. Puis refaire avec une boîte sur la ligne droite.
//
// CRITÈRES DE RÉUSSITE
// - Sans obstacle : arrivée à moins de 30 mm du point (position crue), puis écart réel noté.
// - Trajet douve : le PAMI tourne VERS LE CENTRE au point de virage et s'arrête dans la douve.
// - Avec obstacle : arrêt avant le contact, contournement du côté du centre, obstacle dépassé AVANT de
//   viser de nouveau le point, arrivée au point.
// - Aucune marche arrière, aucune PANNE. Noter la durée d'un contournement.
#include <Arduino.h>
#include <ctype.h>
#include <esp_system.h>
#include <math.h>
#include <stdlib.h>

#include "config.h"
#include "console.h"
#include "encoders.h"
#include "logic/avoidance.h"
#include "logic/detour.h"
#include "logic/mission_step.h"
#include "logic/missions.h"
#include "logic/next_move.h"
#include "logic/safety_filter.h"
#include "motion.h"
#include "motor_driver.h"
#include "sensors.h"
#include "wireless.h"

using sensors::UltrasoundId;

namespace {

// Le seul ultrason utilisé par ce banc, choisi dans config.h.
constexpr UltrasoundId BENCH_US = TEST_BENCH_PARCOURS_USE_LEFT_US ? UltrasoundId::LEFT : UltrasoundId::RIGHT;
constexpr uint8_t BENCH_US_ECHO_PIN = TEST_BENCH_PARCOURS_USE_LEFT_US ? PIN_US_LEFT_ECHO : PIN_US_RIGHT_ECHO;
// Côté du PAMI : « c g » ou « c d », comme la commande de départ du match (D020).
StartSide benchSide = TEST_BENCH_PARCOURS_START_ON_LEFT ? StartSide::LEFT : StartSide::RIGHT;

// Les étapes du banc. GOTO, AVOID_STOP et DETOUR sont celles de la machine à états du match.
enum class Phase {
  IDLE,        // au repos : on attend une commande
  COUNTDOWN,   // « p x y » ou « d » reçu : 10 s d'attente, immobile
  GOTO,        // suit les étapes du trajet (comme l'état MISSION du match)
  AVOID_STOP,  // arrêté devant un obstacle
  DETOUR,      // contourne par le centre (logic/detour.h, le même code que le match)
  SETTLING,    // trajet fini : moteurs freinés, on attend l'arrêt complet avant le bilan
  FAULT        // panne : plus rien ne bouge jusqu'au redémarrage
};

Phase phase = Phase::IDLE;
uint32_t phaseStartMs = 0;
Detour detour;
uint8_t detourCount = 0;  // contournements commencés pendant ce trajet

// Le trajet, comme une mission : une liste d'étapes, dans le repère du PAMI posé au départ.
// « p x y » : une seule étape. « d » : les étapes de la douve du match, déjà mises en miroir.
constexpr uint8_t BENCH_MAX_STEPS = 4;
constexpr uint8_t BENCH_MOAT_STEP_COUNT = sizeof(MOAT_STEPS) / sizeof(MissionStep);
static_assert(BENCH_MOAT_STEP_COUNT <= BENCH_MAX_STEPS, "BENCH_MAX_STEPS trop petit pour MOAT_STEPS");
MissionStep runSteps[BENCH_MAX_STEPS];
uint8_t runStepCount = 0;  // 0 : aucun trajet choisi depuis l'allumage
uint8_t runStepIndex = 0;  // étape en cours
const char* runName = "";

// Point visé par l'étape en cours (messages et distance restante).
float targetXMm = 0.0f;
float targetYMm = 0.0f;
uint32_t runStartMs = 0;
uint32_t runEndMs = 0;
uint32_t lastLogMs = 0;
uint32_t lastCountdownSecond = 0;
const char* endReason = "";

bool obstacleAhead = false;  // ultrason du banc, avec la marge de libération (comme safety)
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
  return chooseDetourSide(benchSide) > 0.0f ? "GAUCHE" : "DROITE";
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
  Console.printf("[PARC] Cote : %s (bord de table a %s du PAMI) -> detour par la %s du PAMI, vers le centre\n",
                 sideName(benchSide), benchSide == StartSide::LEFT ? "gauche" : "droite", detourSideName());
  Console.printf("[PARC] Ultrason : %s seulement (ECHO GPIO %u). Arret a %u mm, voie libre au-dela de %u mm.\n",
                 sensors::shortName(BENCH_US), BENCH_US_ECHO_PIN,
                 OBSTACLE_STOP_DISTANCE_MM, OBSTACLE_STOP_DISTANCE_MM + OBSTACLE_CLEAR_MARGIN_MM);
  Console.printf("[PARC] Contournement apres %.1f s d'arret : quart de tour, %.0f mm de cote (%u pas au plus),"
                 " retour parallele, regard apres %.1f s, depassement de %.0f mm. %u contournements au plus.\n",
                 AVOID_WAIT_MS / 1000.0f, DETOUR_SIDE_MM, DETOUR_MAX_SIDESTEPS, DETOUR_LOOK_MS / 1000.0f,
                 DETOUR_PASS_MM, AVOID_MAX_DETOURS);
  Console.printf("[PARC] Vitesses : ligne %.0f mm/s, rotation %.0f mm/s par roue. Point atteint a %.0f mm pres.\n",
                 DRIVE_SPEED_MM_S, TURN_SPEED_MM_S, GOTO_REACHED_TOLERANCE_MM);
  Console.printf("[PARC] Etalonnage : roue %.2f mm, entraxe %.1f mm\n", WHEEL_DIAMETER_MM, WHEEL_BASE_MM);
}

void printHelp() {
  Console.println("[PARC] Commandes : une lettre, des nombres si besoin, puis Entree.");
  Console.println("[PARC]   p 600 -200   aller au point x = 600 mm (devant), y = -200 mm (a droite)");
  Console.println("[PARC]   d            trajet de la douve du match (environ 2,2 m), selon le cote choisi");
  Console.println("[PARC]   c g / c d    cote GAUCHE / DROITE (sens du detour, miroir du trajet d)");
  Console.println("[PARC]   Entree seule refaire le dernier trajet (replacer le PAMI au depart avant)");
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

// Si l'étape en cours est un point, il devient la cible affichée.
void loadStepTarget() {
  const MissionStep& step = runSteps[runStepIndex];
  if (step.type == StepType::GOTO) {
    targetXMm = step.xMm;
    targetYMm = step.yMm;
  }
}

void printRoute() {
  Console.printf("[PARC] Trajet %s, cote %s :", runName, sideName(benchSide));
  for (uint8_t i = 0; i < runStepCount; i++) {
    const MissionStep& step = runSteps[i];
    switch (step.type) {
      case StepType::GOTO:
        Console.printf(" %s(%.0f, %.0f)", i > 0 ? "puis " : "", step.xMm, step.yMm);
        break;
      case StepType::TURN_TO:
        Console.printf(" %scap %.0f deg", i > 0 ? "puis " : "", step.headingDeg);
        break;
      case StepType::DEFEND:
        Console.print(" puis reste sur place");
        break;
    }
  }
  Console.println();
}

// Range dans runSteps les étapes de la douve du match (logic/missions.h), écrites pour GAUCHE et
// mises en miroir selon le côté choisi.
void loadMoatRoute() {
  for (uint8_t i = 0; i < BENCH_MOAT_STEP_COUNT; i++) {
    runSteps[i] = forSide(MOAT_STEPS[i], benchSide);
  }
  runStepCount = BENCH_MOAT_STEP_COUNT;
  runName = "douve";
}

// Le trajet est déjà rangé dans runSteps : 10 s d'attente avant le départ.
void startCountdown(uint32_t nowMs) {
  if (phase == Phase::FAULT || motion::isFaulty()) {
    Console.println("[PARC] Panne en cours : redemarrez avec EN.");
    return;
  }
  if (sensors::isFaulty(BENCH_US)) {
    Console.println("[PARC] Ultrason du banc sans mesure : pas de depart (comme en match). Verifiez-le, puis EN.");
    return;
  }
  lastCountdownSecond = 0;
  printRoute();
  Console.printf("[PARC] Depart dans %lu s : placez le PAMI. Une touche = annuler.\n",
                 (unsigned long)(TEST_BENCH_PARCOURS_COUNTDOWN_MS / 1000));
  enterPhase(Phase::COUNTDOWN, nowMs);
}

void startRun(uint32_t nowMs) {
  motion::resetPose();  // comme au début d'une mission : le PAMI est posé en (0, 0), cap 0
  detourCount = 0;
  runStepIndex = 0;
  loadStepTarget();
  runStartMs = nowMs;
  lastLogMs = nowMs;
  Console.printf("[PARC] DEPART, trajet %s, vers (%.0f, %.0f). Cote %s : en cas d'obstacle, detour par la %s.\n",
                 runName, targetXMm, targetYMm, sideName(benchSide), detourSideName());
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
  Console.println("[PARC]    Entree seule : refaire le meme trajet (replacer le PAMI au depart avant).");
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
  Console.printf("[FSM] Contournement %u par la %s (vers le centre)\n", detourCount, detourSideName());
  detour.start(chooseDetourSide(benchSide));
  enterPhase(Phase::DETOUR, nowMs);
}

// Exécute l'ordre suivant du contournement (logic/detour.h), comme state_machine.cpp.
void runDetour(uint32_t nowMs) {
  const uint16_t frontMm = sensors::distanceMm(BENCH_US);
  const bool wasLooking = detour.isLooking();
  const DetourOrder order = detour.next(frontMm, nowMs);
  switch (order.type) {
    case DetourOrderType::TURN:
      Console.printf("[FSM] Contournement : quart de tour (%+.0f deg)\n", order.value);
      motion::turnAngle(order.value);
      break;
    case DetourOrderType::DRIVE:
      if (wasLooking) {
        Console.printf("[FSM] Contournement : voie libre (%u mm), on depasse l'obstacle (%.0f mm)\n", frontMm,
                       order.value);
      } else {
        Console.printf("[FSM] Contournement : pas de cote (%.0f mm)\n", order.value);
      }
      motion::driveDistance(order.value);
      break;
    case DetourOrderType::WAIT:
      break;  // pause avant de regarder
    case DetourOrderType::DONE:
      Console.println("[FSM] Contournement reussi : on vise de nouveau le point");
      enterPhase(Phase::GOTO, nowMs);
      break;
    case DetourOrderType::FAILED:
      Console.printf("[FSM] Contournement impossible : voie bloquee a %u mm apres %u pas de cote\n", frontMm,
                     detour.sidesteps());
      enterPhase(Phase::AVOID_STOP, nowMs);
      break;
  }
}

void updateGoto(uint32_t nowMs) {
  if (stopIfObstacle(nowMs)) {
    return;
  }
  if (!motion::isDone()) {
    return;  // un mouvement est en cours : on le laisse finir
  }
  // Même calcul que la mission du match (mission.cpp) : on regarde où on est et où aller, puis on décide.
  const MissionStep& step = runSteps[runStepIndex];
  if (step.type == StepType::DEFEND) {
    finishRun("ARRIVE (on reste sur place, comme dans la douve)", nowMs);
    return;
  }
  const MoveDecision next = decideNextMove(step, motion::pose());
  switch (next.action) {
    case MoveAction::DONE:
      runStepIndex++;
      if (runStepIndex >= runStepCount) {
        finishRun("ARRIVE", nowMs);
        break;
      }
      // Étape intermédiaire (point de virage) : on passe à la suivante sans s'arrêter.
      printPose("Etape terminee");
      loadStepTarget();
      Console.printf("[FSM] Etape %u/%u\n", runStepIndex + 1, runStepCount);
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
  // Niveau 2 : toujours là après AVOID_WAIT_MS -> contournement (nombre limité, D016).
  if (nowMs - phaseStartMs >= AVOID_WAIT_MS && detourCount < AVOID_MAX_DETOURS) {
    startDetour(nowMs);
  }
}

void updateDetour(uint32_t nowMs) {
  // Chaque ligne droite du contournement reste surveillée : obstacle devant -> arrêt, ARRET_OBST.
  if (stopIfObstacle(nowMs)) {
    return;
  }
  // Un mouvement est en cours : on le laisse finir, puis on demande l'ordre suivant.
  if (!motion::isDone()) {
    return;
  }
  runDetour(nowMs);
}

void updatePhases(uint32_t nowMs) {
  // Priorités, comme dans le match : pannes et temps maximal avant tout le reste.
  if (isMoving()) {
    if (sensors::isFaulty(BENCH_US)) {
      enterFault("ultrason du banc sans mesure depuis 1 s", nowMs);
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

// Obstacle devant l'ultrason du banc, avec la marge de libération : la même règle que safety.
void updateObstacle() {
  const uint16_t distanceMm = sensors::distanceMm(BENCH_US);
  const bool wasBlocked = obstacleAhead;
  obstacleAhead = isObstacleAhead(obstacleAhead, distanceMm);
  if (obstacleAhead != wasBlocked && isMoving()) {
    Console.printf("[SAFE] %s (%u mm)\n", obstacleAhead ? "Obstacle devant" : "Voie libre", distanceMm);
  }
}

// Le filtre de sécurité du match (logic/safety_filter.h), avec le seul ultrason du banc.
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

// Au repos, un seul message si l'ultrason du banc ne mesure rien.
void reportSensorFaultOnce() {
  if (sensors::isFaulty(BENCH_US) && !sensorFaultReported) {
    sensorFaultReported = true;
    Console.println("[PARC] Ultrason du banc : aucune mesure depuis 1 s. Aucun depart possible.");
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
  if (*text == '\0') {  // Entrée seule : refaire le dernier trajet
    if (runStepCount > 0) {
      startCountdown(nowMs);
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
      // Point tapé dans le repère du PAMI : pas de miroir.
      runSteps[0] = gotoPoint(xMm, yMm);
      runStepCount = 1;
      runName = "point";
      startCountdown(nowMs);
      break;
    }
    case 'd':
      loadMoatRoute();
      Console.println("[PARC] Trajet de la douve du match (D017) : environ 2,2 m. Marquez au sol le point de virage"
                      " et la douve.");
      startCountdown(nowMs);
      break;
    case 'c': {
      const char sideLetter = (char)tolower(*arguments);
      if ((sideLetter != 'g' && sideLetter != 'd') || *skipSpaces(arguments + 1) != '\0') {
        Console.println("[PARC] Tapez c g (GAUCHE) ou c d (DROITE).");
        return;
      }
      benchSide = (sideLetter == 'g') ? StartSide::LEFT : StartSide::RIGHT;
      // Le trajet de la douve dépend du côté : on le refait pour que « Entrée seule » parte du bon côté.
      if (runStepCount > 0 && runName[0] == 'd') {
        loadMoatRoute();
      }
      printSettings();
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
  Console.printf("[BANC] UN SEUL ULTRASON (%s, ECHO GPIO %u) : NE JAMAIS UTILISER EN MATCH.\n",
                 sensors::shortName(BENCH_US), BENCH_US_ECHO_PIN);
  Console.println("[BANC] Apres le test : remettre le firmware (pio run -t upload, ou -e esp32dev_wifi).");
  Console.println("[BANC] Un message \"PANNE\" pour l'autre ultrason est normal : ce banc l'ignore.");
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

#include "mission.h"

#include <Arduino.h>

#include "config.h"
#include "logic/missions.h"
#include "logic/next_move.h"
#include "logic/zone_approach.h"
#include "motion.h"
#include "sensors.h"

namespace mission {

namespace {

TeamColor teamColor = TeamColor::BLUE;
const MissionStep* steps = nullptr;
uint8_t stepCount = 0;
uint8_t stepIndex = 0;
bool finished = true;
bool defendReached = false;

// Étape REACH_ZONE (D009).
enum class ZonePhase {
  NOT_STARTED,  // pas encore commencée
  SEARCHING,    // avance lentement en surveillant le sol
  ENTERING      // bord vu : avance encore de ZONE_ENTRY_MM pour entrer dans la zone
};
ZonePhase zonePhase = ZonePhase::NOT_STARTED;
bool floorStartLeft = false;  // ce que voyaient les capteurs de sol au début de l'approche
bool floorStartRight = false;
Pose searchStart;             // où l'approche a commencé

void logPose(const char* message) {
  if (!DEBUG_LOGS) {
    return;
  }
  const Pose pose = motion::pose();
  Serial.printf("[MISS] %s | x = %.0f mm, y = %.0f mm, cap = %.1f deg\n", message, pose.xMm,
                pose.yMm, pose.headingDeg);
}

void nextStep() {
  stepIndex++;
  zonePhase = ZonePhase::NOT_STARTED;
  logPose("Etape terminee");
  if (stepIndex >= stepCount) {
    finished = true;
    logPose("Mission terminee");
  }
}

void updateReachZone(const MissionStep& step) {
  // Pendant l'approche, on surveille le sol à chaque tour de loop() : dès qu'il change,
  // on est au bord de la zone. On freine tout de suite, puis on entre de ZONE_ENTRY_MM.
  if (zonePhase == ZonePhase::SEARCHING && motion::isDriving() &&
      floorChanged(floorStartLeft, floorStartRight, sensors::floorLeftReflects(),
                   sensors::floorRightReflects())) {
    motion::stop();
    logPose("Bord de la zone");
    zonePhase = ZonePhase::ENTERING;
    if (ZONE_ENTRY_MM > 0.0f) {
      motion::driveDistance(ZONE_ENTRY_MM, ZONE_APPROACH_SPEED_MM_S);
    }
    return;
  }

  // Un mouvement est en cours : on le laisse finir.
  if (!motion::isDone()) {
    return;
  }

  if (zonePhase == ZonePhase::ENTERING) {
    nextStep();  // dans la zone
    return;
  }

  if (zonePhase == ZonePhase::NOT_STARTED) {
    // Référence : le sol sous les capteurs, juste avant la zone.
    floorStartLeft = sensors::floorLeftReflects();
    floorStartRight = sensors::floorRightReflects();
    searchStart = motion::pose();
    zonePhase = ZonePhase::SEARCHING;
  }

  // Recherche, PAMI à l'arrêt (début, ou reprise après un obstacle) :
  // d'abord s'orienter vers la zone, puis avancer lentement ce qu'il reste à chercher.
  const float headingErrorDeg = normalizeDeg(step.headingDeg - motion::pose().headingDeg);
  if (fabsf(headingErrorDeg) > HEADING_TOLERANCE_DEG) {
    motion::turnAngle(headingErrorDeg);
    return;
  }
  const float remainingMm = remainingSearchMm(searchStart, motion::pose());
  if (remainingMm <= DRIVE_TOLERANCE_MM) {
    logPose("Bord de la zone non trouve");  // on s'arrête là, sans rouler plus loin
    nextStep();
    return;
  }
  motion::driveDistance(remainingMm, ZONE_APPROACH_SPEED_MM_S);
}

}  // namespace

void start(TeamColor color) {
  teamColor = color;
  motion::resetPose();  // le PAMI est posé au point de départ : (0, 0), cap 0
  steps = missionSteps(ACTIVE_MISSION, stepCount);
  stepIndex = 0;
  finished = (stepCount == 0);
  defendReached = false;
  zonePhase = ZonePhase::NOT_STARTED;
  if (DEBUG_LOGS) {
    Serial.printf("[MISS] Debut : %u etapes, couleur %s\n", stepCount,
                  color == TeamColor::BLUE ? "BLEU" : "JAUNE");
  }
}

void update() {
  if (finished || defendReached) {
    return;
  }

  const MissionStep step = forColor(steps[stepIndex], teamColor);
  if (step.type == StepType::REACH_ZONE) {
    updateReachZone(step);
    return;
  }

  // Un mouvement est en cours : on le laisse finir. safety peut le suspendre (obstacle).
  if (!motion::isDone()) {
    return;
  }

  if (step.type == StepType::DEFEND) {
    defendReached = true;
    logPose("Zone atteinte, defense");
    return;
  }

  const MoveDecision next = decideNextMove(step, motion::pose());
  switch (next.action) {
    case MoveAction::DONE:
      nextStep();
      break;
    case MoveAction::TURN:
      motion::turnAngle(next.value);
      break;
    case MoveAction::DRIVE:
      motion::driveDistance(next.value);
      break;
  }
}

bool isFinished() {
  return finished;
}

bool isDefendReached() {
  return defendReached;
}

}  // namespace mission

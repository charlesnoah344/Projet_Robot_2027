#include "mission.h"

#include <Arduino.h>

#include "config.h"
#include "logic/missions.h"
#include "logic/next_move.h"
#include "motion.h"

namespace mission {

namespace {

TeamColor teamColor = TeamColor::BLUE;
const MissionStep* steps = nullptr;
uint8_t stepCount = 0;
uint8_t stepIndex = 0;
bool finished = true;
bool defendReached = false;

void logPose(const char* message) {
  if (!DEBUG_LOGS) {
    return;
  }
  const Pose pose = motion::pose();
  Serial.printf("[MISS] %s | x = %.0f mm, y = %.0f mm, cap = %.1f deg\n", message, pose.xMm,
                pose.yMm, pose.headingDeg);
}

}  // namespace

void start(TeamColor color) {
  teamColor = color;
  motion::resetPose();  // le PAMI est posé au point de départ : (0, 0), cap 0
  steps = missionSteps(ACTIVE_MISSION, stepCount);
  stepIndex = 0;
  finished = (stepCount == 0);
  defendReached = false;
  if (DEBUG_LOGS) {
    Serial.printf("[MISS] Debut : %u etapes, couleur %s\n", stepCount,
                  color == TeamColor::BLUE ? "BLEU" : "JAUNE");
  }
}

void update() {
  if (finished || defendReached) {
    return;
  }
  // Un mouvement est en cours : on le laisse finir. safety peut le suspendre (obstacle).
  if (!motion::isDone()) {
    return;
  }

  const MissionStep step = forColor(steps[stepIndex], teamColor);
  if (step.type == StepType::DEFEND) {
    defendReached = true;
    logPose("Zone atteinte, defense");
    return;
  }

  const MoveDecision next = decideNextMove(step, motion::pose());
  switch (next.action) {
    case MoveAction::DONE:
      stepIndex++;
      logPose("Etape terminee");
      if (stepIndex >= stepCount) {
        finished = true;
        logPose("Mission terminee");
      }
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

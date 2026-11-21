#include "mission.h"

#include <Arduino.h>

#include "config.h"
#include "console.h"
#include "logic/missions.h"
#include "logic/nearest_moat.h"
#include "logic/next_move.h"
#include "motion.h"

namespace mission {

namespace {

StartSide startSide = StartSide::LEFT;
const MissionStep* steps = nullptr;
uint8_t stepCount = 0;
uint8_t stepIndex = 0;
bool finished = true;
bool defendReached = false;

// Mission douve : ses étapes sont remplies au départ, une fois la douve choisie.
MissionStep moatSteps[MOAT_STEP_COUNT] = {defend(), defend()};

// Choisit la douve la plus proche du départ et prépare les étapes pour y aller.
void prepareMoatMission() {
  const uint8_t moat = nearestMoatIndex(MOATS_MM, MOAT_COUNT, 0.0f, 0.0f);
  fillMoatSteps(MOATS_MM[moat], moatSteps);
  steps = moatSteps;
  stepCount = MOAT_STEP_COUNT;
  if (DEBUG_LOGS) {
    // Point écrit pour un départ à gauche ; forSide() le met en miroir pour un départ à droite.
    Console.printf("[MISS] Douve la plus proche : n. %u (x = %.0f mm, y = %.0f mm, cote gauche)\n",
                   moat, MOATS_MM[moat].xMm, MOATS_MM[moat].yMm);
  }
}

void logPose(const char* message) {
  if (!DEBUG_LOGS) {
    return;
  }
  const Pose pose = motion::pose();
  Console.printf("[MISS] %s | x = %.0f mm, y = %.0f mm, cap = %.1f deg\n", message, pose.xMm,
                 pose.yMm, pose.headingDeg);
}

void nextStep() {
  stepIndex++;
  logPose("Etape terminee");
  if (stepIndex >= stepCount) {
    finished = true;
    logPose("Mission terminee");
  }
}

}  // namespace

void start(StartSide side, MissionId id) {
  startSide = side;
  motion::resetPose();  // le PAMI est posé au point de départ : (0, 0), cap 0
  if (id == MissionId::MOAT) {
    prepareMoatMission();
  } else {
    steps = missionSteps(id, stepCount);
  }
  stepIndex = 0;
  finished = (stepCount == 0);
  defendReached = false;
  if (DEBUG_LOGS) {
    Console.printf("[MISS] Debut : %u etapes, depart a %s\n", stepCount, sideName(side));
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

  const MissionStep step = forSide(steps[stepIndex], startSide);
  if (step.type == StepType::DEFEND) {
    defendReached = true;
    logPose("Arrive : on reste ici, bras pret");
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

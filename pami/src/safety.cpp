#include "safety.h"

#include <Arduino.h>

#include "config.h"
#include "logic/safety_filter.h"
#include "match_timer.h"
#include "motor_driver.h"
#include "sensors.h"

namespace safety {

namespace {

bool blockedByUltrasound = false;
bool blockedByObstacle = false;
bool blocking = false;

}  // namespace

void apply(const WheelCommand& requested, uint32_t nowMs) {
  (void)nowMs;

  // Obstacle devant : ultrason (avec marge de libération), ou l'un des IR des angles avant.
  blockedByUltrasound = isObstacleAhead(blockedByUltrasound, sensors::frontDistanceMm());
  const bool wasBlockedByObstacle = blockedByObstacle;
  blockedByObstacle =
      blockedByUltrasound || sensors::irLeftBlocked() || sensors::irRightBlocked();
  if (DEBUG_LOGS && blockedByObstacle != wasBlockedByObstacle) {
    Serial.printf("[SAFE] %s (US %u mm, IR G %d, IR D %d)\n",
                  blockedByObstacle ? "Obstacle devant" : "Voie libre", sensors::frontDistanceMm(),
                  sensors::irLeftBlocked(), sensors::irRightBlocked());
  }

  const WheelCommand sent = filterCommand(requested, matchTimer::isMoveAllowed(),
                                          sensors::isFaulty(), blockedByObstacle);
  blocking = (sent.leftPct != requested.leftPct || sent.rightPct != requested.rightPct);

  // Seul endroit du firmware qui commande les moteurs.
  motorDriver::setWheelPercent(sent.leftPct, sent.rightPct);
}

bool obstacleAhead() {
  return blockedByObstacle;
}

bool isBlocking() {
  return blocking;
}

}  // namespace safety

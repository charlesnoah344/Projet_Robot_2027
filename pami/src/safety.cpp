#include "safety.h"

#include <Arduino.h>

#include "config.h"
#include "logic/safety_filter.h"
#include "match_timer.h"
#include "motor_driver.h"
#include "sensors.h"

namespace safety {

namespace {

bool blockedByObstacle = false;
bool blocking = false;

}  // namespace

void apply(const WheelCommand& requested, uint32_t nowMs) {
  (void)nowMs;

  const bool wasBlockedByObstacle = blockedByObstacle;
  blockedByObstacle = isObstacleAhead(blockedByObstacle, sensors::frontDistanceMm());
  if (DEBUG_LOGS && blockedByObstacle != wasBlockedByObstacle) {
    Serial.printf("[SAFE] %s (%u mm)\n", blockedByObstacle ? "Obstacle devant" : "Voie libre",
                  sensors::frontDistanceMm());
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

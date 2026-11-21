#include "safety.h"

#include <Arduino.h>

#include "config.h"
#include "console.h"
#include "logic/safety_filter.h"
#include "match_timer.h"
#include "motor_driver.h"
#include "sensors.h"

namespace safety {

namespace {

bool blockedLeft = false;   // l'ultrason gauche voit un obstacle
bool blockedRight = false;  // l'ultrason droit voit un obstacle
bool blockedByObstacle = false;
bool blocking = false;

// Obstacle devant UN capteur, avec une marge de libération. Message quand ça change.
bool updateBlocked(bool wasBlocked, sensors::UltrasoundId id) {
  const uint16_t distanceMm = sensors::distanceMm(id);
  const bool blocked = isObstacleAhead(wasBlocked, distanceMm);
  if (DEBUG_LOGS && blocked != wasBlocked) {
    Console.printf("[SAFE] US %s : %s (%u mm)\n", sensors::shortName(id),
                   blocked ? "obstacle devant" : "voie libre", distanceMm);
  }
  return blocked;
}

}  // namespace

void apply(const WheelCommand& requested, uint32_t nowMs) {
  (void)nowMs;

  // Chaque ultrason est jugé séparément (D012), puis on combine par un OU : un seul capteur qui
  // voit un obstacle suffit à freiner. Si l'un rate l'obstacle (écho perdu, capteur mal orienté,
  // obstacle hors de son cône), l'autre le voit encore.
  blockedLeft = updateBlocked(blockedLeft, sensors::UltrasoundId::LEFT);
  blockedRight = updateBlocked(blockedRight, sensors::UltrasoundId::RIGHT);
  blockedByObstacle = blockedLeft || blockedRight;

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

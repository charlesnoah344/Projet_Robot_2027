#include "motion.h"

#include <Arduino.h>

#include "config.h"
#include "console.h"
#include "encoders.h"
#include "logic/speed_controller.h"
#include "logic/speed_profile.h"

namespace motion {

namespace {

// SETTLE : cible atteinte, frein, on attend que le PAMI soit vraiment arrêté (D024).
enum class Mode { IDLE, DRIVE, TURN, SETTLE };

Mode mode = Mode::IDLE;
bool done = true;
bool faulty = false;

// Cible du mouvement en cours.
// DRIVE : distance à parcourir. TURN : arc que chaque roue doit parcourir (toujours positif).
float targetMm = 0.0f;
float turnSign = 1.0f;                     // TURN : +1 = vers la gauche, -1 = vers la droite
float cruiseSpeedMmS = DRIVE_SPEED_MM_S;   // DRIVE : vitesse de croisière de ce mouvement

// Compteurs des encodeurs au début du mouvement, et au cycle précédent.
int32_t startLeftTicks = 0;
int32_t startRightTicks = 0;
int32_t lastLeftTicks = 0;
int32_t lastRightTicks = 0;
uint32_t lastUpdateMs = 0;

float profileSpeedMmS = 0.0f;  // vitesse voulue par le profil en trapèze
SpeedController leftController(LEFT_MOTOR_DEADBAND_PCT, LEFT_MOTOR_PCT_PER_MM_S);
SpeedController rightController(RIGHT_MOTOR_DEADBAND_PCT, RIGHT_MOTOR_PCT_PER_MM_S);
WheelCommand requested;  // {0, 0} : arrêt

uint32_t stallStartMs = 0;  // début d'une suspicion de roue bloquée (0 = aucune)
uint32_t stillSinceMs = 0;  // SETTLE : début de l'immobilité des roues (0 = elles bougent encore)

Odometry odometry;

void resetRegulation() {
  profileSpeedMmS = 0.0f;
  leftController.reset();
  rightController.reset();
  requested = WheelCommand();
  stallStartMs = 0;
}

void startMove(Mode newMode) {
  encoders::read(startLeftTicks, startRightTicks);
  mode = newMode;
  done = false;
  resetRegulation();
}

void finishMove() {
  mode = Mode::IDLE;
  done = true;
  resetRegulation();
}

// Cible atteinte : on freine, mais le mouvement n'est fini qu'une fois le PAMI vraiment arrêté (D024).
// Pourquoi : à 350 mm/s, le PAMI glisse encore d'environ 10 mm après la fin d'une ligne droite. Si la
// rotation suivante démarrait pendant cette glissade, elle voulait l'annuler en faisant reculer les
// deux roues ; safety refusait (pas de marche arrière), et le PAMI restait bloqué (T027).
void brakeAndSettle() {
  mode = Mode::SETTLE;
  resetRegulation();  // commande {0, 0} : frein
  stillSinceMs = 0;
}

// Vrai quand les deux roues sont restées presque immobiles pendant MOTION_SETTLE_TIME_MS.
bool hasSettled(float leftMeasuredMmS, float rightMeasuredMmS, uint32_t nowMs) {
  const bool still = fabsf(leftMeasuredMmS) < MOTION_STOPPED_SPEED_MM_S &&
                     fabsf(rightMeasuredMmS) < MOTION_STOPPED_SPEED_MM_S;
  if (!still) {
    stillSinceMs = 0;  // une roue bouge encore : on recommence à compter
    return false;
  }
  if (stillSinceMs == 0) {
    stillSinceMs = nowMs;
    return false;
  }
  return nowMs - stillSinceMs >= MOTION_SETTLE_TIME_MS;
}

// Une roue qui reçoit beaucoup de PWM mais n'avance pas dans le bon sens, pendant trop longtemps.
bool isWheelStalled(float pct, float measuredMmS) {
  if (fabsf(pct) < STALL_MIN_PCT) {
    return false;
  }
  const float speedInCommandedDirection = (pct > 0.0f) ? measuredMmS : -measuredMmS;
  return speedInCommandedDirection < STALL_MAX_SPEED_MM_S;
}

void checkStall(float leftMeasuredMmS, float rightMeasuredMmS, uint32_t nowMs) {
  const bool stalled = isWheelStalled(requested.leftPct, leftMeasuredMmS) ||
                       isWheelStalled(requested.rightPct, rightMeasuredMmS);
  if (!stalled) {
    stallStartMs = 0;
    return;
  }
  if (stallStartMs == 0) {
    stallStartMs = nowMs;
  } else if (nowMs - stallStartMs >= STALL_TIME_MS) {
    faulty = true;
    finishMove();
    if (DEBUG_LOGS) {
      Console.println("[MOT] PANNE : roue bloquee ou encodeur inverse.");
    }
  }
}

}  // namespace

void begin() {
  encoders::read(lastLeftTicks, lastRightTicks);
  odometry.update(lastLeftTicks, lastRightTicks);  // point de départ : (0, 0), cap 0
  lastUpdateMs = millis();
}

void update(uint32_t nowMs, bool commandBlocked) {
  if (nowMs - lastUpdateMs < MOTION_PERIOD_MS) {
    return;
  }
  const float dtS = (nowMs - lastUpdateMs) / 1000.0f;
  lastUpdateMs = nowMs;

  // Vitesse mesurée de chaque roue depuis le cycle précédent.
  int32_t leftTicks = 0;
  int32_t rightTicks = 0;
  encoders::read(leftTicks, rightTicks);
  // Chaque roue avec son propre nombre de mm par tick (D023).
  const float leftMeasuredMmS = (leftTicks - lastLeftTicks) * LEFT_MM_PER_TICK / dtS;
  const float rightMeasuredMmS = (rightTicks - lastRightTicks) * RIGHT_MM_PER_TICK / dtS;
  lastLeftTicks = leftTicks;
  lastRightTicks = rightTicks;
  // Même à l'arrêt : si on pousse le PAMI, sa position change.
  odometry.update(leftTicks, rightTicks);

  if (mode == Mode::IDLE || faulty) {
    requested = WheelCommand();
    return;
  }

  // Cible atteinte : la commande reste {0, 0} (frein) jusqu'à l'arrêt complet. Pas de délai bloquant :
  // on regarde la vitesse des roues à chaque cycle. Si on pousse le PAMI, il attend ; le minuteur
  // garde le dernier mot (règle d'or n° 3).
  if (mode == Mode::SETTLE) {
    if (hasSettled(leftMeasuredMmS, rightMeasuredMmS, nowMs)) {
      finishMove();
    }
    return;
  }

  // safety a bloqué la dernière commande (obstacle, temps) : on repart de zéro quand la voie se
  // libère, avec une nouvelle accélération progressive. Pas de suspicion de blocage pendant ce temps.
  if (commandBlocked) {
    resetRegulation();
    return;
  }

  // Distance parcourue par chaque roue depuis le début du mouvement. La correction de cap égalise
  // ces deux distances : avec le bon diamètre de chaque roue, elles sont égales en vrai (D023).
  const float leftMm = (leftTicks - startLeftTicks) * LEFT_MM_PER_TICK;
  const float rightMm = (rightTicks - startRightTicks) * RIGHT_MM_PER_TICK;

  float leftTargetMmS = 0.0f;
  float rightTargetMmS = 0.0f;

  if (mode == Mode::DRIVE) {
    const float remainingMm = targetMm - (leftMm + rightMm) / 2.0f;
    if (remainingMm <= DRIVE_TOLERANCE_MM) {
      brakeAndSettle();
      return;
    }
    profileSpeedMmS = nextProfileSpeed(profileSpeedMmS, remainingMm, cruiseSpeedMmS, dtS);
    // Ligne droite : la roue qui a pris de l'avance ralentit, l'autre accélère.
    const float correctionMmS = HEADING_KP_MM_S_PER_MM * (leftMm - rightMm);
    leftTargetMmS = profileSpeedMmS - correctionMmS;
    rightTargetMmS = profileSpeedMmS + correctionMmS;
    // Jamais de roue en arrière en ligne droite (safety le refuserait).
    leftTargetMmS = fmaxf(leftTargetMmS, 0.0f);
    rightTargetMmS = fmaxf(rightTargetMmS, 0.0f);
  } else {  // Mode::TURN
    // Rotation vers la gauche : la roue droite avance, la gauche recule, d'autant.
    const float arcDoneMm = turnSign * (rightMm - leftMm) / 2.0f;
    const float remainingMm = targetMm - arcDoneMm;
    if (remainingMm <= TURN_TOLERANCE_MM) {
      brakeAndSettle();
      return;
    }
    profileSpeedMmS = nextProfileSpeed(profileSpeedMmS, remainingMm, TURN_SPEED_MM_S, dtS);
    // Sur place : les deux distances doivent rester opposées (leur somme nulle).
    // Si la somme devient positive, le PAMI avance un peu : on corrige.
    const float correctionMmS = HEADING_KP_MM_S_PER_MM * (leftMm + rightMm) / 2.0f;
    leftTargetMmS = -turnSign * profileSpeedMmS - correctionMmS;
    rightTargetMmS = turnSign * profileSpeedMmS - correctionMmS;
  }

  requested.leftPct = leftController.update(leftTargetMmS, leftMeasuredMmS, dtS);
  requested.rightPct = rightController.update(rightTargetMmS, rightMeasuredMmS, dtS);
  checkStall(leftMeasuredMmS, rightMeasuredMmS, nowMs);
}

void driveDistance(float distanceMm, float speedMmS) {
  if (faulty || distanceMm <= 0.0f) {
    return;  // pas de marche arrière
  }
  startMove(Mode::DRIVE);
  targetMm = distanceMm;
  // Jamais plus vite que la vitesse de croisière normale.
  cruiseSpeedMmS = fminf(speedMmS, DRIVE_SPEED_MM_S);
}

void turnAngle(float angleDeg) {
  if (faulty) {
    return;
  }
  startMove(Mode::TURN);
  turnSign = (angleDeg >= 0.0f) ? 1.0f : -1.0f;
  // Arc parcouru par chaque roue : angle (rad) × demi-entraxe.
  targetMm = fabsf(angleDeg) * DEG_TO_RAD * WHEEL_BASE_MM / 2.0f;
}

void stop() {
  finishMove();
}

bool isDone() {
  return done;
}

bool isDriving() {
  return mode == Mode::DRIVE;
}

bool isFaulty() {
  return faulty;
}

Pose pose() {
  return odometry.pose();
}

void resetPose() {
  int32_t leftTicks = 0;
  int32_t rightTicks = 0;
  encoders::read(leftTicks, rightTicks);
  odometry = Odometry();
  odometry.update(leftTicks, rightTicks);
}

WheelCommand command() {
  return requested;
}

}  // namespace motion
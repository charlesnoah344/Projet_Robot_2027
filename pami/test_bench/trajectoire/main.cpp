// Banc de test des trajectoires : ligne droite, rotation et carré aux dimensions choisies au clavier,
// puis calcul des corrections d'étalonnage à partir de ce que vous mesurez au mètre ruban.
//
// !!! PROGRAMME DE TEST : PAS D'ÉVITEMENT COMPLET. NE JAMAIS L'UTILISER EN MATCH. !!!
// Après le test, remettre le firmware : pio run -t upload
//
// Téléverser : pio run -e test_trajectoire -t upload -t monitor (maintenir BOOT pendant « Connecting »)
// Méthode complète (ordre des essais, comment corriger) : docs/reglage-regulation.md
//
// CE QUE CE BANC RÉPOND
// À chaque essai, il compare trois choses :
// - ce que vous avez COMMANDÉ (ex. 1000 mm) ;
// - ce que le PAMI CROIT avoir fait (calculé avec ses encodeurs) ;
// - ce qu'il a VRAIMENT fait (vous le mesurez, puis vous le tapez avec « m »).
// Commandé ≠ cru : c'est le contrôle (motion, régulateur) qui a mal travaillé.
// Cru ≠ réel : le contrôle a bien travaillé, mais le PAMI mesure mal : c'est l'ÉTALONNAGE
// (diamètre des roues, entraxe). Le banc calcule alors la valeur à mettre dans config.h.
// Il utilise le VRAI code de déplacement du match (motion, régulateur de vitesse, odométrie).
//
// MATÉRIEL ET MONTAGE
// - Comme test_odometrie : MDD3A, encodeurs, alimentation des moteurs, masse commune.
// - Ultrason branché : pendant les lignes droites, le PAMI s'arrête à 150 mm d'un obstacle.
//   C'est une sécurité, pas l'évitement du match. Ultrason en panne : le banc prévient et roule
//   quand même (attention).
//
// SÉCURITÉ
// - Au démarrage, rien ne bouge avant une commande.
// - Premier essai SUR CALES. Au sol : BAU câblé (règle d'or n° 2), place dégagée, quelqu'un prêt
//   à couper l'alimentation.
// - Pendant un essai, N'IMPORTE QUELLE touche = STOP.
// - Arrêt automatique si l'essai dure 2 fois plus que prévu.
// - Roue bloquée ou encodeur inversé : motion passe en panne, tout s'arrête (redémarrer avec EN).
//
// COMMANDES : une lettre, un nombre si besoin, puis Entrée (le banc affiche ce que vous tapez).
// Chiffres : pavé numérique, ou Maj + touche sur un clavier AZERTY.
//   l 1000   ligne droite de 1000 mm (50 à 2000)
//   r 90     rotation de 90° vers la gauche ; r -90 vers la droite ; r 360 = un tour
//   c 300    carré de 300 mm en tournant à gauche ; c -300 en tournant à droite
//   v 100    vitesse des lignes droites, en mm/s (40 à DRIVE_SPEED_MM_S)
//   m 503    MESURE réelle du dernier essai (mm, ou degrés pour une rotation) : calcule la correction
//   Entrée seule : refaire le même essai (pratique pour répéter 3 à 5 fois)
//   n : nouvelle série de mesures   z : position à zéro   i : réglages   h : aide
//
// PROTOCOLE (détails dans docs/reglage-regulation.md)
//  1. Diamètre des roues : « l 1000 », mesurer la distance réelle, « m ... ». 3 à 5 fois.
//     Le banc donne WHEEL_DIAMETER_MM conseillé. Le reporter dans config.h, téléverser.
//  2. Entraxe, SEULEMENT APRÈS le diamètre (l'angle mesuré en dépend) : « r 360 », mesurer
//     l'angle réel, « m ... ». 3 à 5 fois, puis « r -360 ». WHEEL_BASE_MM conseillé : config.h.
//  3. Carrés dans les deux sens : « c 300 » puis « c -300 », 5 fois chacun, en mesurant l'écart
//     entre le repère de départ et le PAMI à l'arrivée (« m ... »).
//
// CRITÈRES DE RÉUSSITE
// - Ligne droite : distance réelle à 1 % près ; écart latéral réel < 10 mm sur 500 mm.
// - Rotation : angle réel à 5° près par tour.
// - Carré : retour à moins de 30 mm du départ, dans les deux sens.
// - Essais répétés : dispersion < 1 % (sinon : patinage ou mécanique, à régler d'abord).
#include <Arduino.h>
#include <esp_system.h>
#include <stdlib.h>

#include "config.h"
#include "encoders.h"
#include "motion.h"
#include "motor_driver.h"
#include "sensors.h"

namespace {

enum class RunKind { NONE, LINE, TURN, SQUARE };

// Une étape d'un essai : avancer de value mm, ou tourner de value degrés.
struct Step {
  bool drive;
  float value;
};

// Mesures répétées : on garde de quoi calculer la moyenne et la dispersion.
struct Series {
  uint32_t count = 0;
  float sum = 0.0f;
  float min = 0.0f;
  float max = 0.0f;

  void add(float value) {
    if (count == 0 || value < min) {
      min = value;
    }
    if (count == 0 || value > max) {
      max = value;
    }
    sum += value;
    count++;
  }
  float mean() const { return (count > 0) ? sum / count : 0.0f; }
};

// --- Réglages choisis au clavier ---
float lineDistanceMm = TEST_BENCH_TRAJ_DEFAULT_DISTANCE_MM;
float turnAngleDeg = TEST_BENCH_TRAJ_DEFAULT_ANGLE_DEG;
float squareSideMm = TEST_BENCH_TRAJ_DEFAULT_SQUARE_MM;  // > 0 : en tournant à gauche
float lineSpeedMmS = DRIVE_SPEED_MM_S;

// --- Essai en cours ---
RunKind running = RunKind::NONE;
Step steps[8];
int stepCount = 0;
int stepIndex = 0;
uint32_t runStartMs = 0;
uint32_t runTimeoutMs = 0;
int32_t startLeftTicks = 0;
int32_t startRightTicks = 0;
float maxLeftPct = 0.0f;  // PWM la plus forte demandée pendant l'essai : la marge avant 100 %
float maxRightPct = 0.0f;
bool faultReported = false;
bool sensorWarningPrinted = false;

// --- Dernier essai terminé : ce que le PAMI croit avoir fait, pour la commande « m » ---
RunKind lastRun = RunKind::NONE;     // pour « Entrée seule = refaire »
RunKind measurableRun = RunKind::NONE;  // NONE si l'essai a été interrompu
float believedValue = 0.0f;  // mm (ligne), degrés (rotation), écart au départ en mm (carré)
float lastSquareSideMm = 0.0f;

// --- Séries de mesures ---
Series lineRatios;    // réel / cru, pour le diamètre des roues
Series turnRatios;    // cru / réel, pour l'entraxe
Series squaresLeft;   // écart réel au départ, carrés en tournant à gauche (mm)
Series squaresRight;  // idem, en tournant à droite

// --- Ligne de commande tapée ---
char line[32];
size_t lineLength = 0;
bool lastWasCr = false;
uint32_t lastLogMs = 0;

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

void printHelp() {
  Serial.println("[TRAJ] Commandes : une lettre, un nombre si besoin, puis Entree. Exemples :");
  Serial.printf("[TRAJ]   l 1000   ligne droite de 1000 mm (%.0f a %.0f)\n", TEST_BENCH_TRAJ_MIN_DISTANCE_MM,
                TEST_BENCH_TRAJ_MAX_DISTANCE_MM);
  Serial.println("[TRAJ]   r 90     rotation de 90 deg vers la gauche ; r -90 vers la droite ; r 360 = un tour");
  Serial.println("[TRAJ]   c 300    carre de 300 mm en tournant a gauche ; c -300 en tournant a droite");
  Serial.printf("[TRAJ]   v 100    vitesse des lignes droites, en mm/s (%.0f a %.0f)\n", MOTION_MIN_SPEED_MM_S,
                DRIVE_SPEED_MM_S);
  Serial.println("[TRAJ]   m 503    MESURE reelle du dernier essai (mm, ou deg pour une rotation)");
  Serial.println("[TRAJ]   Entree seule : refaire le meme essai    n : nouvelle serie de mesures");
  Serial.println("[TRAJ]   z : position a zero    i : reglages    h : aide");
  Serial.println("[TRAJ] Chiffres : pave numerique, ou Maj + touche sur un clavier AZERTY.");
  Serial.println("[TRAJ] Pendant un essai : N'IMPORTE QUELLE touche = STOP.");
}

void printSettings() {
  Serial.printf("[TRAJ] Etalonnage : roue %.2f mm, entraxe %.1f mm, %ld ticks/tour (%.4f mm/tick)\n",
                WHEEL_DIAMETER_MM, WHEEL_BASE_MM, (long)ENCODER_TICKS_PER_WHEEL_TURN, MM_PER_TICK);
  Serial.printf("[TRAJ] Vitesses : ligne %.0f mm/s, rotation %.0f mm/s par roue, acceleration %.0f mm/s2\n",
                lineSpeedMmS, TURN_SPEED_MM_S, MOTION_ACCEL_MM_S2);
  Serial.printf("[TRAJ] Regulateur : KP %.3f, KI %.3f | anticipation G %.1f %% + %.3f, D %.1f %% + %.3f\n",
                SPEED_KP_PCT_PER_MM_S, SPEED_KI_PCT_PER_MM, LEFT_MOTOR_DEADBAND_PCT, LEFT_MOTOR_PCT_PER_MM_S,
                RIGHT_MOTOR_DEADBAND_PCT, RIGHT_MOTOR_PCT_PER_MM_S);
  Serial.printf("[TRAJ] Derniers choix : ligne %.0f mm, rotation %.0f deg, carre %.0f mm\n", lineDistanceMm,
                turnAngleDeg, squareSideMm);
}

// --- Ce que le PAMI croit avoir fait depuis le début de l'essai (encodeurs) ---

void wheelDistancesMm(float& leftMm, float& rightMm) {
  int32_t leftTicks = 0;
  int32_t rightTicks = 0;
  encoders::read(leftTicks, rightTicks);
  leftMm = (leftTicks - startLeftTicks) * MM_PER_TICK;
  rightMm = (rightTicks - startRightTicks) * MM_PER_TICK;
}

// Distance parcourue : moyenne des deux roues.
float believedDistanceMm() {
  float leftMm = 0.0f;
  float rightMm = 0.0f;
  wheelDistancesMm(leftMm, rightMm);
  return (leftMm + rightMm) / 2.0f;
}

// Angle tourné, sans le ramener entre -180 et +180 (un tour = 360). Positif = vers la gauche.
float believedAngleDeg() {
  float leftMm = 0.0f;
  float rightMm = 0.0f;
  wheelDistancesMm(leftMm, rightMm);
  return (rightMm - leftMm) / WHEEL_BASE_MM * RAD_TO_DEG;
}

// --- Déroulement d'un essai ---

uint32_t expectedDurationMs() {
  float driveMm = 0.0f;
  float arcMm = 0.0f;
  for (int i = 0; i < stepCount; i++) {
    if (steps[i].drive) {
      driveMm += steps[i].value;
    } else {
      arcMm += fabsf(steps[i].value) * DEG_TO_RAD * WHEEL_BASE_MM / 2.0f;
    }
  }
  return (uint32_t)((driveMm / lineSpeedMmS + arcMm / TURN_SPEED_MM_S) * 1000.0f);
}

void startStep(int index) {
  if (steps[index].drive) {
    motion::driveDistance(steps[index].value, lineSpeedMmS);
  } else {
    motion::turnAngle(steps[index].value);
  }
}

void startRun(RunKind kind, uint32_t nowMs) {
  if (motion::isFaulty()) {
    Serial.println("[TRAJ] Panne en cours : verifiez les encodeurs (roues a la main), puis redemarrez avec EN.");
    return;
  }
  stepCount = 0;
  if (kind == RunKind::LINE) {
    steps[stepCount++] = {true, lineDistanceMm};
    Serial.printf("[TRAJ] Essai : ligne droite de %.0f mm a %.0f mm/s. Une touche = STOP.\n", lineDistanceMm,
                  lineSpeedMmS);
  } else if (kind == RunKind::TURN) {
    steps[stepCount++] = {false, turnAngleDeg};
    Serial.printf("[TRAJ] Essai : rotation de %.0f deg vers la %s. Une touche = STOP.\n", fabsf(turnAngleDeg),
                  turnAngleDeg > 0.0f ? "gauche" : "droite");
  } else {
    const float turnDeg = (squareSideMm > 0.0f) ? 90.0f : -90.0f;
    for (int side = 0; side < 4; side++) {
      steps[stepCount++] = {true, fabsf(squareSideMm)};
      steps[stepCount++] = {false, turnDeg};
    }
    Serial.printf("[TRAJ] Essai : carre de %.0f mm en tournant a %s. Une touche = STOP.\n", fabsf(squareSideMm),
                  squareSideMm > 0.0f ? "gauche" : "droite");
  }

  motion::resetPose();
  encoders::read(startLeftTicks, startRightTicks);
  maxLeftPct = 0.0f;
  maxRightPct = 0.0f;
  running = kind;
  lastRun = kind;
  measurableRun = RunKind::NONE;
  stepIndex = 0;
  runStartMs = nowMs;
  runTimeoutMs = 2 * expectedDurationMs() + TEST_BENCH_TRAJ_TIMEOUT_MARGIN_MS;
  lastLogMs = nowMs;
  if (sensors::isFaulty()) {
    Serial.println("[TRAJ] ATTENTION : ultrason en panne, pas d'arret devant un obstacle. Main sur l'alim !");
  }
  startStep(0);
}

void printWheelMargin() {
  Serial.printf("[TRAJ] PWM max demandee : gauche %.0f %%, droite %.0f %% (marge avant 100 %% : %.0f %%)\n",
                maxLeftPct, maxRightPct, 100.0f - fmaxf(maxLeftPct, maxRightPct));
  if (fmaxf(maxLeftPct, maxRightPct) >= 90.0f) {
    Serial.println("[TRAJ] PWM proche de 100 % : le regulateur n'a plus de reserve (vitesse trop haute,"
                   " batterie faible ou roue qui frotte).");
  }
}

// Essai fini normalement : on affiche ce que le PAMI croit avoir fait, et ce qu'il faut mesurer.
void finishRun(uint32_t nowMs) {
  const RunKind finished = running;
  running = RunKind::NONE;
  const float seconds = (nowMs - runStartMs) / 1000.0f;
  const Pose pose = motion::pose();

  if (finished == RunKind::LINE) {
    believedValue = believedDistanceMm();
    Serial.printf("[TRAJ] ===== Ligne droite de %.0f mm terminee en %.1f s =====\n", lineDistanceMm, seconds);
    Serial.printf("[TRAJ] Le PAMI croit avoir parcouru %.1f mm (ecart lateral y = %.0f mm, cap %.1f deg).\n",
                  believedValue, pose.yMm, pose.headingDeg);
    printWheelMargin();
    Serial.println("[TRAJ] -> Mesurez la distance REELLE (meme repere sur le PAMI au depart et a l'arrivee),");
    Serial.println("[TRAJ]    puis tapez : m <distance en mm>. Notez aussi l'ecart lateral reel (< 10 mm sur 500 mm).");
  } else if (finished == RunKind::TURN) {
    believedValue = believedAngleDeg();
    Serial.printf("[TRAJ] ===== Rotation de %.0f deg terminee en %.1f s =====\n", fabsf(turnAngleDeg), seconds);
    Serial.printf("[TRAJ] Le PAMI croit avoir tourne de %.1f deg (x = %.0f mm, y = %.0f mm : il doit rester sur place).\n",
                  fabsf(believedValue), pose.xMm, pose.yMm);
    printWheelMargin();
    Serial.println("[TRAJ] -> Mesurez l'angle REEL (repere au sol, ou laser sur un mur : 1 deg = 35 mm a 2 m),");
    Serial.println("[TRAJ]    puis tapez : m <angle en degres>.");
  } else {
    believedValue = sqrtf(pose.xMm * pose.xMm + pose.yMm * pose.yMm);
    lastSquareSideMm = squareSideMm;
    Serial.printf("[TRAJ] ===== Carre de %.0f mm (a %s) termine en %.1f s =====\n", fabsf(squareSideMm),
                  squareSideMm > 0.0f ? "gauche" : "droite", seconds);
    Serial.printf("[TRAJ] Le PAMI croit etre a %.0f mm du depart (x = %.0f, y = %.0f, cap %.1f deg).\n",
                  believedValue, pose.xMm, pose.yMm, pose.headingDeg);
    printWheelMargin();
    Serial.println("[TRAJ] -> Mesurez la distance REELLE entre le repere de depart et le PAMI, puis tapez :");
    Serial.println("[TRAJ]    m <distance en mm>. Notez de quel cote du depart il s'arrete (voir la doc).");
  }
  measurableRun = finished;
}

void stopRun(const char* reason) {
  motion::stop();
  running = RunKind::NONE;
  measurableRun = RunKind::NONE;  // essai incomplet : pas de mesure possible
  motorDriver::brake();
  const Pose pose = motion::pose();
  Serial.printf("[TRAJ] STOP (%s). Position calculee : x = %.0f mm, y = %.0f mm, cap = %.1f deg.\n", reason,
                pose.xMm, pose.yMm, pose.headingDeg);
}

// À chaque tour de loop() pendant un essai : sécurités, puis étape suivante.
void advanceRun(uint32_t nowMs) {
  // En panne, motion dit « mouvement terminé » : il ne faut surtout pas enchaîner les étapes
  // suivantes ni afficher un faux bilan. Le message de panne est affiché par loop().
  if (motion::isFaulty()) {
    running = RunKind::NONE;
    measurableRun = RunKind::NONE;
    return;
  }
  if (nowMs - runStartMs >= runTimeoutMs) {
    stopRun("essai beaucoup plus long que prevu");
    return;
  }
  // Sécurité du banc : un obstacle devant pendant une ligne droite arrête l'essai.
  if (motion::isDriving() && !sensors::isFaulty() && sensors::frontDistanceMm() < OBSTACLE_STOP_DISTANCE_MM) {
    char reason[48];
    snprintf(reason, sizeof(reason), "obstacle a %u mm devant", sensors::frontDistanceMm());
    stopRun(reason);
    return;
  }
  if (!motion::isDone()) {
    return;
  }
  stepIndex++;
  if (stepIndex < stepCount) {
    startStep(stepIndex);
  } else {
    finishRun(nowMs);
  }
}

// --- Mesures tapées avec « m » ---

void printSeries(const char* what, const Series& series) {
  const float spreadPct = (series.max - series.min) / 2.0f / series.mean() * 100.0f;
  Serial.printf("[TRAJ] Serie de %lu %s : rapport moyen %.4f (dispersion +/- %.2f %%)\n",
                (unsigned long)series.count, what, series.mean(), spreadPct);
  if (series.count >= 2 && spreadPct > TEST_BENCH_TRAJ_DISPERSION_MAX_PCT) {
    Serial.println("[TRAJ] Essais trop differents les uns des autres : patinage, roue qui frotte ou repere mal"
                   " place. A regler avant de changer les constantes.");
  }
  if (series.count < 3) {
    Serial.println("[TRAJ] Refaites l'essai (Entree seule) : 3 a 5 mesures donnent une moyenne fiable.");
  }
}

void measureLine(float realMm) {
  const float ratio = realMm / believedValue;
  lineRatios.add(ratio);
  Serial.printf("[TRAJ] Mesure : reel %.0f mm, le PAMI croyait %.1f mm (%+.2f %%).\n", realMm, believedValue,
                (ratio - 1.0f) * 100.0f);
  printSeries("lignes droites", lineRatios);
  const float meanRatio = lineRatios.mean();
  if (fabsf(meanRatio - 1.0f) * 100.0f <= TEST_BENCH_TRAJ_DISTANCE_TOLERANCE_PCT) {
    Serial.printf("[TRAJ] Distance bonne a moins de %.0f %% : WHEEL_DIAMETER_MM n'a pas besoin de changer.\n",
                  TEST_BENCH_TRAJ_DISTANCE_TOLERANCE_PCT);
  } else {
    // Le PAMI calcule la distance avec le diamètre : s'il parcourt vraiment plus que ce qu'il croit,
    // sa roue est plus grande que la valeur de config.h, dans le même rapport.
    Serial.printf("[TRAJ] Diametre conseille : WHEEL_DIAMETER_MM = %.2ff;  (actuel %.2f) // Txxx\n",
                  WHEEL_DIAMETER_MM * meanRatio, WHEEL_DIAMETER_MM);
  }
}

void measureTurn(float realDeg) {
  realDeg = fabsf(realDeg);
  const float believedDeg = fabsf(believedValue);
  const float ratio = believedDeg / realDeg;
  turnRatios.add(ratio);
  Serial.printf("[TRAJ] Mesure : reel %.1f deg, le PAMI croyait %.1f deg (ecart %+.1f deg).\n", realDeg,
                believedDeg, realDeg - believedDeg);
  if (lineRatios.count == 0) {
    Serial.println("[TRAJ] Conseil : reglez d'abord le diametre (lignes droites, l ...) : l'angle en depend.");
  }
  printSeries("rotations", turnRatios);
  const float toleranceDeg = TEST_BENCH_TRAJ_ANGLE_TOLERANCE_DEG_PER_TURN * believedDeg / 360.0f;
  const float meanErrorDeg = believedDeg / turnRatios.mean() - believedDeg;
  if (fabsf(meanErrorDeg) <= toleranceDeg) {
    Serial.printf("[TRAJ] Angle bon a moins de %.0f deg par tour : WHEEL_BASE_MM n'a pas besoin de changer.\n",
                  TEST_BENCH_TRAJ_ANGLE_TOLERANCE_DEG_PER_TURN);
  } else {
    // Angle calculé = arc des roues / entraxe. S'il tourne vraiment plus que ce qu'il croit, son
    // entraxe réel est plus petit que celui de config.h, dans le même rapport.
    Serial.printf("[TRAJ] Entraxe conseille : WHEEL_BASE_MM = %.1ff;  (actuel %.1f) // Txxx\n",
                  WHEEL_BASE_MM * turnRatios.mean(), WHEEL_BASE_MM);
  }
}

void measureSquare(float realGapMm) {
  Series& series = (lastSquareSideMm > 0.0f) ? squaresLeft : squaresRight;
  series.add(realGapMm);
  Serial.printf("[TRAJ] Mesure : arrive a %.0f mm du depart (le PAMI croyait %.0f mm).\n", realGapMm,
                believedValue);
  Serial.printf("[TRAJ] Carres a gauche : %lu, ecart moyen %.0f mm | a droite : %lu, ecart moyen %.0f mm\n",
                (unsigned long)squaresLeft.count, squaresLeft.mean(), (unsigned long)squaresRight.count,
                squaresRight.mean());
  if (realGapMm > TEST_BENCH_TRAJ_SQUARE_CLOSURE_MAX_MM) {
    Serial.printf("[TRAJ] Plus de %.0f mm : voir docs/reglage-regulation.md, \"carres dans les deux sens\".\n",
                  TEST_BENCH_TRAJ_SQUARE_CLOSURE_MAX_MM);
  } else {
    Serial.printf("[TRAJ] Retour a moins de %.0f mm du depart : bon.\n", TEST_BENCH_TRAJ_SQUARE_CLOSURE_MAX_MM);
  }
}

void handleMeasure(bool hasValue, float value) {
  if (!hasValue || value <= 0.0f) {
    Serial.println("[TRAJ] Tapez la valeur mesuree, par exemple : m 503");
    return;
  }
  switch (measurableRun) {
    case RunKind::LINE: measureLine(value); break;
    case RunKind::TURN: measureTurn(value); break;
    case RunKind::SQUARE: measureSquare(value); break;
    default:
      Serial.println("[TRAJ] Pas d'essai termine a mesurer (essai interrompu, ou deja mesure).");
      return;
  }
  measurableRun = RunKind::NONE;  // une seule mesure par essai
}

// --- Lecture de la ligne tapée ---

char* skipSpaces(char* text) {
  while (*text == ' ') {
    text++;
  }
  return text;
}

// Lit un nombre (virgule ou point acceptés). Faux s'il n'y a pas de nombre, ou s'il reste autre chose.
bool parseNumber(char* text, float& value) {
  for (char* c = text; *c != '\0'; c++) {
    if (*c == ',') {
      *c = '.';
    }
  }
  char* end = nullptr;
  value = strtof(text, &end);
  if (end == text) {
    return false;
  }
  return *skipSpaces(end) == '\0';
}

bool inRange(float value, float minValue, float maxValue, const char* what) {
  if (value >= minValue && value <= maxValue) {
    return true;
  }
  Serial.printf("[TRAJ] %s : entre %.0f et %.0f.\n", what, minValue, maxValue);
  return false;
}

void executeLine(uint32_t nowMs) {
  line[lineLength] = '\0';
  char* text = skipSpaces(line);
  if (*text == '\0') {  // Entrée seule : refaire le dernier essai
    if (lastRun == RunKind::NONE) {
      printHelp();
    } else {
      startRun(lastRun, nowMs);
    }
    return;
  }
  const char command = (char)tolower(*text);
  char* argument = skipSpaces(text + 1);
  float value = 0.0f;
  const bool hasValue = (*argument != '\0') && parseNumber(argument, value);
  if (*argument != '\0' && !hasValue) {
    Serial.println("[TRAJ] Nombre illisible. Exemple : l 1000");
    return;
  }

  switch (command) {
    case 'l':
      if (hasValue && !inRange(value, TEST_BENCH_TRAJ_MIN_DISTANCE_MM, TEST_BENCH_TRAJ_MAX_DISTANCE_MM,
                               "Distance en mm")) {
        return;
      }
      if (hasValue) {
        lineDistanceMm = value;
      }
      startRun(RunKind::LINE, nowMs);
      break;
    case 'r':
      if (hasValue && (value == 0.0f || !inRange(fabsf(value), 1.0f, TEST_BENCH_TRAJ_MAX_ANGLE_DEG,
                                                 "Angle en degres (signe - : vers la droite)"))) {
        return;
      }
      if (hasValue) {
        turnAngleDeg = value;
      }
      startRun(RunKind::TURN, nowMs);
      break;
    case 'c':
      if (hasValue && !inRange(fabsf(value), TEST_BENCH_TRAJ_MIN_SQUARE_MM, TEST_BENCH_TRAJ_MAX_SQUARE_MM,
                               "Cote du carre en mm (signe - : en tournant a droite)")) {
        return;
      }
      if (hasValue) {
        squareSideMm = value;
      }
      startRun(RunKind::SQUARE, nowMs);
      break;
    case 'v':
      if (!hasValue) {
        Serial.printf("[TRAJ] Vitesse des lignes droites : %.0f mm/s. Pour changer : v 100\n", lineSpeedMmS);
        return;
      }
      if (inRange(value, MOTION_MIN_SPEED_MM_S, DRIVE_SPEED_MM_S, "Vitesse en mm/s")) {
        lineSpeedMmS = value;
        Serial.printf("[TRAJ] Vitesse des lignes droites : %.0f mm/s\n", lineSpeedMmS);
      }
      break;
    case 'm':
      handleMeasure(hasValue, value);
      break;
    case 'n':
      lineRatios = Series();
      turnRatios = Series();
      squaresLeft = Series();
      squaresRight = Series();
      Serial.println("[TRAJ] Nouvelle serie : mesures precedentes oubliees.");
      break;
    case 'z':
      motion::resetPose();
      Serial.println("[TRAJ] Position remise a zero.");
      break;
    case 'i':
      printSettings();
      break;
    case 'h':
    case '?':
      printHelp();
      break;
    default:
      Serial.println("[TRAJ] Commande inconnue.");
      printHelp();
      break;
  }
}

void handleChar(char c, uint32_t nowMs) {
  // Beaucoup de moniteurs envoient \r puis \n pour une seule touche Entrée : on n'en compte qu'une.
  // Sinon, le \n arriverait juste après le départ de l'essai et l'arrêterait aussitôt.
  if (c == '\n' && lastWasCr) {
    lastWasCr = false;
    return;
  }
  lastWasCr = (c == '\r');

  // Sécurité : pendant un essai, n'importe quelle touche arrête tout.
  if (running != RunKind::NONE) {
    stopRun("touche");
    lineLength = 0;
    return;
  }
  if (c == '\r' || c == '\n') {
    Serial.println();
    executeLine(nowMs);
    lineLength = 0;
    return;
  }
  if (c == '\b' || c == 127) {  // effacer le dernier caractère
    if (lineLength > 0) {
      lineLength--;
      Serial.print("\b \b");
    }
    return;
  }
  if (c < 32 || c > 126 || lineLength >= sizeof(line) - 1) {
    return;
  }
  line[lineLength++] = c;
  Serial.print(c);  // écho : le moniteur n'affiche pas ce qu'on tape
}

// Une ligne de mesures pendant l'essai : vitesse, PWM et position de chaque instant.
void logMeasures(uint32_t nowMs) {
  const Pose pose = motion::pose();
  const WheelCommand command = motion::command();
  float leftMm = 0.0f;
  float rightMm = 0.0f;
  wheelDistancesMm(leftMm, rightMm);
  Serial.printf("[ODO] t = %4.1f s | roue G %6.0f mm D %6.0f mm | PWM G %4.0f D %4.0f %% | x %5.0f y %5.0f mm"
                " cap %6.1f deg | US %u mm\n",
                (nowMs - runStartMs) / 1000.0f, leftMm, rightMm, command.leftPct, command.rightPct, pose.xMm,
                pose.yMm, pose.headingDeg, sensors::frontDistanceMm());
}

}  // namespace

void setup() {
  // En tout premier : moteurs freinés. Rien ne bouge avant une commande.
  motorDriver::begin();

  Serial.begin(SERIAL_BAUD);
  Serial.println();
  Serial.println("[BANC] ===== BANC DE TEST DES TRAJECTOIRES (ligne, rotation, carre) =====");
  Serial.println("[BANC] PAS D'EVITEMENT COMPLET : NE JAMAIS UTILISER EN MATCH.");
  Serial.println("[BANC] Apres le test : pio run -t upload (remet le firmware).");
  printResetReason();

  encoders::begin();
  sensors::begin();
  motion::begin();

  printSettings();
  printHelp();
  Serial.println("[TRAJ] Moteurs arretes. Premier essai sur cales, par exemple : l 300 puis Entree.");
}

void loop() {
  const uint32_t nowMs = millis();

  while (Serial.available() > 0) {
    handleChar((char)Serial.read(), nowMs);
  }

  sensors::update(nowMs);
  if (running != RunKind::NONE) {
    advanceRun(nowMs);
  }

  if (motion::isFaulty() && !faultReported) {
    faultReported = true;
    running = RunKind::NONE;
    measurableRun = RunKind::NONE;
    Serial.println("[TRAJ] PANNE : roue bloquee ou encodeur inverse. Tout est arrete.");
    Serial.println("[TRAJ] Verifiez le sens des encodeurs (roues a la main), puis redemarrez avec EN.");
  }
  if (sensors::isFaulty() && !sensorWarningPrinted) {
    sensorWarningPrinted = true;
    Serial.println("[TRAJ] Ultrason en panne (aucune mesure) : le banc ne s'arretera pas devant un obstacle.");
  }

  // Pas de safety dans ce banc : rien ne bloque la commande (false). Hors essai, freins serrés.
  motion::update(nowMs, false);
  const WheelCommand command = (running != RunKind::NONE) ? motion::command() : WheelCommand();
  motorDriver::setWheelPercent(command.leftPct, command.rightPct);

  if (running != RunKind::NONE) {
    maxLeftPct = fmaxf(maxLeftPct, fabsf(command.leftPct));
    maxRightPct = fmaxf(maxRightPct, fabsf(command.rightPct));
    if (nowMs - lastLogMs >= TEST_BENCH_ODOMETRY_LOG_PERIOD_MS) {
      lastLogMs = nowMs;
      logMeasures(nowMs);
    }
  }
}

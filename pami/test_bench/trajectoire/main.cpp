// Banc de test des trajectoires : ligne droite, rotation et carré aux dimensions choisies au clavier,
// puis calcul des corrections d'étalonnage à partir de ce que vous mesurez au mètre ruban.
//
// !!! PROGRAMME DE TEST : AUCUN ÉVITEMENT. NE JAMAIS L'UTILISER EN MATCH. !!!
// Après le test, remettre le firmware : pio run -t upload (sans fil : pio run -e esp32dev_wifi -t upload)
//
// Téléverser : pio run -e test_trajectoire -t upload -t monitor (maintenir BOOT pendant « Connecting »)
// Sans fil (D010) : pio run -e test_trajectoire_wifi -t upload, puis pio device monitor -e test_trajectoire_wifi.
//   Téléversement accepté seulement hors essai. Les commandes marchent aussi depuis le moniteur Wi-Fi.
// Méthode complète (ordre des essais, comment corriger) : docs/reglage-regulation.md
//
// CE QUE CE BANC RÉPOND
// À chaque essai, il compare trois choses :
// - ce que vous avez COMMANDÉ (ex. 1000 mm) ;
// - ce que le PAMI CROIT avoir fait (calculé avec ses encodeurs) ;
// - ce qu'il a VRAIMENT fait (vous le mesurez, puis vous le tapez avec « m »).
// « Cru » est relu au moment où vous tapez « m », PAMI arrêté : le petit glissement après la fin
// du mouvement compte, comme pour votre mesure (correction du 21/11/2026, T003).
// Commandé ≠ cru : c'est le contrôle (motion, régulateur) qui a mal travaillé.
// Cru ≠ réel : le contrôle a bien travaillé, mais le PAMI mesure mal : c'est l'ÉTALONNAGE
// (diamètre des roues, entraxe). Le banc calcule alors la valeur à mettre dans config.h.
// Il utilise le VRAI code de déplacement du match (motion, régulateur de vitesse, odométrie).
//
// MATÉRIEL ET MONTAGE
// - Comme test_odometrie : MDD3A, encodeurs, alimentation des moteurs, masse commune.
// - Pas d'ultrason : ce banc règle le déplacement, il ne regarde jamais devant lui (comme
//   test_odometrie et test_pid). C'est à vous de garder la place dégagée.
//
// SÉCURITÉ
// - Au démarrage, rien ne bouge avant une commande.
// - Premier essai SUR CALES. Au sol : BAU câblé (règle d'or n° 2), place dégagée (aucun obstacle
//   n'arrête le PAMI), quelqu'un prêt à couper l'alimentation.
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
//   m 503    MESURE réelle du dernier essai : calcule la correction. Ligne : distance en mm (positive).
//            Rotation : angle en degrés, signe libre (m 358 = m -358). Carré : écart au départ en mm,
//            AVEC SIGNE : + si le PAMI s'arrête à gauche du départ, - à droite, 0 si pile dessus.
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
#include "console.h"
#include "encoders.h"
#include "motion.h"
#include "motor_driver.h"
#include "wireless.h"

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
  float sumAbs = 0.0f;  // somme des valeurs sans leur signe (carrés : écart moyen, quel que soit le côté)
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
    sumAbs += fabsf(value);
    count++;
  }
  float mean() const { return (count > 0) ? sum / count : 0.0f; }
  float meanAbs() const { return (count > 0) ? sumAbs / count : 0.0f; }
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
  Console.printf("[BANC] Cause du redemarrage : %s\n", reason);
}

void printHelp() {
  Console.println("[TRAJ] Commandes : une lettre, un nombre si besoin, puis Entree. Exemples :");
  Console.printf("[TRAJ]   l 1000   ligne droite de 1000 mm (%.0f a %.0f)\n", TEST_BENCH_TRAJ_MIN_DISTANCE_MM,
                 TEST_BENCH_TRAJ_MAX_DISTANCE_MM);
  Console.println("[TRAJ]   r 90     rotation de 90 deg vers la gauche ; r -90 vers la droite ; r 360 = un tour");
  Console.println("[TRAJ]   c 300    carre de 300 mm en tournant a gauche ; c -300 en tournant a droite");
  Console.printf("[TRAJ]   v 100    vitesse des lignes droites, en mm/s (%.0f a %.0f)\n", MOTION_MIN_SPEED_MM_S,
                 DRIVE_SPEED_MM_S);
  Console.println("[TRAJ]   m 503    MESURE reelle du dernier essai : mm (ligne), deg (rotation, signe libre),");
  Console.println("[TRAJ]            mm avec signe pour un carre (+ arrete a gauche du depart, - a droite)");
  Console.println("[TRAJ]   Entree seule : refaire le meme essai    n : nouvelle serie de mesures");
  Console.println("[TRAJ]   z : position a zero    i : reglages    h : aide");
  Console.println("[TRAJ] Chiffres : pave numerique, ou Maj + touche sur un clavier AZERTY.");
  Console.println("[TRAJ] Pendant un essai : N'IMPORTE QUELLE touche = STOP.");
}

void printSettings() {
  Console.printf("[TRAJ] Etalonnage : roue %.2f mm, entraxe %.1f mm, %ld ticks/tour (%.4f mm/tick)\n",
                 WHEEL_DIAMETER_MM, WHEEL_BASE_MM, (long)ENCODER_TICKS_PER_WHEEL_TURN, MM_PER_TICK);
  Console.printf("[TRAJ] Vitesses : ligne %.0f mm/s, rotation %.0f mm/s par roue, acceleration %.0f mm/s2\n",
                 lineSpeedMmS, TURN_SPEED_MM_S, MOTION_ACCEL_MM_S2);
  Console.printf("[TRAJ] Regulateur : KP %.3f, KI %.3f | anticipation G %.1f %% + %.3f, D %.1f %% + %.3f\n",
                 SPEED_KP_PCT_PER_MM_S, SPEED_KI_PCT_PER_MM, LEFT_MOTOR_DEADBAND_PCT, LEFT_MOTOR_PCT_PER_MM_S,
                 RIGHT_MOTOR_DEADBAND_PCT, RIGHT_MOTOR_PCT_PER_MM_S);
  Console.printf("[TRAJ] Derniers choix : ligne %.0f mm, rotation %.0f deg, carre %.0f mm\n", lineDistanceMm,
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
    Console.println("[TRAJ] Panne en cours : verifiez les encodeurs (roues a la main), puis redemarrez avec EN.");
    return;
  }
  stepCount = 0;
  if (kind == RunKind::LINE) {
    steps[stepCount++] = {true, lineDistanceMm};
    Console.printf("[TRAJ] Essai : ligne droite de %.0f mm a %.0f mm/s. Une touche = STOP.\n", lineDistanceMm,
                   lineSpeedMmS);
  } else if (kind == RunKind::TURN) {
    steps[stepCount++] = {false, turnAngleDeg};
    Console.printf("[TRAJ] Essai : rotation de %.0f deg vers la %s. Une touche = STOP.\n", fabsf(turnAngleDeg),
                   turnAngleDeg > 0.0f ? "gauche" : "droite");
  } else {
    const float turnDeg = (squareSideMm > 0.0f) ? 90.0f : -90.0f;
    for (int side = 0; side < 4; side++) {
      steps[stepCount++] = {true, fabsf(squareSideMm)};
      steps[stepCount++] = {false, turnDeg};
    }
    Console.printf("[TRAJ] Essai : carre de %.0f mm en tournant a %s. Une touche = STOP.\n", fabsf(squareSideMm),
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
  startStep(0);
}

void printWheelMargin() {
  Console.printf("[TRAJ] PWM max demandee : gauche %.0f %%, droite %.0f %% (marge avant 100 %% : %.0f %%)\n",
                 maxLeftPct, maxRightPct, 100.0f - fmaxf(maxLeftPct, maxRightPct));
  if (fmaxf(maxLeftPct, maxRightPct) >= 90.0f) {
    Console.println("[TRAJ] PWM proche de 100 % : le regulateur n'a plus de reserve (vitesse trop haute,"
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
    Console.printf("[TRAJ] ===== Ligne droite de %.0f mm terminee en %.1f s =====\n", lineDistanceMm, seconds);
    Console.printf("[TRAJ] Fin du mouvement : le PAMI croit avoir parcouru %.1f mm (ecart lateral y = %.0f mm, cap %.1f deg).\n",
                   believedValue, pose.yMm, pose.headingDeg);
    printWheelMargin();
    Console.println("[TRAJ] -> Mesurez la distance REELLE (meme repere sur le PAMI au depart et a l'arrivee),");
    Console.println("[TRAJ]    puis tapez m et la distance en mm, par exemple : m 947");
    Console.println("[TRAJ]    Notez aussi l'ecart lateral reel (moins de 10 mm sur 500 mm).");
  } else if (finished == RunKind::TURN) {
    believedValue = believedAngleDeg();
    Console.printf("[TRAJ] ===== Rotation de %.0f deg terminee en %.1f s =====\n", fabsf(turnAngleDeg), seconds);
    Console.printf("[TRAJ] Le PAMI croit avoir tourne de %.1f deg (x = %.0f mm, y = %.0f mm : il doit rester sur place).\n",
                   fabsf(believedValue), pose.xMm, pose.yMm);
    printWheelMargin();
    Console.println("[TRAJ] -> Mesurez l'angle REEL (repere au sol, ou laser sur un mur : 1 deg = 35 mm a 2 m),");
    Console.println("[TRAJ]    puis tapez m et l'angle en degres, par exemple : m 358");
  } else {
    believedValue = sqrtf(pose.xMm * pose.xMm + pose.yMm * pose.yMm);
    lastSquareSideMm = squareSideMm;
    Console.printf("[TRAJ] ===== Carre de %.0f mm (a %s) termine en %.1f s =====\n", fabsf(squareSideMm),
                   squareSideMm > 0.0f ? "gauche" : "droite", seconds);
    Console.printf("[TRAJ] Le PAMI croit etre a %.0f mm du depart (x = %.0f, y = %.0f, cap %.1f deg).\n",
                   believedValue, pose.xMm, pose.yMm, pose.headingDeg);
    printWheelMargin();
    Console.println("[TRAJ] -> Mesurez la distance REELLE entre le repere de depart et le PAMI, avec un SIGNE pour le cote :");
    Console.println("[TRAJ]    + s'il s'arrete a GAUCHE du depart, - s'il s'arrete a DROITE (vu de derriere le PAMI");
    Console.println("[TRAJ]    au depart, en regardant dans son sens de depart). Exemples : m 12, m -8, m 0.");
  }
  measurableRun = finished;
}

void stopRun(const char* reason) {
  motion::stop();
  running = RunKind::NONE;
  measurableRun = RunKind::NONE;  // essai incomplet : pas de mesure possible
  motorDriver::brake();
  const Pose pose = motion::pose();
  Console.printf("[TRAJ] STOP (%s). Position calculee : x = %.0f mm, y = %.0f mm, cap = %.1f deg.\n", reason,
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
  Console.printf("[TRAJ] Serie de %lu %s : rapport moyen %.4f (dispersion +/- %.2f %%)\n",
                 (unsigned long)series.count, what, series.mean(), spreadPct);
  if (series.count >= 2 && spreadPct > TEST_BENCH_TRAJ_DISPERSION_MAX_PCT) {
    Console.println("[TRAJ] Essais trop differents les uns des autres : patinage, roue qui frotte ou repere mal"
                    " place. A regler avant de changer les constantes.");
  }
  if (series.count < 3) {
    Console.println("[TRAJ] Refaites l'essai (Entree seule) : 3 a 5 mesures donnent une moyenne fiable.");
  }
}

// Chaque mesure vérifie sa valeur. Elle renvoie faux si la valeur est refusée : l'essai reste alors
// mesurable, et on peut retaper la bonne valeur.
//
// Ce que le PAMI croit est relu au moment de la mesure (« m »), PAMI arrêté depuis longtemps :
// après « mouvement fini », les roues glissent encore de quelques mm (freinage), et la règle mesure ce
// glissement. Comparer avec la valeur de fin de mouvement faussait l'étalonnage (T003).
// Les encodeurs comptent depuis le début de l'essai : « z » (position à zéro) ne change rien ici.
bool measureLine(float realMm) {
  // Le PAMI ne recule jamais : une distance parcourue est forcément positive.
  if (realMm <= 0.0f) {
    Console.println("[TRAJ] Ligne droite : tapez la distance parcourue, positive. Exemple : m 947");
    return false;
  }
  const float atRestMm = believedDistanceMm();
  const float ratio = realMm / atRestMm;
  lineRatios.add(ratio);
  Console.printf("[TRAJ] Mesure : reel %.0f mm, le PAMI croyait %.1f mm a l'arret (%+.2f %%).\n", realMm,
                 atRestMm, (ratio - 1.0f) * 100.0f);
  Console.printf("[TRAJ] Glissement apres la fin du mouvement : %.1f mm (%.1f mm a la fin du mouvement).\n",
                 atRestMm - believedValue, believedValue);
  printSeries("lignes droites", lineRatios);
  const float meanRatio = lineRatios.mean();
  if (fabsf(meanRatio - 1.0f) * 100.0f <= TEST_BENCH_TRAJ_DISTANCE_TOLERANCE_PCT) {
    Console.printf("[TRAJ] Distance bonne a moins de %.0f %% : WHEEL_DIAMETER_MM n'a pas besoin de changer.\n",
                   TEST_BENCH_TRAJ_DISTANCE_TOLERANCE_PCT);
  } else {
    // Le PAMI calcule la distance avec le diamètre : s'il parcourt vraiment plus que ce qu'il croit,
    // sa roue est plus grande que la valeur de config.h, dans le même rapport.
    Console.printf("[TRAJ] Diametre conseille : WHEEL_DIAMETER_MM = %.2ff;  (actuel %.2f) // Txxx\n",
                   WHEEL_DIAMETER_MM * meanRatio, WHEEL_DIAMETER_MM);
  }
  return true;
}

// Même principe que measureLine : l'angle cru est relu PAMI arrêté, glissement de fin compris.
// On compare des angles SANS leur signe : le sens de rotation est connu par la commande
// (r -360 = vers la droite). « m -357 » et « m 357 » donnent donc le même résultat.
bool measureTurn(float realDeg) {
  if (realDeg == 0.0f) {
    Console.println("[TRAJ] Rotation : tapez l'angle reellement tourne, par exemple : m 358 (ou m -358)");
    return false;
  }
  const float believedSignedDeg = believedAngleDeg();
  if (realDeg * believedSignedDeg < 0.0f) {
    Console.println("[TRAJ] Signe de la mesure oppose au sens de la rotation : seul l'angle compte, le signe"
                    " est ignore.");
  }
  realDeg = fabsf(realDeg);
  const float believedDeg = fabsf(believedSignedDeg);
  const float ratio = believedDeg / realDeg;
  turnRatios.add(ratio);
  Console.printf("[TRAJ] Mesure : reel %.1f deg, le PAMI croyait %.1f deg a l'arret (ecart %+.1f deg).\n", realDeg,
                 believedDeg, realDeg - believedDeg);
  Console.printf("[TRAJ] Glissement apres la fin du mouvement : %.1f deg (%.1f deg a la fin du mouvement).\n",
                 believedDeg - fabsf(believedValue), fabsf(believedValue));
  if (lineRatios.count == 0) {
    Console.println("[TRAJ] Conseil : reglez d'abord le diametre (lignes droites, l ...) : l'angle en depend.");
  }
  printSeries("rotations", turnRatios);
  const float toleranceDeg = TEST_BENCH_TRAJ_ANGLE_TOLERANCE_DEG_PER_TURN * believedDeg / 360.0f;
  const float meanErrorDeg = believedDeg / turnRatios.mean() - believedDeg;
  if (fabsf(meanErrorDeg) <= toleranceDeg) {
    Console.printf("[TRAJ] Angle bon a moins de %.0f deg par tour : WHEEL_BASE_MM n'a pas besoin de changer.\n",
                   TEST_BENCH_TRAJ_ANGLE_TOLERANCE_DEG_PER_TURN);
  } else {
    // Angle calculé = arc des roues / entraxe. S'il tourne vraiment plus que ce qu'il croit, son
    // entraxe réel est plus petit que celui de config.h, dans le même rapport.
    Console.printf("[TRAJ] Entraxe conseille : WHEEL_BASE_MM = %.1ff;  (actuel %.1f) // Txxx\n",
                   WHEEL_BASE_MM * turnRatios.mean(), WHEEL_BASE_MM);
  }
  return true;
}

const char* gapSideName(float signedGapMm) {
  if (signedGapMm > 0.0f) {
    return "a gauche";
  }
  return (signedGapMm < 0.0f) ? "a droite" : "pile au depart";
}

void printSquareSeries(const char* turning, const Series& series) {
  if (series.count == 0) {
    Console.printf("[TRAJ] Carres en tournant a %s : aucun.\n", turning);
    return;
  }
  Console.printf("[TRAJ] Carres en tournant a %s : %lu, ecart moyen %.0f mm, cote moyen %+.0f mm (%s)\n", turning,
                 (unsigned long)series.count, series.meanAbs(), series.mean(), gapSideName(series.mean()));
}

// Carré : écart AVEC SIGNE, + = arrêté à gauche du départ, - = à droite (vu de derrière le PAMI au
// départ, comme y dans l'odométrie). 0 est une vraie mesure : le carré s'est refermé pile.
// Le signe sert à la méthode des carrés dans les deux sens (docs/reglage-regulation.md, § 8).
bool measureSquare(float signedGapMm) {
  Series& series = (lastSquareSideMm > 0.0f) ? squaresLeft : squaresRight;
  series.add(signedGapMm);
  Console.printf("[TRAJ] Mesure : arrive a %.0f mm du depart, %s (le PAMI croyait %.0f mm).\n", fabsf(signedGapMm),
                 gapSideName(signedGapMm), believedValue);
  printSquareSeries("gauche", squaresLeft);
  printSquareSeries("droite", squaresRight);
  if (fabsf(signedGapMm) > TEST_BENCH_TRAJ_SQUARE_CLOSURE_MAX_MM) {
    Console.printf("[TRAJ] Plus de %.0f mm : voir docs/reglage-regulation.md, \"carres dans les deux sens\".\n",
                   TEST_BENCH_TRAJ_SQUARE_CLOSURE_MAX_MM);
  } else {
    Console.printf("[TRAJ] Retour a moins de %.0f mm du depart : bon.\n", TEST_BENCH_TRAJ_SQUARE_CLOSURE_MAX_MM);
  }

  // Lecture de la méthode des carrés, quand chaque sens a au moins 3 mesures.
  if (squaresLeft.count < 3 || squaresRight.count < 3) {
    return true;
  }
  if (squaresLeft.meanAbs() <= TEST_BENCH_TRAJ_SQUARE_CLOSURE_MAX_MM &&
      squaresRight.meanAbs() <= TEST_BENCH_TRAJ_SQUARE_CLOSURE_MAX_MM) {
    Console.println("[TRAJ] Lecture : les deux sens referment bien. Rien a corriger.");
  } else if (squaresLeft.mean() * squaresRight.mean() > 0.0f) {
    Console.println("[TRAJ] Lecture : les deux sens finissent du MEME cote : les deux roues n'ont pas le meme"
                    " diametre (doc, \"carres dans les deux sens\").");
  } else {
    Console.println("[TRAJ] Lecture : les deux sens finissent de cotes OPPOSES : l'entraxe est faux"
                    " (refaire les rotations r 360 / r -360).");
  }
  return true;
}

void handleMeasure(bool hasValue, float value) {
  if (!hasValue) {
    Console.println("[TRAJ] Tapez la valeur mesuree, par exemple : m 947 (ligne), m -358 (rotation), m -8 (carre)");
    return;
  }
  bool accepted = false;
  switch (measurableRun) {
    case RunKind::LINE: accepted = measureLine(value); break;
    case RunKind::TURN: accepted = measureTurn(value); break;
    case RunKind::SQUARE: accepted = measureSquare(value); break;
    default:
      Console.println("[TRAJ] Pas d'essai termine a mesurer (essai interrompu, ou deja mesure).");
      return;
  }
  if (accepted) {
    measurableRun = RunKind::NONE;  // une seule mesure par essai
  }
}

// --- Lecture de la ligne tapée ---

char* skipSpaces(char* text) {
  while (*text == ' ') {
    text++;
  }
  return text;
}

// Lit un nombre (virgule ou point acceptés). Faux s'il n'y a pas de nombre, ou s'il reste autre chose.
// Les chevrons sont ignorés : « m <925> » vaut « m 925 » (les anciens messages du banc montraient
// « m <distance en mm> », et des mesures ont été perdues ainsi le 21/11/2026).
bool parseNumber(char* text, float& value) {
  for (char* c = text; *c != '\0'; c++) {
    if (*c == ',') {
      *c = '.';
    } else if (*c == '<' || *c == '>') {
      *c = ' ';
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
  Console.printf("[TRAJ] %s : entre %.0f et %.0f.\n", what, minValue, maxValue);
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
    Console.println("[TRAJ] Nombre illisible. Exemples : l 1000 (essai), m 947 (mesure).");
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
        Console.printf("[TRAJ] Vitesse des lignes droites : %.0f mm/s. Pour changer : v 100\n", lineSpeedMmS);
        return;
      }
      if (inRange(value, MOTION_MIN_SPEED_MM_S, DRIVE_SPEED_MM_S, "Vitesse en mm/s")) {
        lineSpeedMmS = value;
        Console.printf("[TRAJ] Vitesse des lignes droites : %.0f mm/s\n", lineSpeedMmS);
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
      Console.println("[TRAJ] Nouvelle serie : mesures precedentes oubliees.");
      break;
    case 'z':
      motion::resetPose();
      Console.println("[TRAJ] Position remise a zero.");
      break;
    case 'i':
      printSettings();
      break;
    case 'h':
    case '?':
      printHelp();
      break;
    default:
      Console.println("[TRAJ] Commande inconnue.");
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

// Une ligne de mesures pendant l'essai : vitesse, PWM et position de chaque instant.
void logMeasures(uint32_t nowMs) {
  const Pose pose = motion::pose();
  const WheelCommand command = motion::command();
  float leftMm = 0.0f;
  float rightMm = 0.0f;
  wheelDistancesMm(leftMm, rightMm);
  Console.printf("[ODO] t = %4.1f s | roue G %6.0f mm D %6.0f mm | PWM G %4.0f D %4.0f %% | x %5.0f y %5.0f mm"
                 " cap %6.1f deg\n",
                 (nowMs - runStartMs) / 1000.0f, leftMm, rightMm, command.leftPct, command.rightPct, pose.xMm,
                 pose.yMm, pose.headingDeg);
}

}  // namespace

void setup() {
  // En tout premier : moteurs freinés. Rien ne bouge avant une commande.
  motorDriver::begin();

  Serial.begin(SERIAL_BAUD);
  Console.println();
  Console.println("[BANC] ===== BANC DE TEST DES TRAJECTOIRES (ligne, rotation, carre) =====");
  Console.println("[BANC] AUCUN EVITEMENT (pas d'ultrason) : NE JAMAIS UTILISER EN MATCH.");
  Console.println("[BANC] Apres le test : remettre le firmware (pio run -t upload, ou -e esp32dev_wifi).");
  printResetReason();

  encoders::begin();
  motion::begin();

  printSettings();
  printHelp();
  Console.println("[TRAJ] Moteurs arretes. Premier essai sur cales, par exemple : l 300 puis Entree.");
  wireless::begin();
}

void loop() {
  const uint32_t nowMs = millis();

  while (Console.available() > 0) {
    handleChar((char)Console.read(), nowMs);
  }

  if (running != RunKind::NONE) {
    advanceRun(nowMs);
  }

  if (motion::isFaulty() && !faultReported) {
    faultReported = true;
    running = RunKind::NONE;
    measurableRun = RunKind::NONE;
    Console.println("[TRAJ] PANNE : roue bloquee ou encodeur inverse. Tout est arrete.");
    Console.println("[TRAJ] Verifiez le sens des encodeurs (roues a la main), puis redemarrez avec EN.");
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

  wireless::update(nowMs, running == RunKind::NONE);  // téléversement seulement hors essai
}

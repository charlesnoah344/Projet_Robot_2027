// Banc de test du régulateur de vitesse des roues (logic/speed_controller.h).
//
// !!! PROGRAMME DE TEST : AUCUN ÉVITEMENT. NE JAMAIS L'UTILISER EN MATCH. !!!
// Après le test, remettre le firmware : pio run -t upload
//
// Téléverser : pio run -e test_pid -t upload -t monitor (maintenir BOOT pendant « Connecting »)
// Méthode complète (à quoi sert chaque essai, ordre des réglages) : docs/reglage-regulation.md
//
// CE QUE CE BANC TESTE
// Le VRAI régulateur du match (classe SpeedController, valeurs de config.h), seul : chaque roue
// reçoit une consigne de vitesse fixe, sans le profil en trapèze ni la correction de cap de
// motion. On voit donc exactement ce que fait le régulateur : en combien de temps il atteint la
// consigne, s'il la dépasse, s'il la tient.
// Le régulateur = anticipation (zone morte + pente) + P + I, avec anti-emballement de l'intégrale.
// Pas de terme D : il amplifierait le bruit de la mesure de vitesse (voir « mesure de vitesse »).
// Pour le comportement complet en déplacement (trapèze, cap) : banc test_odometrie.
//
// MATÉRIEL ET MONTAGE
// - Comme test_odometrie : ESP32 en USB ; MDD3A, M1A/M1B sur GPIO 25/26, M2A/M2B sur GPIO 33/32 ;
//   encodeurs, gauche A/B sur GPIO 27/14, droit sur 13/4 ; alim de labo à 6 V sur VB+/VB- ;
//   masse commune.
// - Avant tout : vérification de la page 7 du PDF de câblage (chaque roue tournée à la main vers
//   l'avant fait monter son compteur).
//
// SÉCURITÉ
// - Au démarrage, rien ne bouge. Mode AU SOL par défaut : chaque essai s'arrête après 800 mm.
// - Premiers essais SUR CALES (touche k), roues en l'air.
// - Au sol : BAU câblé obligatoire (règle d'or n° 2), 1,5 m dégagé devant.
// - Pendant un essai, N'IMPORTE QUELLE touche = STOP.
// - Roue bloquée ou encodeur inversé (même règle que motion) : tout s'arrête, redémarrer avec EN.
//
// COMMANDES : des lettres seulement (sur un clavier AZERTY, les chiffres demandent Maj)
//   g / d / a : roue gauche seule, roue droite seule, les deux roues
//   e : échelon      0 -> V pendant 3 s           (temps de montée, dépassement, erreur)
//   s : escalier     50, 100, 150, 200 mm/s       (mesure l'anticipation de chaque roue)
//   c : constante    V pendant 8 s                (freiner la roue au doigt : l'intégrale compense)
//   p / m (ou + / -) : V plus ou moins 25 mm/s     n : sens avant / arrière
//   k : AU SOL / SUR CALES   v : courbes CSV   i : réglages   h : aide
//
// CE QUE LE BANC AFFICHE
// - Pendant l'essai, toutes les 200 ms :
//     [PID] t = 0.4 s | G cible  150 mesure  147 PWM   41 % | D cible  150 mesure  152 PWM   39 %
// - Avec v : une ligne par cycle de 20 ms, à coller dans un tableur (séparateur ;) pour tracer
//   les courbes :  CSV;t_ms;cibleG;mesureG;pwmG;cibleD;mesureD;pwmD
// - À la fin, un BILAN par roue :
//   - échelon : temps de montée à 90 %, dépassement, vitesse moyenne et erreur en régime établi,
//     ondulation (min et max), PWM moyenne et part de la correction P + I dans cette PWM ;
//   - escalier : la PWM de chaque palier, puis la zone morte et la pente mesurées, à reporter
//     dans config.h.
// - Mesure de vitesse : 1 tick = 0,1 mm. Sur un cycle de 20 ms, 1 tick d'écart = 5 mm/s.
//   L'affichage et le bilan utilisent donc la moyenne des 3 dernières mesures (60 ms).
//   Le régulateur, lui, reçoit la mesure brute de chaque cycle, comme en match.
//
// PROTOCOLE
//  1. Sur cales (k), « a » puis « e » : échelon à 150 mm/s. Puis « g » « e », et « d » « e ».
//  2. Sur cales, « c » : freiner LÉGÈREMENT une roue au doigt. La correction monte et la vitesse
//     revient vers la consigne ; au relâchement, petit dépassement puis retour. Ne pas bloquer la
//     roue plus de 0,6 s : le banc coupe (c'est voulu).
//  3. Au sol, « a » puis « s » : le bilan donne la zone morte et la pente de chaque roue.
//     Refaire 2 fois : valeurs stables à 10 % près. Les reporter dans config.h
//     (LEFT_/RIGHT_MOTOR_DEADBAND_PCT et LEFT_/RIGHT_MOTOR_PCT_PER_MM_S), puis téléverser à nouveau.
//     Sur cales, la roue n'a pas de charge : ces valeurs-là ne sont pas à reporter.
//  4. Au sol, « a » puis « e » : avec la bonne anticipation, la correction P + I est proche de 0.
//  5. Seulement si un critère échoue : changer UN seul gain dans config.h, téléverser, refaire 4.
//     - montée trop lente : vérifier d'abord l'anticipation (3), puis augmenter SPEED_KP_PCT_PER_MM_S ;
//     - dépassement ou oscillation : baisser SPEED_KI_PCT_PER_MM, puis SPEED_KP_PCT_PER_MM_S ;
//     - erreur qui reste : augmenter SPEED_KI_PCT_PER_MM.
//  6. « n » : refaire 1 en marche arrière, sur cales.
//
// CRITÈRES DE RÉUSSITE (au sol, échelon à 150 mm/s, chaque roue)
// - montée à 90 % de la consigne en 300 ms au plus ;
// - dépassement de 15 % au plus ;
// - erreur moyenne en régime établi de 5 mm/s au plus ;
// - aucune PANNE.
#include <Arduino.h>
#include <esp_system.h>

#include "config.h"
#include "encoders.h"
#include "logic/speed_controller.h"
#include "motor_driver.h"

namespace {

enum class TestKind { NONE, STEP, STAIRS, CONSTANT };

constexpr int LEFT = 0;
constexpr int RIGHT = 1;
constexpr int STAIR_COUNT =
    sizeof(TEST_BENCH_PID_STAIR_SPEEDS_MM_S) / sizeof(TEST_BENCH_PID_STAIR_SPEEDS_MM_S[0]);

const char* const WHEEL_NAMES[2] = {"gauche", "droite"};
const float DEADBAND_PCT[2] = {LEFT_MOTOR_DEADBAND_PCT, RIGHT_MOTOR_DEADBAND_PCT};
const float PCT_PER_MM_S[2] = {LEFT_MOTOR_PCT_PER_MM_S, RIGHT_MOTOR_PCT_PER_MM_S};
const char* const DEADBAND_NAMES[2] = {"LEFT_MOTOR_DEADBAND_PCT", "RIGHT_MOTOR_DEADBAND_PCT"};
const char* const SLOPE_NAMES[2] = {"LEFT_MOTOR_PCT_PER_MM_S", "RIGHT_MOTOR_PCT_PER_MM_S"};

// Les vrais régulateurs du match, avec l'anticipation propre à chaque roue (comme motion.cpp).
SpeedController controllers[2] = {
    SpeedController(LEFT_MOTOR_DEADBAND_PCT, LEFT_MOTOR_PCT_PER_MM_S),
    SpeedController(RIGHT_MOTOR_DEADBAND_PCT, RIGHT_MOTOR_PCT_PER_MM_S)};

// État d'une roue pendant un essai, et ce qu'on mesure pour le bilan.
struct WheelState {
  bool active = false;  // roue testée dans cet essai
  int32_t lastTicks = 0;
  int32_t startTicks = 0;
  float measuredMmS = 0.0f;  // vitesse brute du dernier cycle : celle que reçoit le régulateur
  float history[3] = {0.0f, 0.0f, 0.0f};  // 3 dernières mesures, pour l'affichage et le bilan
  uint8_t historyIndex = 0;
  float targetMmS = 0.0f;
  float pct = 0.0f;
  uint32_t stallStartMs = 0;  // début d'une suspicion de roue bloquée (0 = aucune)
  // Bilan de l'échelon et de la vitesse constante (vitesses comptées dans le sens commandé).
  int32_t riseTimeMs = -1;  // -1 : 90 % de la consigne jamais atteints
  float peakMmS = 0.0f;
  uint32_t steadyCount = 0;
  float steadySumMmS = 0.0f;
  float steadyMinMmS = 0.0f;
  float steadyMaxMmS = 0.0f;
  float steadyPwmSumPct = 0.0f;
  float steadyCorrectionSumPct = 0.0f;
  // Bilan de l'escalier : un total par palier.
  uint32_t stairCount[STAIR_COUNT] = {};
  float stairSpeedSumMmS[STAIR_COUNT] = {};
  float stairPwmSumPct[STAIR_COUNT] = {};
};

WheelState wheels[2];
TestKind test = TestKind::NONE;
bool selected[2] = {true, true};                  // roues à tester (a = les deux)
float speedMmS = TEST_BENCH_PID_STEP_SPEED_MM_S;  // consigne de l'échelon et de la vitesse constante
float direction = 1.0f;                           // +1 : en avant, -1 : en arrière
bool onStands = false;                            // false = AU SOL : distance limitée par essai
bool csvOutput = false;
bool faulty = false;
uint32_t testStartMs = 0;
uint32_t lastCycleMs = 0;
uint32_t lastLogMs = 0;

const char* testName(TestKind kind) {
  switch (kind) {
    case TestKind::STEP: return "echelon";
    case TestKind::STAIRS: return "escalier";
    case TestKind::CONSTANT: return "vitesse constante";
    default: return "aucun";
  }
}

const char* selectionName() {
  if (selected[LEFT] && selected[RIGHT]) {
    return "les deux roues";
  }
  return selected[LEFT] ? "roue gauche seule" : "roue droite seule";
}

const char* modeName() {
  return onStands ? "SUR CALES" : "AU SOL (800 mm max par essai)";
}

float smoothedMmS(const WheelState& w) {
  return (w.history[0] + w.history[1] + w.history[2]) / 3.0f;
}

// Part « anticipation » de la PWM, avec la même formule que SpeedController.
// Sert seulement à afficher ce que la correction P + I ajoute par-dessus.
float feedForwardPct(int i, float targetMmS) {
  if (targetMmS == 0.0f) {
    return 0.0f;
  }
  const float sign = (targetMmS > 0.0f) ? 1.0f : -1.0f;
  return sign * DEADBAND_PCT[i] + targetMmS * PCT_PER_MM_S[i];
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
  Serial.printf("[BANC] Cause du redemarrage : %s\n", reason);
}

void printSettings() {
  Serial.printf("[BANC] Regulateur : KP %.3f %%/(mm/s), KI %.3f %%/mm, integrale max %.0f %%,"
                " cycle %lu ms, PWM max %d %%\n",
                SPEED_KP_PCT_PER_MM_S, SPEED_KI_PCT_PER_MM, SPEED_INTEGRAL_MAX_PCT,
                (unsigned long)MOTION_PERIOD_MS, MOTOR_MAX_PWM_PCT);
  Serial.printf("[BANC] Anticipation : gauche %.1f %% + %.3f %%/(mm/s) | droite %.1f %% + %.3f %%/(mm/s)\n",
                DEADBAND_PCT[LEFT], PCT_PER_MM_S[LEFT], DEADBAND_PCT[RIGHT], PCT_PER_MM_S[RIGHT]);
  Serial.printf("[BANC] Prochain essai : %s, %.0f mm/s, %s, %s\n", selectionName(), speedMmS,
                direction > 0.0f ? "en avant" : "en arriere", modeName());
}

void printHelp() {
  Serial.println("[BANC] Commandes (une lettre, sans Entree) :");
  Serial.println("[BANC] REGLER (rien ne tourne) :");
  Serial.println("[BANC]   g / d / a = choisir la roue gauche seule / la roue droite seule / les deux");
  Serial.println("[BANC]   p / m (ou + / -) = V plus / moins 25 mm/s     n = sens avant / arriere");
  Serial.println("[BANC]   k = au sol / sur cales   v = courbes CSV   i = reglages   h = aide");
  Serial.println("[BANC] LANCER UN ESSAI (les roues tournent) :");
  Serial.println("[BANC]   e = echelon 0 -> V pendant 3 s (montee, depassement, erreur)");
  Serial.println("[BANC]   s = escalier 50, 100, 150, 200 mm/s (mesure l'anticipation, AU SOL)");
  Serial.println("[BANC]   c = vitesse constante V pendant 8 s (freiner la roue au doigt)");
  Serial.println("[BANC] Pendant un essai : N'IMPORTE QUELLE touche = STOP.");
}

// Consigne (sans signe) à l'instant elapsedMs de l'essai. Renvoie false quand l'essai est fini.
// stairIndex : numéro du palier de l'escalier, -1 pour les autres essais.
bool scheduledSpeed(uint32_t elapsedMs, float& speed, int& stairIndex) {
  stairIndex = -1;
  switch (test) {
    case TestKind::STEP:
      speed = speedMmS;
      return elapsedMs < TEST_BENCH_PID_STEP_DURATION_MS;
    case TestKind::CONSTANT:
      speed = speedMmS;
      return elapsedMs < TEST_BENCH_PID_CONSTANT_DURATION_MS;
    case TestKind::STAIRS: {
      const int index = (int)(elapsedMs / TEST_BENCH_PID_STAIR_DURATION_MS);
      if (index >= STAIR_COUNT) {
        return false;
      }
      stairIndex = index;
      speed = TEST_BENCH_PID_STAIR_SPEEDS_MM_S[index];
      return true;
    }
    default:
      return false;
  }
}

// Même règle que motion : beaucoup de PWM, mais la roue n'avance pas dans le bon sens, trop longtemps.
// Causes : roue bloquée, ou encodeur inversé (le régulateur pousserait alors jusqu'à 100 %).
bool isStalledTooLong(WheelState& w, uint32_t nowMs) {
  const float speedInCommandedDirection = (w.pct > 0.0f) ? w.measuredMmS : -w.measuredMmS;
  if (fabsf(w.pct) < STALL_MIN_PCT || speedInCommandedDirection >= STALL_MAX_SPEED_MM_S) {
    w.stallStartMs = 0;
    return false;
  }
  if (w.stallStartMs == 0) {
    w.stallStartMs = nowMs;
    return false;
  }
  return nowMs - w.stallStartMs >= STALL_TIME_MS;
}

// Ajoute le cycle en cours au bilan de la roue i.
void accumulate(int i, uint32_t elapsedMs, int stairIndex) {
  WheelState& w = wheels[i];
  const float targetAbsMmS = fabsf(w.targetMmS);
  const float alongMmS = direction * smoothedMmS(w);  // vitesse dans le sens commandé
  const float pctAlong = direction * w.pct;

  if (test == TestKind::STAIRS) {
    if (stairIndex < 0) {
      return;
    }
    const uint32_t inStairMs = elapsedMs - (uint32_t)stairIndex * TEST_BENCH_PID_STAIR_DURATION_MS;
    if (inStairMs >= TEST_BENCH_PID_STAIR_SETTLE_MS) {
      w.stairCount[stairIndex]++;
      w.stairSpeedSumMmS[stairIndex] += alongMmS;
      w.stairPwmSumPct[stairIndex] += pctAlong;
    }
    return;
  }

  // Échelon et vitesse constante.
  if (w.riseTimeMs < 0 && alongMmS >= TEST_BENCH_PID_RISE_FRACTION * targetAbsMmS) {
    w.riseTimeMs = (int32_t)elapsedMs;
  }
  if (alongMmS > w.peakMmS) {
    w.peakMmS = alongMmS;
  }
  if (elapsedMs < TEST_BENCH_PID_SETTLE_MS) {
    return;
  }
  if (w.steadyCount == 0) {
    w.steadyMinMmS = alongMmS;
    w.steadyMaxMmS = alongMmS;
  }
  w.steadyMinMmS = fminf(w.steadyMinMmS, alongMmS);
  w.steadyMaxMmS = fmaxf(w.steadyMaxMmS, alongMmS);
  w.steadyCount++;
  w.steadySumMmS += alongMmS;
  w.steadyPwmSumPct += pctAlong;
  w.steadyCorrectionSumPct += pctAlong - direction * feedForwardPct(i, w.targetMmS);
}

// Droite « PWM = zone morte + pente × vitesse » qui passe au mieux par les paliers mesurés
// (moindres carrés). C'est exactement la forme de l'anticipation de SpeedController.
bool fitFeedForward(const WheelState& w, float& deadbandPct, float& pctPerMmS) {
  float sumX = 0.0f;
  float sumY = 0.0f;
  float sumXX = 0.0f;
  float sumXY = 0.0f;
  int n = 0;
  for (int k = 0; k < STAIR_COUNT; k++) {
    if (w.stairCount[k] == 0) {
      continue;
    }
    const float x = w.stairSpeedSumMmS[k] / w.stairCount[k];
    const float y = w.stairPwmSumPct[k] / w.stairCount[k];
    sumX += x;
    sumY += y;
    sumXX += x * x;
    sumXY += x * y;
    n++;
  }
  const float denominator = n * sumXX - sumX * sumX;
  if (n < 2 || fabsf(denominator) < 1.0f) {
    return false;
  }
  pctPerMmS = (n * sumXY - sumX * sumY) / denominator;
  deadbandPct = (sumY - pctPerMmS * sumX) / n;
  return true;
}

void printStepReport(int i, bool withVerdict) {
  const WheelState& w = wheels[i];
  Serial.printf("[PID] Roue %s, consigne %.0f mm/s :\n", WHEEL_NAMES[i], speedMmS);
  if (w.riseTimeMs >= 0) {
    Serial.printf("[PID]   montee a 90 %% : %ld ms\n", (long)w.riseTimeMs);
  } else {
    Serial.println("[PID]   montee a 90 % : jamais atteinte");
  }
  const float overshootPct = fmaxf(0.0f, (w.peakMmS - speedMmS) / speedMmS * 100.0f);
  Serial.printf("[PID]   depassement : %.0f %% (pointe a %.0f mm/s)\n", overshootPct, w.peakMmS);

  float errorMmS = 0.0f;
  if (w.steadyCount > 0) {
    const float meanMmS = w.steadySumMmS / w.steadyCount;
    const float meanPwmPct = w.steadyPwmSumPct / w.steadyCount;
    const float meanCorrectionPct = w.steadyCorrectionSumPct / w.steadyCount;
    errorMmS = meanMmS - speedMmS;
    Serial.printf("[PID]   regime etabli : moyenne %.0f mm/s (erreur %+.1f mm/s), min %.0f, max %.0f\n",
                  meanMmS, errorMmS, w.steadyMinMmS, w.steadyMaxMmS);
    Serial.printf("[PID]   PWM moyenne %.1f %% = anticipation %.1f %% + correction P+I %+.1f %%\n",
                  meanPwmPct, meanPwmPct - meanCorrectionPct, meanCorrectionPct);
    if (fabsf(meanCorrectionPct) > TEST_BENCH_PID_CORRECTION_HINT_PCT) {
      Serial.println("[PID]   La correction fait une grosse part du travail : mesurez l'anticipation"
                     " au sol (touche s).");
    }
  } else {
    Serial.println("[PID]   regime etabli : non atteint (essai coupe avant 1,5 s)");
  }

  if (withVerdict) {
    const bool riseOk = w.riseTimeMs >= 0 && (uint32_t)w.riseTimeMs <= TEST_BENCH_PID_RISE_TIME_MAX_MS;
    const bool overshootOk = overshootPct <= TEST_BENCH_PID_OVERSHOOT_MAX_PCT;
    const bool errorOk = w.steadyCount > 0 && fabsf(errorMmS) <= TEST_BENCH_PID_ERROR_MAX_MM_S;
    Serial.printf("[PID]   -> montee %s (<= %lu ms) | depassement %s (<= %.0f %%) | erreur %s (<= %.0f mm/s)\n",
                  riseOk ? "OK" : "ECHEC", (unsigned long)TEST_BENCH_PID_RISE_TIME_MAX_MS,
                  overshootOk ? "OK" : "ECHEC", TEST_BENCH_PID_OVERSHOOT_MAX_PCT,
                  errorOk ? "OK" : "ECHEC", TEST_BENCH_PID_ERROR_MAX_MM_S);
  }
}

void printStairReport(int i) {
  const WheelState& w = wheels[i];
  Serial.printf("[PID] Roue %s :\n", WHEEL_NAMES[i]);
  for (int k = 0; k < STAIR_COUNT; k++) {
    if (w.stairCount[k] == 0) {
      Serial.printf("[PID]   palier %3.0f mm/s : non mesure (essai coupe)\n",
                    TEST_BENCH_PID_STAIR_SPEEDS_MM_S[k]);
      continue;
    }
    Serial.printf("[PID]   palier %3.0f mm/s : mesure %4.0f mm/s, PWM %5.1f %%\n",
                  TEST_BENCH_PID_STAIR_SPEEDS_MM_S[k], w.stairSpeedSumMmS[k] / w.stairCount[k],
                  w.stairPwmSumPct[k] / w.stairCount[k]);
  }
  float deadbandPct = 0.0f;
  float pctPerMmS = 0.0f;
  if (!fitFeedForward(w, deadbandPct, pctPerMmS)) {
    Serial.println("[PID]   Pas assez de paliers mesures pour calculer l'anticipation.");
    return;
  }
  Serial.printf("[PID]   Anticipation mesuree : zone morte %.1f %%, pente %.3f %%/(mm/s)"
                "  (actuel : %.1f %% et %.3f)\n",
                deadbandPct, pctPerMmS, DEADBAND_PCT[i], PCT_PER_MM_S[i]);
  if (onStands) {
    Serial.println("[PID]   SUR CALES, la roue n'a pas de charge : ne pas reporter. Refaire au sol.");
  } else if (direction < 0.0f) {
    Serial.println("[PID]   En marche arriere : a comparer avec la marche avant (config.h n'a qu'une"
                   " valeur par roue).");
  } else {
    Serial.printf("[PID]   A reporter dans config.h : %s = %.1ff; et %s = %.3ff;  // Txxx\n",
                  DEADBAND_NAMES[i], deadbandPct, SLOPE_NAMES[i], pctPerMmS);
  }
}

void printReport(TestKind kind) {
  Serial.printf("[PID] ===== BILAN : %s, %s, %s =====\n", testName(kind),
                direction > 0.0f ? "en avant" : "en arriere", onStands ? "sur cales" : "au sol");
  for (int i = 0; i < 2; i++) {
    if (!wheels[i].active) {
      continue;
    }
    if (kind == TestKind::STAIRS) {
      printStairReport(i);
    } else {
      // Le verdict n'a de sens que pour l'échelon : pendant la vitesse constante, on freine au doigt.
      printStepReport(i, kind == TestKind::STEP);
    }
  }
}

void startTest(TestKind kind, uint32_t nowMs) {
  if (faulty) {
    Serial.println("[BANC] Panne en cours : verifiez les encodeurs (roues a la main), puis redemarrez avec EN.");
    return;
  }
  int32_t ticks[2];
  encoders::read(ticks[LEFT], ticks[RIGHT]);
  for (int i = 0; i < 2; i++) {
    wheels[i] = WheelState();
    wheels[i].active = selected[i];
    wheels[i].lastTicks = ticks[i];
    wheels[i].startTicks = ticks[i];
    controllers[i].reset();
  }
  test = kind;
  testStartMs = nowMs;
  lastCycleMs = nowMs;
  lastLogMs = nowMs;
  if (kind == TestKind::STAIRS) {
    Serial.printf("[BANC] Essai : escalier, %s, %s, %s. Une touche = STOP.\n", selectionName(),
                  direction > 0.0f ? "en avant" : "en arriere", modeName());
  } else {
    Serial.printf("[BANC] Essai : %s a %.0f mm/s, %s, %s, %s. Une touche = STOP.\n", testName(kind),
                  speedMmS, selectionName(), direction > 0.0f ? "en avant" : "en arriere", modeName());
  }
  if (csvOutput) {
    Serial.println("CSV;t_ms;cibleG;mesureG;pwmG;cibleD;mesureD;pwmD");
  }
}

void stopTest(const char* reason, uint32_t nowMs) {
  const TestKind finished = test;
  test = TestKind::NONE;
  for (int i = 0; i < 2; i++) {
    wheels[i].pct = 0.0f;
  }
  motorDriver::brake();
  Serial.printf("[BANC] Fin de l'essai (%s) apres %.1f s.\n", reason, (nowMs - testStartMs) / 1000.0f);
  printReport(finished);
}

void printLiveLine(uint32_t elapsedMs) {
  Serial.printf("[PID] t = %4.1f s | G cible %4.0f mesure %4.0f PWM %4.0f %% | D cible %4.0f mesure %4.0f"
                " PWM %4.0f %%\n",
                elapsedMs / 1000.0f, wheels[LEFT].targetMmS, smoothedMmS(wheels[LEFT]),
                wheels[LEFT].pct, wheels[RIGHT].targetMmS, smoothedMmS(wheels[RIGHT]),
                wheels[RIGHT].pct);
}

// Valeurs brutes, en entiers : un tableur français les lit sans problème de virgule.
void printCsvLine(uint32_t elapsedMs) {
  Serial.printf("CSV;%lu;%ld;%ld;%ld;%ld;%ld;%ld\n", (unsigned long)elapsedMs,
                lroundf(wheels[LEFT].targetMmS), lroundf(wheels[LEFT].measuredMmS),
                lroundf(wheels[LEFT].pct), lroundf(wheels[RIGHT].targetMmS),
                lroundf(wheels[RIGHT].measuredMmS), lroundf(wheels[RIGHT].pct));
}

// Un cycle du régulateur, toutes les MOTION_PERIOD_MS, comme motion en match.
void runCycle(uint32_t nowMs) {
  const float dtS = (nowMs - lastCycleMs) / 1000.0f;
  lastCycleMs = nowMs;

  int32_t ticks[2];
  encoders::read(ticks[LEFT], ticks[RIGHT]);
  for (int i = 0; i < 2; i++) {
    WheelState& w = wheels[i];
    w.measuredMmS = (dtS > 0.0f) ? (ticks[i] - w.lastTicks) * MM_PER_TICK / dtS : 0.0f;
    w.lastTicks = ticks[i];
    w.history[w.historyIndex] = w.measuredMmS;
    w.historyIndex = (w.historyIndex + 1) % 3;
  }

  if (test == TestKind::NONE) {
    motorDriver::brake();  // hors essai : freins serrés
    return;
  }

  const uint32_t elapsedMs = nowMs - testStartMs;
  float speed = 0.0f;
  int stairIndex = -1;
  if (!scheduledSpeed(elapsedMs, speed, stairIndex)) {
    stopTest("fin normale", nowMs);
    return;
  }

  for (int i = 0; i < 2; i++) {
    WheelState& w = wheels[i];
    if (!w.active) {
      w.targetMmS = 0.0f;
      w.pct = 0.0f;  // roue non testée : freinée
      continue;
    }
    w.targetMmS = direction * speed;
    w.pct = controllers[i].update(w.targetMmS, w.measuredMmS, dtS);
    accumulate(i, elapsedMs, stairIndex);

    if (isStalledTooLong(w, nowMs)) {
      faulty = true;
      stopTest("PANNE", nowMs);
      Serial.printf("[BANC] PANNE roue %s : beaucoup de PWM, mais la roue n'avance pas dans le bon sens.\n",
                    WHEEL_NAMES[i]);
      Serial.println("[BANC] Roue bloquee ou encodeur inverse : verifiez (roues a la main), puis EN.");
      return;
    }
    const float travelledMm = fabsf((ticks[i] - w.startTicks) * MM_PER_TICK);
    if (!onStands && travelledMm >= TEST_BENCH_PID_FLOOR_MAX_DISTANCE_MM) {
      stopTest("800 mm parcourus au sol", nowMs);
      return;
    }
  }

  motorDriver::setWheelPercent(wheels[LEFT].pct, wheels[RIGHT].pct);
  if (csvOutput) {
    printCsvLine(elapsedMs);
  }
}

// Attention : dans test_moteurs, g, d et a font tourner les roues. Ici, elles ne font que
// choisir les roues : on le rappelle après chaque réglage.
void printLaunchHint() {
  Serial.println("[BANC]   -> reglage enregistre, rien ne tourne. LANCER : e (echelon), s (escalier)"
                 " ou c (vitesse constante).");
}

void changeSpeed(float deltaMmS) {
  speedMmS = constrain(speedMmS + deltaMmS, TEST_BENCH_PID_SPEED_STEP_MM_S, TEST_BENCH_PID_MAX_SPEED_MM_S);
  Serial.printf("[BANC] Vitesse de l'echelon et de la vitesse constante : %.0f mm/s\n", speedMmS);
  printLaunchHint();
}

void selectWheels(bool left, bool right) {
  selected[LEFT] = left;
  selected[RIGHT] = right;
  Serial.printf("[BANC] Roues testees : %s\n", selectionName());
  printLaunchHint();
}

void handleKey(char key, uint32_t nowMs) {
  if (key == '\r' || key == '\n') {
    return;  // touche Entrée : ignorée
  }
  // Sécurité : pendant un essai, n'importe quelle touche arrête tout.
  if (test != TestKind::NONE) {
    stopTest("touche", nowMs);
    return;
  }
  switch (tolower(key)) {
    case 'g': selectWheels(true, false); break;
    case 'd': selectWheels(false, true); break;
    case 'a': selectWheels(true, true); break;
    case 'e': startTest(TestKind::STEP, nowMs); break;
    case 's': startTest(TestKind::STAIRS, nowMs); break;
    case 'c': startTest(TestKind::CONSTANT, nowMs); break;
    case 'p':
    case '+': changeSpeed(TEST_BENCH_PID_SPEED_STEP_MM_S); break;
    case 'm':
    case '-': changeSpeed(-TEST_BENCH_PID_SPEED_STEP_MM_S); break;
    case 'n':
      direction = -direction;
      Serial.printf("[BANC] Sens : %s\n", direction > 0.0f ? "en avant" : "EN ARRIERE");
      printLaunchHint();
      break;
    case 'k':
      onStands = !onStands;
      Serial.printf("[BANC] Mode : %s\n", modeName());
      printLaunchHint();
      break;
    case 'v':
      csvOutput = !csvOutput;
      Serial.printf("[BANC] Courbes CSV (une ligne toutes les %lu ms) : %s\n",
                    (unsigned long)MOTION_PERIOD_MS, csvOutput ? "oui" : "non");
      break;
    case 'i': printSettings(); break;
    case 'h':
    case '?': printHelp(); break;
    default:
      Serial.println("[BANC] Touche inconnue (ce banc n'utilise que des lettres).");
      printHelp();
      break;
  }
}

}  // namespace

void setup() {
  // En tout premier : moteurs freinés. Rien ne bouge avant une commande.
  motorDriver::begin();

  Serial.begin(SERIAL_BAUD);
  Serial.println();
  Serial.println("[BANC] ===== BANC DE TEST DU REGULATEUR DE VITESSE (PID) =====");
  Serial.println("[BANC] PAS D'EVITEMENT : NE JAMAIS UTILISER EN MATCH.");
  Serial.println("[BANC] Apres le test : pio run -t upload (remet le firmware).");
  printResetReason();

  encoders::begin();
  encoders::read(wheels[LEFT].lastTicks, wheels[RIGHT].lastTicks);
  lastCycleMs = millis();

  printSettings();
  printHelp();
  Serial.println("[BANC] Moteurs arretes. Premier essai : PAMI sur cales, k puis e.");
}

void loop() {
  const uint32_t nowMs = millis();

  while (Serial.available() > 0) {
    handleKey((char)Serial.read(), nowMs);
  }

  if (nowMs - lastCycleMs >= MOTION_PERIOD_MS) {
    runCycle(nowMs);
  }

  if (test != TestKind::NONE && !csvOutput && nowMs - lastLogMs >= TEST_BENCH_PID_LOG_PERIOD_MS) {
    lastLogMs = nowMs;
    printLiveLine(nowMs - testStartMs);
  }
}

// Banc de test de l'ultrason HC-SR04 : chaque mesure, brute et filtrée.
//
// !!! PROGRAMME DE TEST : AUCUN MOUVEMENT, AUCUN ÉVITEMENT. NE JAMAIS L'UTILISER EN MATCH. !!!
// Après le test, remettre le firmware : pio run -t upload
//
// Téléverser : pio run -e test_ultrason -t upload -t monitor (maintenir BOOT pendant « Connecting »)
//
// Ce banc utilise le VRAI code du firmware (sensors.cpp) : ce qu'on mesure ici, c'est ce que voit
// l'évitement en match. Les moteurs restent freinés en permanence.
//
// MATÉRIEL ET MONTAGE
// - ESP32 en USB. HC-SR04 : VCC sur 5V, GND commun, TRIG sur GPIO 18,
//   ECHO sur GPIO 19 à travers le pont 1 kΩ (côté ECHO) / 2 kΩ (vers GND).
// - Capteur monté sur le PAMI, à sa hauteur réelle (moins de 70 mm du sol), bien horizontal.
// - Alimentation des moteurs : pas nécessaire.
//
// CE QUE LE BANC AFFICHE
// - Une ligne par mesure (environ 16 par seconde) :
//     [US] #123 | echo   583 us | brute  100 mm | filtree  101 mm
//     [US] #124 | PAS D'ECHO (38012 us) | brute 2000 mm | filtree  101 mm
//   « brute » = cette mesure seule ; « filtree » = médiane des 3 dernières, c'est ce qu'utilise safety.
// - Une ligne de bilan par seconde :
//     [BILAN] 16 mesures (17 envois) | 2 sans echo | brute min 98 max 104 moy 101 mm | filtree 101 mm
//   « envois » plus grand que « mesures » : des impulsions sont restées sans écho complet.
// - Touche « v » : afficher ou masquer les lignes de mesure (le bilan reste). « h » : aide.
// - Touche « r » : MODE DE RÉFÉRENCE. Mesure avec la méthode classique des tutoriels Arduino
//   (déclenchement puis pulseIn()), sans aucun code du firmware. Lignes préfixées [REF].
//   Sert à savoir si un problème vient du capteur ou de notre code :
//   - la référence rate aussi les échos  -> problème matériel (capteur, alimentation, montage) ;
//   - la référence voit bien, pas sensors -> problème dans notre code.
//   pulseIn() bloque le programme jusqu'à 70 ms : interdit en match, sans risque ici.
//
// PROTOCOLE (objet immobile au moins 3 s à chaque distance, face au capteur, à sa hauteur)
//  1. Boîte en carton (face plate) à 5, 10, 15, 20, 30, 50 et 100 cm (distance au mètre ruban).
//  2. Même chose avec l'avant du PAMI 2 (éteint), bien de face.
//  3. Boîte et PAMI 2 tournés de 30° puis de 45°, à 15 et 30 cm.
//  4. Objet rond de la taille d'un PAMI (bouteille, boîte de conserve, pot) à 10, 15, 20, 30 cm.
//  5. La main, à 10, 15 et 20 cm.
//  6. Zone aveugle : approcher la boîte lentement de 10 cm à 0 ; noter la distance où les mesures
//     deviennent fausses ou « PAS D'ECHO ».
//  Pour chaque cas, noter : distance réelle, filtree affichée, min/max du bilan, nombre « sans echo ».
//
// CRITÈRES DE RÉUSSITE
// - Boîte de face, de 10 à 100 cm : filtree à ± 10 mm de la distance réelle, 0 « sans echo ».
// - PAMI 2 de face et objet rond : détectés de façon stable (écart min/max < 20 mm) jusqu'à 30 cm.
// - Zone aveugle mesurée (attendue : 2 à 5 cm).
#include <Arduino.h>

#include "config.h"
#include "motor_driver.h"
#include "sensors.h"

namespace {

bool showEachMeasure = true;
bool referenceMode = false;  // touche r : méthode classique pulseIn(), sans le code du firmware
uint32_t lastReferenceMs = 0;
uint32_t referenceCount = 0;
uint32_t lastPrintedMeasure = 0;

// Bilan de la seconde en cours.
uint32_t summaryStartMs = 0;
uint32_t summaryStartTriggers = 0;
uint32_t summaryStartMeasures = 0;
uint32_t summaryNoEcho = 0;
uint32_t summaryValid = 0;
uint32_t summarySumMm = 0;
uint16_t summaryMinMm = 0;
uint16_t summaryMaxMm = 0;
bool faultReported = false;

void printHelp() {
  Serial.println("[BANC] Commandes (une touche, sans Entree) :");
  Serial.println("[BANC]   v = afficher / masquer chaque mesure (le bilan par seconde reste)");
  Serial.println("[BANC]   r = mode de reference (methode classique pulseIn, sans notre code)");
  Serial.println("[BANC]   h = cette aide");
}

void resetSummary(uint32_t nowMs) {
  const sensors::UltrasoundRaw raw = sensors::lastRawMeasure();
  summaryStartMs = nowMs;
  summaryStartTriggers = raw.triggers;
  summaryStartMeasures = raw.measures;
  summaryNoEcho = 0;
  summaryValid = 0;
  summarySumMm = 0;
  summaryMinMm = 0;
  summaryMaxMm = 0;
}

// Une nouvelle mesure est arrivée : on l'affiche et on l'ajoute au bilan.
void handleNewMeasure(const sensors::UltrasoundRaw& raw) {
  if (raw.noEcho) {
    summaryNoEcho++;
  } else {
    if (summaryValid == 0 || raw.rawMm < summaryMinMm) {
      summaryMinMm = raw.rawMm;
    }
    if (summaryValid == 0 || raw.rawMm > summaryMaxMm) {
      summaryMaxMm = raw.rawMm;
    }
    summaryValid++;
    summarySumMm += raw.rawMm;
  }

  if (!showEachMeasure) {
    return;
  }
  if (raw.noEcho) {
    Serial.printf("[US] #%lu | PAS D'ECHO (%lu us) | brute %4u mm | filtree %4u mm\n",
                  (unsigned long)raw.measures, (unsigned long)raw.echoUs, raw.rawMm,
                  sensors::frontDistanceMm());
  } else {
    Serial.printf("[US] #%lu | echo %5lu us | brute %4u mm | filtree %4u mm\n",
                  (unsigned long)raw.measures, (unsigned long)raw.echoUs, raw.rawMm,
                  sensors::frontDistanceMm());
  }
}

void printSummary(uint32_t nowMs) {
  const sensors::UltrasoundRaw raw = sensors::lastRawMeasure();
  const uint32_t measures = raw.measures - summaryStartMeasures;
  const uint32_t triggers = raw.triggers - summaryStartTriggers;
  if (summaryValid > 0) {
    Serial.printf("[BILAN] %lu mesures (%lu envois) | %lu sans echo | brute min %u max %u moy %lu mm"
                  " | filtree %u mm\n",
                  (unsigned long)measures, (unsigned long)triggers, (unsigned long)summaryNoEcho,
                  summaryMinMm, summaryMaxMm, (unsigned long)(summarySumMm / summaryValid),
                  sensors::frontDistanceMm());
  } else {
    Serial.printf("[BILAN] %lu mesures (%lu envois) | %lu sans echo | aucune distance valide"
                  " | filtree %u mm\n",
                  (unsigned long)measures, (unsigned long)triggers, (unsigned long)summaryNoEcho,
                  sensors::frontDistanceMm());
  }
  resetSummary(nowMs);
}

// Mode de référence : la méthode « classique » des tutoriels, indépendante de sensors.cpp.
// Déclenchement de 10 µs, puis pulseIn() attend l'impulsion ECHO et mesure sa durée.
void referenceMeasure(uint32_t nowMs) {
  if (nowMs - lastReferenceMs < US_PERIOD_MS) {
    return;
  }
  lastReferenceMs = nowMs;
  digitalWrite(PIN_US_TRIG, LOW);
  delayMicroseconds(2);
  digitalWrite(PIN_US_TRIG, HIGH);
  delayMicroseconds(US_TRIGGER_PULSE_US);
  digitalWrite(PIN_US_TRIG, LOW);
  // 0 = aucune impulsion vue en 70 ms (ou impulsion plus longue que 70 ms).
  const unsigned long durationUs = pulseIn(PIN_US_ECHO, HIGH, 70000UL);
  referenceCount++;
  if (durationUs == 0) {
    Serial.printf("[REF] #%lu | aucune impulsion ECHO en 70 ms\n", (unsigned long)referenceCount);
  } else if (durationUs > US_MAX_ECHO_US) {
    Serial.printf("[REF] #%lu | PAS D'ECHO (%lu us)\n", (unsigned long)referenceCount, durationUs);
  } else {
    Serial.printf("[REF] #%lu | echo %5lu us | %4u mm\n", (unsigned long)referenceCount, durationUs,
                  (unsigned)(durationUs * US_MM_PER_ECHO_US));
  }
}

}  // namespace

void setup() {
  // En tout premier : moteurs freinés. Ce banc ne les fait jamais tourner.
  motorDriver::begin();

  Serial.begin(SERIAL_BAUD);
  Serial.println();
  Serial.println("[BANC] ===== BANC DE TEST ULTRASON =====");
  Serial.println("[BANC] Aucun mouvement. Apres le test : pio run -t upload (remet le firmware).");
  Serial.printf("[BANC] Mesure toutes les %lu ms. Pas d'echo apres %lu us = %u mm affiches.\n",
                (unsigned long)US_PERIOD_MS, (unsigned long)US_MAX_ECHO_US, US_NO_OBSTACLE_MM);
  printHelp();

  sensors::begin();
  resetSummary(millis());
}

void loop() {
  const uint32_t nowMs = millis();

  while (Serial.available() > 0) {
    const char key = (char)Serial.read();
    if (key == 'v' || key == 'V') {
      showEachMeasure = !showEachMeasure;
      Serial.printf("[BANC] Affichage de chaque mesure : %s\n", showEachMeasure ? "oui" : "non");
    } else if (key == 'r' || key == 'R') {
      referenceMode = !referenceMode;
      Serial.printf("[BANC] Mode de reference (pulseIn, sans notre code) : %s\n",
                    referenceMode ? "OUI" : "non, retour au code du firmware");
    } else if (key == 'h' || key == 'H' || key == '?') {
      printHelp();
    }
  }

  if (referenceMode) {
    referenceMeasure(nowMs);
    return;
  }

  sensors::update(nowMs);

  const sensors::UltrasoundRaw raw = sensors::lastRawMeasure();
  if (raw.measures != lastPrintedMeasure) {
    lastPrintedMeasure = raw.measures;
    handleNewMeasure(raw);
  }

  if (sensors::isFaulty() && !faultReported) {
    faultReported = true;
    Serial.println("[BANC] Aucune mesure pendant 1 s : verifier VCC 5 V, GND commun, TRIG, ECHO et le pont.");
    Serial.println("[BANC] Le banc continue de mesurer : les lignes reprennent des que l'echo revient.");
  }

  if (nowMs - summaryStartMs >= TEST_BENCH_US_SUMMARY_PERIOD_MS) {
    printSummary(nowMs);
  }
}

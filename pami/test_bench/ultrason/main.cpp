// Banc de test des deux ultrasons HC-SR04 (gauche et droit, D012) : chaque mesure, brute et filtrée.
//
// !!! PROGRAMME DE TEST : AUCUN MOUVEMENT, AUCUN ÉVITEMENT. NE JAMAIS L'UTILISER EN MATCH. !!!
// Après le test, remettre le firmware : pio run -t upload (sans fil : pio run -e esp32dev_wifi -t upload)
//
// Téléverser : pio run -e test_ultrason -t upload -t monitor (maintenir BOOT pendant « Connecting »)
// Sans fil (D010) : pio run -e test_ultrason_wifi -t upload, puis pio device monitor -e test_ultrason_wifi.
//
// Ce banc utilise le VRAI code du firmware (sensors.cpp) : ce qu'on mesure ici, c'est ce que voit
// l'évitement en match. Les moteurs restent freinés en permanence.
//
// MATÉRIEL ET MONTAGE
// - Brochage du 25/11/2026 (D018, T016) : gauche TRIG 25 / ECHO 35, droit TRIG 5 / ECHO 4 (config.h).
//   Chacun a son TRIG, mais le firmware (sensors.cpp) les fait toujours émettre ensemble.
// - Pont diviseur 1 kΩ / 2 kΩ sur chaque ECHO (5 V -> 3,3 V) : l'ESP32 supporte 3,3 V au plus.
// - Capteurs montés sur le PAMI, à leur hauteur réelle (moins de 70 mm du sol), bien horizontaux.
// - Alimentation des moteurs : pas nécessaire.
//
// CE QUE LE BANC AFFICHE
// - Une ligne par mesure (environ 16 par seconde et par capteur, les deux émettent ensemble) :
//     [US G] #123 | echo   583 us | brute  100 mm | filtree  101 mm
//     [US D] #124 | PAS D'ECHO (38012 us) | brute 2000 mm | filtree 2000 mm
//   « brute » = cette mesure seule ; « filtree » = médiane des 3 dernières de CE capteur, puis
//   confirmation des distances qui s'allongent d'un coup (logic/rise_filter.h). C'est ce qu'utilise
//   safety : il freine si G OU D voit un obstacle plus proche que OBSTACLE_STOP_DISTANCE_MM.
// - Une ligne de bilan par seconde et par capteur, puis la décision de safety :
//     [BILAN G] 16 mesures (16 envois) | 0 sans echo | brute min 98 max 104 moy 101 mm | filtree 101 mm
//     [BILAN] le plus proche : 101 mm -> safety FREINERAIT (G)
//   « envois » plus grand que « mesures » : des impulsions sont restées sans écho complet.
// - Touche « v » : afficher ou masquer les lignes de mesure (le bilan reste). « h » : aide.
// - Touche « r » : MODE DE RÉFÉRENCE. Mesure avec la méthode classique des tutoriels Arduino
//   (déclenchement puis pulseIn()), sans aucun code du firmware. Un seul capteur émet à la fois
//   (son propre TRIG), puis on lit son ECHO : le gauche, puis le droit, l'un après l'autre.
//   Lignes préfixées [REF G] et [REF D]. Sert à savoir si un problème vient du capteur ou de notre code :
//   - la référence rate aussi les échos  -> problème matériel (capteur, alimentation, connecteur) ;
//   - la référence voit bien, pas sensors -> problème dans notre code.
//   pulseIn() bloque le programme jusqu'à 70 ms : interdit en match, sans risque ici.
//   Un seul ultrason muet dans les deux modes : son connecteur, son TRIG ou son ECHO (TRIG et ECHO
//   inversés ?), son pont diviseur, ou le capteur. Les deux muets : l'alimentation des ultrasons.
//   Muet avec le firmware mais pas en référence : un capteur entend peut-être l'autre (protocole 6).
//
// PROTOCOLE (objet immobile au moins 3 s à chaque distance, à hauteur des capteurs)
//  1. Boîte en carton (face plate) bien en face de CHAQUE capteur, à 5, 10, 15, 20, 30, 50 et 100 cm.
//  2. Boîte devant le capteur GAUCHE seulement, à 15 cm : G doit la voir. Noter ce que voit D.
//     Même chose devant le capteur DROIT seulement.
//  3. Avant du PAMI 2 (éteint), au centre, à 15 et 30 cm : au moins un des deux le voit.
//  4. Objet rond de la taille d'un PAMI (bouteille, boîte de conserve) au centre, à 10, 15, 20, 30 cm.
//  5. Zone aveugle : approcher la boîte lentement de 10 cm à 0 devant chaque capteur ; noter la
//     distance où les mesures deviennent fausses ou « PAS D'ECHO ».
//  6. Rien devant (2 m dégagés) pendant 10 s : aucune distance sous 300 mm. Important : le firmware
//     fait émettre les deux capteurs ensemble (D015, D018), et l'un ne doit pas entendre directement
//     le son de l'autre. Sinon : les écarter, les tourner un peu vers l'extérieur, ou ajouter une
//     cloison entre eux.
//  Pour chaque cas, noter : distance réelle, filtree de G et de D, min/max des bilans, « sans echo ».
//
// CRITÈRES DE RÉUSSITE
// - Boîte en face d'un capteur, de 10 à 100 cm : sa filtree à ± 10 mm de la distance réelle, 0 « sans echo ».
// - Boîte devant un seul capteur à 15 cm : la ligne [BILAN] annonce « FREINERAIT ».
// - PAMI 2 et objet rond au centre : vus de façon stable (écart min/max < 20 mm) jusqu'à 30 cm.
// - Rien devant : aucune mesure parasite sous 300 mm.
#include <Arduino.h>

#include "config.h"
#include "console.h"
#include "logic/safety_filter.h"
#include "motor_driver.h"
#include "sensors.h"
#include "wireless.h"

using sensors::UltrasoundId;

namespace {

constexpr UltrasoundId ALL_SENSORS[sensors::ULTRASOUND_COUNT] = {UltrasoundId::LEFT,
                                                                 UltrasoundId::RIGHT};

bool showEachMeasure = true;
bool referenceMode = false;  // touche r : méthode classique pulseIn(), sans le code du firmware
uint32_t lastReferenceMs = 0;
uint8_t nextReferenceSensor = 0;
uint32_t referenceCount[sensors::ULTRASOUND_COUNT] = {0, 0};

// Bilan de la seconde en cours, pour un capteur.
struct Summary {
  uint32_t lastPrintedMeasure = 0;
  uint32_t startTriggers = 0;
  uint32_t startMeasures = 0;
  uint32_t noEcho = 0;
  uint32_t valid = 0;
  uint32_t sumMm = 0;
  uint16_t minMm = 0;
  uint16_t maxMm = 0;
  bool faultReported = false;
};
Summary summaries[sensors::ULTRASOUND_COUNT];
uint32_t summaryStartMs = 0;

uint8_t indexOf(UltrasoundId id) {
  return static_cast<uint8_t>(id);
}

void printHelp() {
  Console.println("[BANC] Commandes (une touche, sans Entree) :");
  Console.println("[BANC]   v = afficher / masquer chaque mesure (le bilan par seconde reste)");
  Console.println("[BANC]   r = mode de reference (methode classique pulseIn, sans notre code)");
  Console.println("[BANC]   h = cette aide");
}

void resetSummary(UltrasoundId id) {
  const sensors::UltrasoundRaw raw = sensors::lastRawMeasure(id);
  Summary& summary = summaries[indexOf(id)];
  summary.startTriggers = raw.triggers;
  summary.startMeasures = raw.measures;
  summary.noEcho = 0;
  summary.valid = 0;
  summary.sumMm = 0;
  summary.minMm = 0;
  summary.maxMm = 0;
}

// Une nouvelle mesure est arrivée : on l'affiche et on l'ajoute au bilan de ce capteur.
void handleNewMeasure(UltrasoundId id, const sensors::UltrasoundRaw& raw) {
  Summary& summary = summaries[indexOf(id)];
  if (raw.noEcho) {
    summary.noEcho++;
  } else {
    if (summary.valid == 0 || raw.rawMm < summary.minMm) {
      summary.minMm = raw.rawMm;
    }
    if (summary.valid == 0 || raw.rawMm > summary.maxMm) {
      summary.maxMm = raw.rawMm;
    }
    summary.valid++;
    summary.sumMm += raw.rawMm;
  }

  if (!showEachMeasure) {
    return;
  }
  if (raw.noEcho) {
    Console.printf("[US %s] #%lu | PAS D'ECHO (%lu us) | brute %4u mm | filtree %4u mm\n",
                   sensors::shortName(id), (unsigned long)raw.measures, (unsigned long)raw.echoUs,
                   raw.rawMm, sensors::distanceMm(id));
  } else {
    Console.printf("[US %s] #%lu | echo %5lu us | brute %4u mm | filtree %4u mm\n",
                   sensors::shortName(id), (unsigned long)raw.measures, (unsigned long)raw.echoUs,
                   raw.rawMm, sensors::distanceMm(id));
  }
}

void printSummary(UltrasoundId id) {
  const sensors::UltrasoundRaw raw = sensors::lastRawMeasure(id);
  const Summary& summary = summaries[indexOf(id)];
  const uint32_t measures = raw.measures - summary.startMeasures;
  const uint32_t triggers = raw.triggers - summary.startTriggers;
  if (summary.valid > 0) {
    Console.printf("[BILAN %s] %lu mesures (%lu envois) | %lu sans echo | brute min %u max %u moy %lu mm"
                   " | filtree %u mm\n",
                   sensors::shortName(id), (unsigned long)measures, (unsigned long)triggers,
                   (unsigned long)summary.noEcho, summary.minMm, summary.maxMm,
                   (unsigned long)(summary.sumMm / summary.valid), sensors::distanceMm(id));
  } else {
    Console.printf("[BILAN %s] %lu mesures (%lu envois) | %lu sans echo | aucune distance valide"
                   " | filtree %u mm\n",
                   sensors::shortName(id), (unsigned long)measures, (unsigned long)triggers,
                   (unsigned long)summary.noEcho, sensors::distanceMm(id));
  }
  resetSummary(id);
}

// Ce que déciderait safety : obstacle si G OU D est sous le seuil (sans la marge de libération).
void printSafetyVerdict() {
  const bool left = isObstacleAhead(false, sensors::distanceMm(UltrasoundId::LEFT));
  const bool right = isObstacleAhead(false, sensors::distanceMm(UltrasoundId::RIGHT));
  if (left || right) {
    Console.printf("[BILAN] le plus proche : %u mm -> safety FREINERAIT (%s%s%s)\n",
                   sensors::frontDistanceMm(), left ? "G" : "", (left && right) ? " et " : "",
                   right ? "D" : "");
  } else {
    Console.printf("[BILAN] le plus proche : %u mm -> voie libre\n", sensors::frontDistanceMm());
  }
}

// Mode de référence : la méthode « classique » des tutoriels, indépendante de sensors.cpp.
// Déclenchement de 10 µs sur le TRIG d'un seul capteur, puis pulseIn() attend son impulsion ECHO :
// le gauche, puis le droit au tour suivant. L'autre capteur se tait : pas de son parasite.
void referenceMeasure(uint32_t nowMs) {
  if (nowMs - lastReferenceMs < US_PERIOD_MS) {
    return;
  }
  lastReferenceMs = nowMs;
  const uint8_t index = nextReferenceSensor;
  nextReferenceSensor = (nextReferenceSensor + 1) % sensors::ULTRASOUND_COUNT;
  const uint8_t trigPin = (index == 0) ? PIN_US_LEFT_TRIG : PIN_US_RIGHT_TRIG;
  const uint8_t echoPin = (index == 0) ? PIN_US_LEFT_ECHO : PIN_US_RIGHT_ECHO;
  const char* name = sensors::shortName(ALL_SENSORS[index]);

  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(US_TRIGGER_PULSE_US);
  digitalWrite(trigPin, LOW);
  // 0 = aucune impulsion vue en 70 ms (ou impulsion plus longue que 70 ms).
  const unsigned long durationUs = pulseIn(echoPin, HIGH, 70000UL);
  referenceCount[index]++;
  const unsigned long count = referenceCount[index];
  if (durationUs == 0) {
    Console.printf("[REF %s] #%lu | aucune impulsion ECHO en 70 ms\n", name, count);
  } else if (durationUs > US_MAX_ECHO_US) {
    Console.printf("[REF %s] #%lu | PAS D'ECHO (%lu us)\n", name, count, durationUs);
  } else {
    Console.printf("[REF %s] #%lu | echo %5lu us | %4u mm\n", name, count, durationUs,
                   (unsigned)(durationUs * US_MM_PER_ECHO_US));
  }
}

}  // namespace

void setup() {
  // En tout premier : moteurs freinés. Ce banc ne les fait jamais tourner.
  motorDriver::begin();

  Serial.begin(SERIAL_BAUD);
  Console.println();
  Console.println("[BANC] ===== BANC DE TEST DES 2 ULTRASONS =====");
  Console.println("[BANC] Aucun mouvement. Apres le test : remettre le firmware (pio run -t upload, ou -e esp32dev_wifi).");
  Console.printf("[BANC] Gauche : TRIG %u / ECHO %u. Droit : TRIG %u / ECHO %u.\n", PIN_US_LEFT_TRIG,
                 PIN_US_LEFT_ECHO, PIN_US_RIGHT_TRIG, PIN_US_RIGHT_ECHO);
  Console.printf("[BANC] Les 2 capteurs emettent ensemble toutes les %lu ms. Pas d'echo apres %lu us = %u mm.\n",
                 (unsigned long)US_PERIOD_MS, (unsigned long)US_MAX_ECHO_US, US_NO_OBSTACLE_MM);
  Console.printf("[BANC] safety freine si G OU D voit un obstacle a moins de %u mm.\n",
                 OBSTACLE_STOP_DISTANCE_MM);
  printHelp();

  sensors::begin();
  for (UltrasoundId id : ALL_SENSORS) {
    resetSummary(id);
  }
  summaryStartMs = millis();
  wireless::begin();
}

void loop() {
  const uint32_t nowMs = millis();

  while (Console.available() > 0) {
    const char key = (char)Console.read();
    if (key == 'v' || key == 'V') {
      showEachMeasure = !showEachMeasure;
      Console.printf("[BANC] Affichage de chaque mesure : %s\n", showEachMeasure ? "oui" : "non");
    } else if (key == 'r' || key == 'R') {
      referenceMode = !referenceMode;
      Console.printf("[BANC] Mode de reference (pulseIn, sans notre code) : %s\n",
                     referenceMode ? "OUI" : "non, retour au code du firmware");
    } else if (key == 'h' || key == 'H' || key == '?') {
      printHelp();
    }
  }

  // Avant le « return » du mode de référence : le Wi-Fi est servi dans les deux modes.
  wireless::update(nowMs, true);  // aucun moteur ne tourne : téléversement toujours accepté

  if (referenceMode) {
    referenceMeasure(nowMs);
    return;
  }

  sensors::update(nowMs);

  for (UltrasoundId id : ALL_SENSORS) {
    const sensors::UltrasoundRaw raw = sensors::lastRawMeasure(id);
    Summary& summary = summaries[indexOf(id)];
    if (raw.measures != summary.lastPrintedMeasure) {
      summary.lastPrintedMeasure = raw.measures;
      handleNewMeasure(id, raw);
    }
    if (sensors::isFaulty(id) && !summary.faultReported) {
      summary.faultReported = true;
      Console.printf("[BANC] Ultrason %s : aucune mesure pendant 1 s. Verifier le connecteur, VCC 5 V,"
                     " GND, TRIG et ECHO (inverses ?).\n",
                     sensors::shortName(id));
      Console.println("[BANC] Le banc continue de mesurer : les lignes reprennent des que l'echo revient.");
    }
  }

  if (nowMs - summaryStartMs >= TEST_BENCH_US_SUMMARY_PERIOD_MS) {
    summaryStartMs = nowMs;
    for (UltrasoundId id : ALL_SENSORS) {
      printSummary(id);
    }
    printSafetyVerdict();
  }
}

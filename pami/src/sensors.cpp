#include "sensors.h"

#include <Arduino.h>

#include "config.h"
#include "logic/median3.h"

namespace sensors {

namespace {

// --- Mesure de l'écho, remplie par l'interruption ---
volatile uint32_t echoRiseUs = 0;      // instant du front montant d'ECHO
volatile bool echoStarted = false;     // front montant vu : la descente suivante est un écho complet
volatile uint32_t echoDurationUs = 0;  // durée du dernier écho complet
volatile bool echoReady = false;       // un nouvel écho complet attend d'être lu
portMUX_TYPE echoMux = portMUX_INITIALIZER_UNLOCKED;

// ECHO monte quand le son part, et redescend quand l'écho revient (ou après environ 38 ms sans écho).
void IRAM_ATTR onEchoChange() {
  const uint32_t nowUs = micros();
  if (digitalRead(PIN_US_ECHO) == HIGH) {
    echoRiseUs = nowUs;
    echoStarted = true;
  } else if (echoStarted) {
    // Sans front montant vu, la durée serait fausse : on ignore cette descente.
    echoStarted = false;
    portENTER_CRITICAL_ISR(&echoMux);
    echoDurationUs = nowUs - echoRiseUs;
    echoReady = true;
    portEXIT_CRITICAL_ISR(&echoMux);
  }
}

// --- État du filtre ---
uint16_t lastMeasuresMm[3] = {0, 0, 0};  // 0 au départ : « obstacle » tant qu'on n'a rien mesuré
uint8_t nextMeasureIndex = 0;
uint16_t filteredMm = 0;
uint32_t lastTriggerMs = 0;
uint32_t lastValidMeasureMs = 0;
bool faulty = false;

// --- Capteurs de sol : 3 dernières lectures de chaque côté (true = surface claire) ---
bool floorLeftReads[3] = {false, false, false};
bool floorRightReads[3] = {false, false, false};
uint8_t nextFloorIndex = 0;
uint32_t lastFloorReadMs = 0;

bool majority(const bool reads[3]) {
  return (reads[0] + reads[1] + reads[2]) >= 2;
}

void trigger() {
  digitalWrite(PIN_US_TRIG, HIGH);
  // 10 µs d'attente active : c'est la seule attente du programme, 1000 fois plus courte qu'un tour
  // de boucle qui compte. Elle ne gêne pas l'évitement.
  delayMicroseconds(US_TRIGGER_PULSE_US);
  digitalWrite(PIN_US_TRIG, LOW);
}

uint16_t echoToMm(uint32_t durationUs) {
  if (durationUs > US_MAX_ECHO_US) {
    return US_NO_OBSTACLE_MM;  // rien dans la portée : c'est normal
  }
  return (uint16_t)(durationUs * US_MM_PER_ECHO_US);
}

void addMeasure(uint16_t distanceMm, uint32_t nowMs) {
  lastMeasuresMm[nextMeasureIndex] = distanceMm;
  nextMeasureIndex = (nextMeasureIndex + 1) % 3;
  filteredMm = median3(lastMeasuresMm[0], lastMeasuresMm[1], lastMeasuresMm[2]);
  lastValidMeasureMs = nowMs;
}

void updateUltrasound(uint32_t nowMs) {
  // 1. Un écho complet est arrivé : on le convertit en distance.
  bool ready = false;
  uint32_t durationUs = 0;
  portENTER_CRITICAL(&echoMux);
  if (echoReady) {
    ready = true;
    durationUs = echoDurationUs;
    echoReady = false;
  }
  portEXIT_CRITICAL(&echoMux);
  if (ready) {
    addMeasure(echoToMm(durationUs), nowMs);
  }

  // 2. Nouvelle mesure toutes les US_PERIOD_MS.
  if (nowMs - lastTriggerMs >= US_PERIOD_MS) {
    lastTriggerMs = nowMs;
    // ECHO encore haut : la mesure précédente n'est pas finie (ou ECHO est bloqué). On n'insiste pas.
    if (digitalRead(PIN_US_ECHO) == LOW) {
      trigger();
    }
  }

  // 3. Aucune mesure valide depuis 1 s : panne (fil débranché, ECHO bloqué, capteur mort).
  if (!faulty && nowMs - lastValidMeasureMs >= SENSOR_FAULT_TIMEOUT_MS) {
    faulty = true;
    if (DEBUG_LOGS) {
      Serial.println("[SENS] PANNE ultrason : aucune mesure depuis 1 s.");
    }
  }
}

// Sortie du module à l'état bas = la lumière IR revient = surface claire sous le capteur.
void readFloor(uint8_t index) {
  floorLeftReads[index] = (digitalRead(PIN_FLOOR_LEFT) == LOW);
  floorRightReads[index] = (digitalRead(PIN_FLOOR_RIGHT) == LOW);
}

void updateFloor(uint32_t nowMs) {
  if (nowMs - lastFloorReadMs < FLOOR_SENSOR_PERIOD_MS) {
    return;
  }
  lastFloorReadMs = nowMs;
  readFloor(nextFloorIndex);
  nextFloorIndex = (nextFloorIndex + 1) % 3;
}

}  // namespace

void begin() {
  pinMode(PIN_US_TRIG, OUTPUT);
  digitalWrite(PIN_US_TRIG, LOW);
  pinMode(PIN_US_ECHO, INPUT);  // ECHO passe par un pont diviseur 5 V -> 3,3 V
  attachInterrupt(digitalPinToInterrupt(PIN_US_ECHO), onEchoChange, CHANGE);
  lastValidMeasureMs = millis();

  // GPIO 34 et 35 : entrées seules, sans pull-up interne. Avec la résistance de 100 kΩ vers la
  // masse, un fil débranché lit toujours la même chose : le bord de la zone n'est jamais « vu »,
  // et l'approche s'arrête au bout de ZONE_SEARCH_MAX_MM.
  pinMode(PIN_FLOOR_LEFT, INPUT);
  pinMode(PIN_FLOOR_RIGHT, INPUT);
  // Les 3 lectures partent toutes de l'état réel du sol au démarrage.
  for (uint8_t i = 0; i < 3; i++) {
    readFloor(i);
  }
}

void update(uint32_t nowMs) {
  updateUltrasound(nowMs);
  updateFloor(nowMs);
}

uint16_t frontDistanceMm() {
  return filteredMm;
}

bool floorLeftReflects() {
  return majority(floorLeftReads);
}

bool floorRightReflects() {
  return majority(floorRightReads);
}

bool isFaulty() {
  return faulty;
}

}  // namespace sensors

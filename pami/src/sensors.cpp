#include "sensors.h"

#include <Arduino.h>

#include "config.h"
#include "console.h"
#include "logic/median3.h"
#include "logic/rise_filter.h"

namespace sensors {

namespace {

// Tout ce qu'on sait d'un ultrason : sa broche ECHO, sa mesure en cours et son filtre.
// Chaque capteur a son TRIG (D018), mais on les déclenche toujours ensemble (voir trigger()).
struct Ultrasound {
  Ultrasound(uint8_t echo, const char* displayName, const char* displayShortName)
      : echoPin(echo), name(displayName), shortName(displayShortName) {}

  const uint8_t echoPin;
  const char* name;       // "gauche" ou "droit", pour les messages
  const char* shortName;  // "G" ou "D"

  // --- Mesure de l'écho, remplie par l'interruption ---
  volatile uint32_t echoRiseUs = 0;      // instant du front montant d'ECHO
  volatile bool echoStarted = false;     // front montant vu : la descente suivante est un écho complet
  volatile uint32_t echoDurationUs = 0;  // durée du dernier écho complet
  volatile bool echoReady = false;       // un nouvel écho complet attend d'être lu

  // --- État du filtre ---
  uint16_t lastMeasuresMm[3] = {0, 0, 0};  // 0 au départ : « obstacle » tant qu'on n'a rien mesuré
  uint8_t nextMeasureIndex = 0;
  uint16_t filteredMm = 0;
  RiseFilter riseFilter;  // une distance qui s'allonge d'un coup doit être confirmée
  uint32_t lastValidMeasureMs = 0;
  bool faulty = false;
  UltrasoundRaw raw = {0, 0, 0, 0, false};  // détail pour le banc test_ultrason
};

// Dans l'ordre de UltrasoundId : d'abord le gauche, puis le droit.
Ultrasound ultrasounds[ULTRASOUND_COUNT] = {
    {PIN_US_LEFT_ECHO, "gauche", "G"},
    {PIN_US_RIGHT_ECHO, "droit", "D"},
};

// Verrou partagé entre les interruptions et update().
portMUX_TYPE echoMux = portMUX_INITIALIZER_UNLOCKED;

uint32_t lastTriggerMs = 0;

Ultrasound& sensorFor(UltrasoundId id) {
  return ultrasounds[static_cast<uint8_t>(id)];
}

// ECHO monte quand le son part, et redescend quand l'écho revient. Sans écho, il redescend tout seul
// au bout de 38 à 60 ms selon le module (58 ms pour le nôtre).
// IRAM_ATTR : appelée depuis une interruption, elle doit être en RAM rapide, comme l'interruption.
void IRAM_ATTR handleEchoChange(Ultrasound& sensor) {
  const uint32_t nowUs = micros();
  if (digitalRead(sensor.echoPin) == HIGH) {
    sensor.echoRiseUs = nowUs;
    sensor.echoStarted = true;
  } else if (sensor.echoStarted) {
    // Sans front montant vu, la durée serait fausse : on ignore cette descente.
    sensor.echoStarted = false;
    portENTER_CRITICAL_ISR(&echoMux);
    sensor.echoDurationUs = nowUs - sensor.echoRiseUs;
    sensor.echoReady = true;
    portEXIT_CRITICAL_ISR(&echoMux);
  }
}

// Une interruption par capteur : chacune sait de quel capteur il s'agit.
void IRAM_ATTR onLeftEcho() {
  handleEchoChange(ultrasounds[static_cast<uint8_t>(UltrasoundId::LEFT)]);
}

void IRAM_ATTR onRightEcho() {
  handleEchoChange(ultrasounds[static_cast<uint8_t>(UltrasoundId::RIGHT)]);
}

// Une impulsion sur les deux TRIG en même temps : les deux capteurs émettent ensemble.
// Pourquoi ensemble, alors que chacun a son TRIG depuis D018 : c'est le fonctionnement déjà écrit et
// réfléchi avec le TRIG commun (D015), et chaque capteur garde une mesure toutes les 60 ms.
// Si le protocole 6 de test_ultrason montre qu'un capteur entend l'autre, on les fera émettre
// chacun leur tour.
void trigger() {
  digitalWrite(PIN_US_LEFT_TRIG, HIGH);
  digitalWrite(PIN_US_RIGHT_TRIG, HIGH);
  // 10 µs d'attente active : c'est la seule attente du programme, 1000 fois plus courte qu'un tour
  // de boucle qui compte. Elle ne gêne pas l'évitement.
  delayMicroseconds(US_TRIGGER_PULSE_US);
  digitalWrite(PIN_US_LEFT_TRIG, LOW);
  digitalWrite(PIN_US_RIGHT_TRIG, LOW);
}

uint16_t echoToMm(uint32_t durationUs) {
  if (durationUs > US_MAX_ECHO_US) {
    return US_NO_OBSTACLE_MM;  // rien dans la portée : c'est normal
  }
  return (uint16_t)(durationUs * US_MM_PER_ECHO_US);
}

void addMeasure(Ultrasound& sensor, uint16_t distanceMm, uint32_t nowMs) {
  sensor.lastMeasuresMm[sensor.nextMeasureIndex] = distanceMm;
  sensor.nextMeasureIndex = (sensor.nextMeasureIndex + 1) % 3;
  // 1. La médiane écarte une mesure aberrante isolée (vers le haut comme vers le bas).
  // 2. Le filtre de montée refuse qu'une distance s'allonge d'un coup sans confirmation.
  const uint16_t medianMm =
      median3(sensor.lastMeasuresMm[0], sensor.lastMeasuresMm[1], sensor.lastMeasuresMm[2]);
  sensor.filteredMm = sensor.riseFilter.update(medianMm);
  sensor.lastValidMeasureMs = nowMs;
}

// Un écho complet est arrivé : on le convertit en distance.
void collectEcho(Ultrasound& sensor, uint32_t nowMs) {
  bool ready = false;
  uint32_t durationUs = 0;
  portENTER_CRITICAL(&echoMux);
  if (sensor.echoReady) {
    ready = true;
    durationUs = sensor.echoDurationUs;
    sensor.echoReady = false;
  }
  portEXIT_CRITICAL(&echoMux);
  if (!ready) {
    return;
  }
  sensor.raw.measures++;
  sensor.raw.echoUs = durationUs;
  sensor.raw.rawMm = echoToMm(durationUs);
  sensor.raw.noEcho = (durationUs > US_MAX_ECHO_US);
  addMeasure(sensor, sensor.raw.rawMm, nowMs);
}

// Aucune mesure valide depuis 1 s : panne (fil débranché, ECHO bloqué, capteur mort).
void checkFault(Ultrasound& sensor, uint32_t nowMs) {
  if (sensor.faulty || nowMs - sensor.lastValidMeasureMs < SENSOR_FAULT_TIMEOUT_MS) {
    return;
  }
  sensor.faulty = true;
  if (DEBUG_LOGS) {
    Console.printf("[SENS] PANNE ultrason %s : aucune mesure depuis 1 s.\n", sensor.name);
  }
}

// Les deux capteurs émettent ensemble (voir trigger()), au plus toutes les
// US_PERIOD_MS, pour que le son précédent et ses échos aient disparu.
// On attend aussi que les deux mesures précédentes soient finies (les deux ECHO à l'état bas) :
// sinon on réessaie au tour de boucle suivant. Mais un ECHO bloqué à l'état haut ne doit pas
// empêcher l'autre capteur de mesurer : après US_ECHO_STUCK_MS, on déclenche quand même.
// Le capteur bloqué ne donne plus de mesure et sera déclaré en panne au bout de 1 s (checkFault).
void triggerBoth(uint32_t nowMs) {
  const uint32_t sinceLastTriggerMs = nowMs - lastTriggerMs;
  if (sinceLastTriggerMs < US_PERIOD_MS) {
    return;
  }
  const bool echoesDone =
      digitalRead(PIN_US_LEFT_ECHO) == LOW && digitalRead(PIN_US_RIGHT_ECHO) == LOW;
  if (!echoesDone && sinceLastTriggerMs < US_ECHO_STUCK_MS) {
    return;
  }
  lastTriggerMs = nowMs;
  trigger();
  for (Ultrasound& sensor : ultrasounds) {
    sensor.raw.triggers++;
  }
}

}  // namespace

void begin() {
  const uint32_t nowMs = millis();
  pinMode(PIN_US_LEFT_TRIG, OUTPUT);
  digitalWrite(PIN_US_LEFT_TRIG, LOW);
  pinMode(PIN_US_RIGHT_TRIG, OUTPUT);
  digitalWrite(PIN_US_RIGHT_TRIG, LOW);
  for (Ultrasound& sensor : ultrasounds) {
    pinMode(sensor.echoPin, INPUT);  // ECHO passe par un pont diviseur 5 V -> 3,3 V
    sensor.lastValidMeasureMs = nowMs;
  }
  attachInterrupt(digitalPinToInterrupt(PIN_US_LEFT_ECHO), onLeftEcho, CHANGE);
  attachInterrupt(digitalPinToInterrupt(PIN_US_RIGHT_ECHO), onRightEcho, CHANGE);
}

void update(uint32_t nowMs) {
  for (Ultrasound& sensor : ultrasounds) {
    collectEcho(sensor, nowMs);
    checkFault(sensor, nowMs);
  }
  triggerBoth(nowMs);
}

uint16_t distanceMm(UltrasoundId id) {
  return sensorFor(id).filteredMm;
}

uint16_t frontDistanceMm() {
  const uint16_t leftMm = distanceMm(UltrasoundId::LEFT);
  const uint16_t rightMm = distanceMm(UltrasoundId::RIGHT);
  return leftMm < rightMm ? leftMm : rightMm;
}

bool isFaulty(UltrasoundId id) {
  return sensorFor(id).faulty;
}

bool isFaulty() {
  return isFaulty(UltrasoundId::LEFT) || isFaulty(UltrasoundId::RIGHT);
}

const char* shortName(UltrasoundId id) {
  return sensorFor(id).shortName;
}

UltrasoundRaw lastRawMeasure(UltrasoundId id) {
  return sensorFor(id).raw;
}

}  // namespace sensors

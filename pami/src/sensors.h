// Couche capteurs : deux ultrasons HC-SR04 à l'avant, un à gauche et un à droite (D012).
//
// Rien n'attend jamais : chaque écho est mesuré par une interruption sur ECHO,
// pas avec pulseIn(), qui bloquerait la boucle jusqu'à 38 ms.
// Chaque capteur a son TRIG (D018), mais les deux émettent toujours ensemble, comme du temps du
// TRIG commun (D015). Un capteur peut donc entendre le son de l'autre :
// - renvoyé par un vrai obstacle : sans conséquence, puisque safety combine les deux par un OU ;
// - passé directement d'un capteur à l'autre : faux obstacle tout proche, le PAMI s'arrête sans
//   raison (côté sûr). À vérifier avec le banc test_ultrason (protocole 6 : rien devant).
#pragma once

#include <stdint.h>

namespace sensors {

enum class UltrasoundId : uint8_t { LEFT = 0, RIGHT = 1 };
constexpr uint8_t ULTRASOUND_COUNT = 2;

void begin();

// À appeler à chaque tour de loop(). Les deux capteurs émettent ensemble, toutes les US_PERIOD_MS.
void update(uint32_t nowMs);

// Distance vue par un capteur, filtrée, en mm : médiane de ses 3 dernières mesures, puis
// confirmation des distances qui s'allongent d'un coup (logic/rise_filter.h).
// US_NO_OBSTACLE_MM quand il n'y a rien dans la portée. 0 au démarrage (« obstacle »).
uint16_t distanceMm(UltrasoundId id);

// La plus courte des deux distances : l'objet le plus proche devant le PAMI.
// Sert au bras, aux messages et au banc test_trajectoire.
// safety, lui, teste chaque capteur séparément puis combine par un OU (même résultat).
uint16_t frontDistanceMm();

// Vrai si ce capteur est en panne (aucune mesure valide depuis SENSOR_FAULT_TIMEOUT_MS).
bool isFaulty(UltrasoundId id);

// Vrai si l'un OU l'autre capteur est en panne. Sécurité par défaut : panne = obstacle.
// Une fois vrai, le reste jusqu'au redémarrage : on ne repart pas avec un capteur douteux.
bool isFaulty();

// Nom court pour les messages : "G" ou "D".
const char* shortName(UltrasoundId id);

// Détail d'un ultrason, pour le banc test_ultrason (le firmware de match ne s'en sert pas).
struct UltrasoundRaw {
  uint32_t triggers;  // impulsions envoyées sur TRIG depuis l'allumage
  uint32_t measures;  // échos complets reçus depuis l'allumage (change à chaque nouvelle mesure)
  uint32_t echoUs;    // durée du dernier écho, en µs
  uint16_t rawMm;     // distance brute du dernier écho, avant la médiane
  bool noEcho;        // vrai si le dernier écho a dépassé la portée (distance brute = 2000 mm)
};
UltrasoundRaw lastRawMeasure(UltrasoundId id);

}  // namespace sensors

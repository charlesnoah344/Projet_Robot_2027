// Couche capteurs : ultrason devant (évitement), 2 capteurs IR tournés vers le sol (zone, D009).
//
// Rien n'attend jamais : l'écho de l'ultrason est mesuré par une interruption sur ECHO,
// pas avec pulseIn(), qui bloquerait la boucle jusqu'à 38 ms.
#pragma once

#include <stdint.h>

namespace sensors {

void begin();

// À appeler à chaque tour de loop(). Chaque capteur est lu à son propre rythme.
void update(uint32_t nowMs);

// Distance devant, filtrée, en mm : médiane des 3 dernières mesures, puis confirmation des
// distances qui s'allongent d'un coup (logic/rise_filter.h).
// US_NO_OBSTACLE_MM quand il n'y a rien dans la portée. 0 au démarrage (« obstacle »).
uint16_t frontDistanceMm();

// Capteurs de sol, à l'avant gauche et à l'avant droit : vrai si le sol renvoie la lumière IR
// (surface claire). Avis de 2 lectures sur 3. Ils ne servent qu'à repérer le bord de la zone :
// l'évitement ne dépend que de l'ultrason.
bool floorLeftReflects();
bool floorRightReflects();

// Vrai si un capteur est en panne (aucune mesure valide depuis 1 s).
// Une fois vrai, le reste jusqu'au redémarrage : on ne repart pas avec un capteur douteux.
bool isFaulty();

// Détail de l'ultrason, pour le banc test_ultrason (le firmware de match ne s'en sert pas).
struct UltrasoundRaw {
  uint32_t triggers;  // impulsions envoyées sur TRIG depuis l'allumage
  uint32_t measures;  // échos complets reçus depuis l'allumage (change à chaque nouvelle mesure)
  uint32_t echoUs;    // durée du dernier écho, en µs
  uint16_t rawMm;     // distance brute du dernier écho, avant la médiane
  bool noEcho;        // vrai si le dernier écho a dépassé la portée (distance brute = 2000 mm)
};
UltrasoundRaw lastRawMeasure();

}  // namespace sensors

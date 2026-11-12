// Liaison Wi-Fi du PAMI (D010) : téléverser et lire les messages sans câble.
//
// - Le PAMI crée son propre réseau (point d'accès) : son adresse est toujours 192.168.4.1.
// - Moniteur : telnet sur le port 23 (pio device monitor -e esp32dev_wifi). Il affiche les
//   messages de Console, et les touches tapées arrivent dans Console.read().
// - Téléversement : ArduinoOTA (pio run -e esp32dev_wifi -t upload), seulement PAMI immobile.
//
// Rien ici ne bloque loop() : sans PC, ou avec un PC qui s'éloigne, le PAMI fonctionne pareil.
// Le firmware de match ne lit aucune touche : on ne peut pas piloter le PAMI par le Wi-Fi.
#pragma once

#include <stdint.h>

namespace wireless {

// Allume le point d'accès, le moniteur telnet et le téléversement sans fil.
void begin();

// À chaque tour de loop() : accepte le PC, reçoit les touches, envoie les messages en attente.
// updateAllowed : vrai seulement quand le PAMI est immobile. Sinon, un téléversement est refusé.
void update(uint32_t nowMs, bool updateAllowed);

// Allume ou éteint la radio. Profil Eurobot : éteinte pendant le match (règlement général F.6).
// Radio éteinte, les messages s'accumulent dans le tampon et partent à la reconnexion.
void setRadioOn(bool on);

}  // namespace wireless

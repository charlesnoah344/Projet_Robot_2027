// États de la machine à états d'exploration (voir pami/CLAUDE.md).
#pragma once

enum class PamiState {
  IDLE,     // moteurs arrêtés, attend l'appui sur BOOT
  FORWARD,  // avance tout droit, vitesse régulée
  STOP,     // obstacle devant : arrêt, puis court temps d'attente
  DECIDE,   // obstacle toujours là : choisit de tourner ou de reculer
  BACKUP,   // recule un peu
  TURN,     // tourne sur place d'un angle mesuré
  END,      // 99,5 s : tout est arrêté, définitivement
  FAULT     // capteur en panne : arrêt jusqu'au redémarrage de la carte
};

// Nom lisible pour les logs (sans accents : le moniteur série les affiche mal).
inline const char* stateName(PamiState state) {
  switch (state) {
    case PamiState::IDLE: return "IDLE";
    case PamiState::FORWARD: return "FORWARD";
    case PamiState::STOP: return "STOP";
    case PamiState::DECIDE: return "DECIDE";
    case PamiState::BACKUP: return "BACKUP";
    case PamiState::TURN: return "TURN";
    case PamiState::END: return "END";
    case PamiState::FAULT: return "FAULT";
  }
  return "?";
}

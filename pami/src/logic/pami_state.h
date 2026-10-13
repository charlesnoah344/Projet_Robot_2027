// États de la machine à états du PAMI défenseur (voir docs/plan-logiciel-pami.md, § 3).
#pragma once

enum class PamiState {
  WAIT_START,  // allumé, immobile : attend l'appui sur le bouton de départ
  WAIT_DELAY,  // départ donné : immobile jusqu'à MATCH_MOVE_ALLOWED_MS (5 s)
  MISSION,     // suit les étapes de la mission (points de passage)
  AVOID_STOP,  // obstacle devant : arrêté, attend qu'il s'en aille
  DETOUR,      // obstacle toujours là : petit détour du côté libre
  DEFEND,      // arrivé dans la zone : immobile, le bras touche l'adversaire à portée
  END,         // fin du temps (14,5 s) : tout est arrêté, définitivement
  FAULT        // capteur en panne : arrêt jusqu'au redémarrage de la carte
};

// Nom lisible pour les logs (sans accents : le moniteur série les affiche mal).
inline const char* stateName(PamiState state) {
  switch (state) {
    case PamiState::WAIT_START: return "WAIT_START";
    case PamiState::WAIT_DELAY: return "WAIT_DELAY";
    case PamiState::MISSION: return "MISSION";
    case PamiState::AVOID_STOP: return "AVOID_STOP";
    case PamiState::DETOUR: return "DETOUR";
    case PamiState::DEFEND: return "DEFEND";
    case PamiState::END: return "END";
    case PamiState::FAULT: return "FAULT";
  }
  return "?";
}

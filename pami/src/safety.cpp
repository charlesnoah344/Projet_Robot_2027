#include "safety.h"

#include "motor_driver.h"

namespace safety {

void apply(const WheelCommand& requested, uint32_t nowMs) {
  (void)requested;
  (void)nowMs;
  // Étape 1 : les capteurs d'obstacles n'existent pas encore, on ne peut donc pas savoir
  // si la voie est libre. Sécurité par défaut : aucune commande ne passe, moteurs freinés.
  // Le vrai filtre (fin de match, panne, obstacle) arrive à l'étape 5, avec l'ultrason.
  motorDriver::brake();
}

}  // namespace safety

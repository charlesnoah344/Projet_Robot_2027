#include "motion.h"

namespace motion {

namespace {

WheelCommand requested;  // {0, 0} : arrêt

}  // namespace

void begin() {}

void update(uint32_t nowMs) {
  (void)nowMs;  // étape 4 : régulation de vitesse toutes les 20 ms
}

WheelCommand command() {
  return requested;
}

}  // namespace motion

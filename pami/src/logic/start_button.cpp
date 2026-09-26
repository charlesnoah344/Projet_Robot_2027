#include "start_button.h"

StartButtonDetector::StartButtonDetector(uint32_t debounceMs) : debounceMs_(debounceMs) {}

bool StartButtonDetector::update(bool pressed, uint32_t nowMs) {
  // On note l'instant de chaque changement : un état ne compte que s'il reste stable.
  if (!hasSample_ || pressed != lastPressed_) {
    hasSample_ = true;
    lastPressed_ = pressed;
    lastChangeMs_ = nowMs;
  }
  // Soustraction : reste juste quand millis() repasse par zéro.
  const bool isStable = (nowMs - lastChangeMs_) >= debounceMs_;

  if (fired_ || !isStable) {
    return false;
  }
  if (!armed_) {
    // Pas encore armé : on attend de voir le bouton relâché.
    if (!pressed) {
      armed_ = true;
    }
    return false;
  }
  if (pressed) {
    fired_ = true;
    return true;
  }
  return false;
}

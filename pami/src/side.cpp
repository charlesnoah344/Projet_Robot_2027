#include "side.h"

#include <Arduino.h>

#include "config.h"
#include "console.h"

namespace side {

namespace {

StartSide selected = StartSide::LEFT;
bool chosen = false;

}  // namespace

void choose(StartSide newSide) {
  if (chosen) {
    return;  // un seul choix par allumage : le côté ne change plus pendant le match
  }
  selected = newSide;
  chosen = true;
  if (DEBUG_LOGS) {
    const bool left = (selected == StartSide::LEFT);
    Console.printf("[COTE] %s (bord a %s, centre a %s)\n", sideName(selected), left ? "gauche" : "droite",
                   left ? "droite" : "gauche");
  }
}

bool isChosen() {
  return chosen;
}

StartSide current() {
  return selected;
}

}  // namespace side

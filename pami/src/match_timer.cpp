#include "match_timer.h"

#include <Arduino.h>

#include "config.h"
#include "console.h"
#include "logic/match_clock.h"
#include "logic/start_command.h"

namespace matchTimer {

namespace {

StartCommandDetector startCommand;
bool started = false;
bool moveAllowed = false;
bool over = false;
uint32_t startMs = 0;

// Lit tout ce qui a été tapé dans le moniteur, sans jamais attendre : available() dit combien de
// caractères sont déjà arrivés. Seule la commande de départ fait quelque chose (D013).
void readStartCommand(uint32_t nowMs) {
  while (Console.available() > 0) {
    switch (startCommand.feed((char)Console.read())) {
      case CommandResult::START:
        started = true;
        startMs = nowMs;
        if (DEBUG_LOGS) {
          Console.printf("[TIME] DEPART ! Mouvement dans %.1f s, arret total a %.1f s.\n",
                         MATCH_MOVE_ALLOWED_MS / 1000.0f, MATCH_STOP_MS / 1000.0f);
        }
        break;
      case CommandResult::UNKNOWN:
        if (DEBUG_LOGS) {
          Console.printf("[TIME] Commande inconnue. Pour le depart : tapez %s puis Entree.\n",
                         START_COMMAND);
        }
        break;
      case CommandResult::IGNORED:
        if (DEBUG_LOGS) {
          Console.println("[TIME] Depart deja donne : commande ignoree.");
        }
        break;
      case CommandResult::NONE:
        break;
    }
  }
}

}  // namespace

void begin() {
  // Pas de broche à configurer : le départ arrive par la console (Wi-Fi ou USB).
  // Ce message attend dans le tampon du Wi-Fi : il s'affiche dès que le moniteur se connecte.
  if (DEBUG_LOGS) {
    Console.printf("[TIME] Pret : tapez %s puis Entree dans le moniteur pour donner le depart.\n",
                   START_COMMAND);
  }
}

void update(uint32_t nowMs) {
  readStartCommand(nowMs);

  if (started && !over && isMatchOver(startMs, nowMs)) {
    over = true;  // verrouillé : une fois fini, c'est fini
    if (DEBUG_LOGS) {
      Console.printf("[TIME] FIN (%.1f s) : tout s'arrete.\n", MATCH_STOP_MS / 1000.0f);
    }
  }

  const bool wasMoveAllowed = moveAllowed;
  moveAllowed = started && !over && isInMovePhase(startMs, nowMs);
  if (DEBUG_LOGS && moveAllowed && !wasMoveAllowed) {
    Console.println("[TIME] Mouvement autorise.");
  }
}

bool isStarted() {
  return started;
}

bool isMoveAllowed() {
  return moveAllowed;
}

bool isOver() {
  return over;
}

uint32_t elapsedMs(uint32_t nowMs) {
  return started ? elapsedSinceMs(startMs, nowMs) : 0;
}

}  // namespace matchTimer

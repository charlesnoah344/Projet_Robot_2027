#include "match_timer.h"

#include <Arduino.h>

#include "config.h"
#include "console.h"
#include "logic/match_clock.h"
#include "logic/start_command.h"
#include "side.h"

namespace matchTimer {

namespace {

StartCommandDetector startCommand;
bool started = false;
bool moveAllowed = false;
bool over = false;
uint32_t startMs = 0;

void printStartHelp() {
  if (DEBUG_LOGS) {
    Console.printf("[TIME] Pour le depart, tapez %s (PAMI GAUCHE : bord a gauche, centre a droite)\n",
                   START_COMMAND_LEFT);
    Console.printf("[TIME]   ou %s (PAMI DROITE : bord a droite, centre a gauche), puis Entree.\n",
                   START_COMMAND_RIGHT);
  }
}

// Lit tout ce qui a été tapé dans le moniteur, sans jamais attendre : available() dit combien de
// caractères sont déjà arrivés. Seule la commande de départ fait quelque chose (D013, D020).
void readStartCommand(uint32_t nowMs) {
  while (Console.available() > 0) {
    switch (startCommand.feed((char)Console.read())) {
      case CommandResult::START:
        // Le côté d'abord : la machine à états le lit dès qu'elle voit le départ.
        side::choose(startCommand.startSide());
        started = true;
        startMs = nowMs;
        if (DEBUG_LOGS) {
          Console.printf("[TIME] DEPART ! Mouvement dans %.1f s, arret total a %.1f s.\n",
                         MATCH_MOVE_ALLOWED_MS / 1000.0f, MATCH_STOP_MS / 1000.0f);
        }
        break;
      case CommandResult::MISSING_SIDE:
        if (DEBUG_LOGS) {
          Console.println("[TIME] Il manque le cote : pas de depart.");
        }
        printStartHelp();
        break;
      case CommandResult::UNKNOWN:
        if (DEBUG_LOGS) {
          Console.println("[TIME] Commande inconnue.");
        }
        printStartHelp();
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
    Console.println("[TIME] Pret.");
  }
  printStartHelp();
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

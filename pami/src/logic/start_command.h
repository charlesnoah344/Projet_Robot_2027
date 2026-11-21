// Commande de départ tapée dans le moniteur, Wi-Fi ou USB (logique pure, testable sur PC, D013).
//
// Pourquoi ces règles :
// - le PAMI ne part jamais tout seul : il faut taper un mot précis (START_COMMAND, config.h), puis
//   Entrée. Une touche frappée par erreur ne suffit pas ;
// - majuscules et espaces ne comptent pas : « GO » et « go  » marchent aussi ;
// - un seul départ par allumage : les commandes suivantes sont ignorées.
//
// ⚠ Interdit à Eurobot (règlement général F.6 : ni télécommande, ni communication avec l'extérieur
// de la table pendant le match). C'est le départ du profil labo seulement.
#pragma once

#include <stdint.h>

#include "../config.h"

enum class CommandResult {
  NONE,     // ligne pas encore terminée, ou ligne vide
  START,    // la commande de départ vient d'être validée (une seule fois par allumage)
  UNKNOWN,  // une ligne a été tapée avant le départ, mais ce n'est pas la commande de départ
  IGNORED   // une ligne a été tapée après le départ : elle ne fait rien
};

class StartCommandDetector {
public:
  // Reçoit un caractère tapé dans le moniteur.
  CommandResult feed(char c) {
    if (c == '\r' || c == '\n') {
      return endOfLine();
    }
    // Espaces et caractères de contrôle (effacement, tabulation...) : ignorés.
    if (c <= ' ' || c == 127) {
      return CommandResult::NONE;
    }
    if (length_ < MAX_LENGTH) {
      line_[length_] = toLower(c);
    }
    // Au-delà de MAX_LENGTH, on compte seulement : une ligne trop longue n'est jamais la commande.
    if (length_ <= MAX_LENGTH) {
      length_++;
    }
    return CommandResult::NONE;
  }

  bool hasStarted() const { return started_; }

private:
  static constexpr uint8_t MAX_LENGTH = 16;
  static_assert(sizeof(START_COMMAND) - 1 <= MAX_LENGTH, "START_COMMAND trop long");

  static char toLower(char c) { return (c >= 'A' && c <= 'Z') ? (char)(c - 'A' + 'a') : c; }

  bool lineIsStartCommand() const {
    if (length_ != sizeof(START_COMMAND) - 1) {
      return false;
    }
    for (uint8_t i = 0; i < length_; i++) {
      if (line_[i] != START_COMMAND[i]) {
        return false;
      }
    }
    return true;
  }

  CommandResult endOfLine() {
    // Entrée seule, ou le \n qui suit un \r : rien à décider.
    if (length_ == 0) {
      return CommandResult::NONE;
    }
    const bool isStart = lineIsStartCommand();
    length_ = 0;  // ligne suivante
    if (started_) {
      return CommandResult::IGNORED;
    }
    if (isStart) {
      started_ = true;
      return CommandResult::START;
    }
    return CommandResult::UNKNOWN;
  }

  char line_[MAX_LENGTH] = {};
  uint8_t length_ = 0;     // nombre de caractères de la ligne en cours (au plus MAX_LENGTH + 1)
  bool started_ = false;   // true une fois le départ donné
};

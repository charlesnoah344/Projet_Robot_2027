// Commande de départ tapée dans le moniteur, Wi-Fi ou USB (logique pure, testable sur PC, D013).
// Elle donne aussi le côté du PAMI : « go g » pour GAUCHE, « go d » pour DROITE (D020).
//
// Pourquoi ces règles :
// - le PAMI ne part jamais tout seul : il faut taper une commande précise (config.h), puis Entrée.
//   Une touche frappée par erreur ne suffit pas ;
// - « go » seul est refusé : le côté décide du trajet et du sens des détours, il doit toujours être
//   choisi exprès, jamais par défaut ;
// - majuscules et espaces ne comptent pas : « GO G », « go  g » et « gog » marchent aussi ;
// - un seul départ par allumage : les commandes suivantes sont ignorées.
//
// ⚠ Interdit à Eurobot (règlement général F.6 : ni télécommande, ni communication avec l'extérieur
// de la table pendant le match). C'est le départ du profil labo seulement.
#pragma once

#include <stdint.h>

#include "../config.h"
#include "mission_step.h"

enum class CommandResult {
  NONE,          // ligne pas encore terminée, ou ligne vide
  START,         // départ validé (une seule fois par allumage) : le côté est dans startSide()
  MISSING_SIDE,  // « go » seul : il manque le côté
  UNKNOWN,       // une ligne a été tapée avant le départ, mais ce n'est pas une commande de départ
  IGNORED        // une ligne a été tapée après le départ : elle ne fait rien
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

  // Côté choisi avec la commande de départ. N'a de sens qu'après le départ.
  StartSide startSide() const { return side_; }

private:
  static constexpr uint8_t MAX_LENGTH = 16;
  static_assert(sizeof(START_COMMAND_LEFT) - 1 <= MAX_LENGTH, "START_COMMAND_LEFT trop long");
  static_assert(sizeof(START_COMMAND_RIGHT) - 1 <= MAX_LENGTH, "START_COMMAND_RIGHT trop long");

  static char toLower(char c) { return (c >= 'A' && c <= 'Z') ? (char)(c - 'A' + 'a') : c; }

  // La ligne tapée (sans espaces, en minuscules) est-elle cette commande ? Les espaces de la
  // commande ne comptent pas non plus : « go g » correspond à la ligne « gog ».
  bool lineIs(const char* command) const {
    uint8_t i = 0;
    for (const char* c = command; *c != '\0'; c++) {
      if (*c == ' ') {
        continue;
      }
      if (i >= length_ || line_[i] != *c) {
        return false;
      }
      i++;
    }
    return i == length_;
  }

  CommandResult endOfLine() {
    // Entrée seule, ou le \n qui suit un \r : rien à décider.
    if (length_ == 0) {
      return CommandResult::NONE;
    }
    const bool isLeft = lineIs(START_COMMAND_LEFT);
    const bool isRight = lineIs(START_COMMAND_RIGHT);
    const bool isWithoutSide = lineIs(START_COMMAND_WITHOUT_SIDE);
    length_ = 0;  // ligne suivante
    if (started_) {
      return CommandResult::IGNORED;
    }
    if (isLeft || isRight) {
      started_ = true;
      side_ = isLeft ? StartSide::LEFT : StartSide::RIGHT;
      return CommandResult::START;
    }
    if (isWithoutSide) {
      return CommandResult::MISSING_SIDE;
    }
    return CommandResult::UNKNOWN;
  }

  char line_[MAX_LENGTH] = {};
  uint8_t length_ = 0;     // nombre de caractères de la ligne en cours (au plus MAX_LENGTH + 1)
  bool started_ = false;   // true une fois le départ donné
  StartSide side_ = StartSide::LEFT;
};

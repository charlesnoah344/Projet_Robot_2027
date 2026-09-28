// Détection d'un appui volontaire sur le bouton de départ (logique pure, testable sur PC).
//
// Pourquoi ces règles :
// - le PAMI ne doit jamais partir tout seul à l'allumage. Le départ n'est donc « armé »
//   qu'après avoir vu le bouton relâché pendant au moins debounceMs. Un bouton maintenu
//   ou coincé au démarrage ne peut pas lancer le PAMI ;
// - un appui doit durer au moins debounceMs pour compter : les contacts rebondissent ;
// - un seul départ par allumage : les appuis suivants sont ignorés.
#pragma once

#include <stdint.h>

class StartButtonDetector {
public:
  explicit StartButtonDetector(uint32_t debounceMs);

  // pressed : état brut du bouton (true = appuyé).
  // Renvoie true une seule fois : au moment où le départ est validé.
  bool update(bool pressed, uint32_t nowMs);

  // Vrai quand le bouton a été vu relâché assez longtemps : un appui lancera le départ.
  bool isArmed() const { return armed_; }

private:
  uint32_t debounceMs_;
  bool hasSample_ = false;     // false tant qu'on n'a jamais lu le bouton
  bool lastPressed_ = false;   // dernier état brut lu
  uint32_t lastChangeMs_ = 0;  // instant du dernier changement d'état brut
  bool armed_ = false;
  bool fired_ = false;         // true une fois le départ donné
};

// Couche mission : déroule la liste d'étapes de la mission choisie (ACTIVE_MISSION, config.h).
//
// Une étape à la fois. Pour chaque étape, logic/next_move.h choisit le mouvement suivant
// (tourner ou avancer) d'après la position mesurée, et motion l'exécute.
#pragma once

#include "config.h"
#include "logic/mission_step.h"

namespace mission {

// Démarre la mission, avec la couleur verrouillée au départ.
// id : la mission à jouer. Le firmware de match prend ACTIVE_MISSION ; le banc de test
// test_odometrie choisit la sienne au clavier.
void start(TeamColor color, MissionId id = ACTIVE_MISSION);

// À appeler à chaque tour de loop() pendant l'état MISSION.
// Lance le mouvement suivant quand le précédent est terminé.
void update();

// Vrai quand toutes les étapes sont faites.
bool isFinished();

// Vrai quand la mission arrive à l'étape DEFEND (dans la zone, bien orienté).
bool isDefendReached();

}  // namespace mission

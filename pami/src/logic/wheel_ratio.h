// Rapport des diamètres droite / gauche à partir de l'écart latéral d'une ligne droite
// (logique pure, testable sur PC, D023).
//
// Pourquoi : si une roue avance un peu plus que l'autre à chaque tick, le PAMI croit aller droit
// (mêmes distances calculées des deux côtés), mais il décrit en vrai un grand arc de cercle.
//
// Le calcul : sur une ligne de L mm, si la roue droite avance d'une fraction ε de plus que la
// gauche, le cap tourne de L × ε / entraxe (en radians), régulièrement tout au long de la ligne.
// L'écart sur le côté à l'arrivée vaut alors environ e = L² × ε / (2 × entraxe). Donc :
//   ε = 2 × entraxe × e / L².
// e > 0 : le PAMI finit à GAUCHE (la roue droite avance trop) ; e < 0 : à droite.
// Exemple (T022) : 25 mm à gauche sur 800 mm, entraxe 101,6 mm -> ε = 0,0079 (0,79 %).
#pragma once

// Écart relatif entre les deux roues qui explique un écart latéral de lateralMm sur lineMm.
inline float wheelRatioError(float lateralMm, float lineMm, float wheelBaseMm) {
  return 2.0f * wheelBaseMm * lateralMm / (lineMm * lineMm);
}

// Nouveau rapport droite / gauche à mettre dans config.h, à partir du rapport actuel et de
// l'écart moyen relatif mesuré avec ce rapport (wheelRatioError).
// Le PAMI finit encore à gauche (erreur > 0) : la roue droite avance encore trop par tick, on
// augmente son diamètre dans le calcul, elle fera donc moins de tours pour la même distance.
inline float correctedWheelRatio(float currentRatio, float meanRatioError) {
  return currentRatio * (1.0f + meanRatioError);
}

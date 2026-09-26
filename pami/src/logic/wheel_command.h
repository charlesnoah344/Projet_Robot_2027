// Commande des deux roues, transmise de motion à safety.
#pragma once

// En pourcentage de PWM : +100 = pleine vitesse en avant, -100 = pleine vitesse en arrière,
// 0 = frein.
struct WheelCommand {
  float leftPct = 0.0f;
  float rightPct = 0.0f;
};

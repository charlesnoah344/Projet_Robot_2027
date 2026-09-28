// Banc de test des moteurs et du driver MDD3A (étape 2).
//
// !!! PROGRAMME DE TEST : AUCUN ÉVITEMENT. NE JAMAIS L'UTILISER EN MATCH. !!!
// Après le test, remettre le firmware : pio run -t upload
//
// Téléverser : pio run -e test_moteurs -t upload (maintenir BOOT pendant « Connecting »)
// Moniteur   : pio device monitor -b 115200
//
// MATÉRIEL ET MONTAGE (docs/guide-pratique-pami.md, § 3)
// - ESP32 branché en USB : c'est lui qui alimente la carte.
// - MDD3A : entrées M1A/M1B sur GPIO 25/26 (gauche), M2A/M2B sur GPIO 33/32 (droit),
//   GND commun avec l'ESP32.
// - 2 moteurs FIT0450 sur les sorties du MDD3A. Encodeurs et capteurs NON branchés.
// - Alimentation de labo réglée à 6 V, limite de courant à environ 2 A, à travers le BAU.
//
// SÉCURITÉ
// - PAMI sur cales, roues en l'air (sauf pour la mesure de la zone morte au sol).
// - BAU câblé en série sur l'alimentation du MDD3A, à portée de main.
// - Au démarrage, rien ne bouge avant une commande.
// - Espace, « s », ou n'importe quelle touche inconnue : STOP.
// - Arrêt automatique après 10 s sans commande.
// - Attention : si on relâche le BAU pendant qu'une commande est active, les roues repartent.
//   Tapez « s » avant de relâcher le BAU.
//
// PROTOCOLE (détails dans le message de l'étape 2)
//  1. Broches flottantes : alimentation allumée, appuyer 3 fois sur EN. Aucune roue ne doit bouger.
//  2. Sens : « g » puis « d » : une seule roue tourne, la bonne, vers l'avant du PAMI.
//  3. « a » (les deux en avant) puis « r » (les deux en arrière).
//  4. Zone morte sur cales : vitesse à 0 %, « g », puis « > » jusqu'à ce que la roue tourne
//     régulièrement. Noter le pourcentage. Même chose avec « d ».
//  5. Zone morte au sol : même méthode avec « a ». Noter le pourcentage où le PAMI avance.
//  6. Arrêt : à 100 % (« x » puis « a »), taper « s » : les roues s'arrêtent net. 3 fois.
//  7. Chute de tension : 10 départs à 100 % (« a » puis « s »). L'ESP32 ne doit jamais redémarrer.
//  8. BAU : à 50 %, appuyer sur le BAU. Les roues s'arrêtent, l'ESP32 continue de tourner.
//
// CRITÈRES DE RÉUSSITE
// - g, d, a, r : la bonne roue, dans le bon sens (après correction éventuelle dans config.h).
// - 0 mouvement pendant les 3 redémarrages.
// - Zone morte mesurée pour chaque roue (sur cales) et pour le PAMI (au sol).
// - Arrêt en moins de 0,2 s : à l'œil, instantané, sans roue libre.
// - 0 redémarrage de l'ESP32 sur 10 départs à 100 %.
// - BAU : arrêt immédiat des roues, l'ESP32 ne redémarre pas.
#include <Arduino.h>
#include <esp_system.h>

#include "config.h"
#include "motor_driver.h"

namespace {

enum class Drive { STOPPED, BOTH_FORWARD, BOTH_BACKWARD, LEFT_ONLY, RIGHT_ONLY };

Drive drive = Drive::STOPPED;
int speedPct = TEST_BENCH_START_PWM_PCT;
uint32_t lastCommandMs = 0;

// Affiche la cause du dernier redémarrage. « BROWNOUT » = chute de tension quand les moteurs démarrent.
void printResetReason() {
  const char* reason = "autre";
  switch (esp_reset_reason()) {
    case ESP_RST_POWERON: reason = "mise sous tension ou bouton EN"; break;
    case ESP_RST_SW: reason = "logiciel"; break;
    case ESP_RST_PANIC: reason = "plantage du programme"; break;
    case ESP_RST_BROWNOUT: reason = "BROWNOUT : CHUTE DE TENSION !"; break;
    case ESP_RST_INT_WDT:
    case ESP_RST_TASK_WDT:
    case ESP_RST_WDT: reason = "chien de garde (programme bloque)"; break;
    default: break;
  }
  Serial.printf("[BANC] Cause du redemarrage : %s\n", reason);
}

void printHelp() {
  Serial.println("[BANC] Commandes (une touche, sans Entree) :");
  Serial.println("[BANC]   a = les deux roues en avant     r = les deux roues en arriere");
  Serial.println("[BANC]   g = roue GAUCHE seule en avant  d = roue DROITE seule en avant");
  Serial.println("[BANC]   + / - = vitesse +5 / -5 %       > / < = vitesse +1 / -1 %");
  Serial.println("[BANC]   x = vitesse 100 %               h = cette aide");
  Serial.println("[BANC]   s, espace ou toute autre touche = STOP");
}

void applyDrive() {
  float leftPct = 0.0f;
  float rightPct = 0.0f;
  switch (drive) {
    case Drive::BOTH_FORWARD: leftPct = speedPct; rightPct = speedPct; break;
    case Drive::BOTH_BACKWARD: leftPct = -speedPct; rightPct = -speedPct; break;
    case Drive::LEFT_ONLY: leftPct = speedPct; break;
    case Drive::RIGHT_ONLY: rightPct = speedPct; break;
    case Drive::STOPPED: break;
  }
  motorDriver::setWheelPercent(leftPct, rightPct);
  Serial.printf("[MOT] vitesse %d %% | gauche = %+d %% | droite = %+d %%\n", speedPct, (int)leftPct,
                (int)rightPct);
}

void handleKey(char key, uint32_t nowMs) {
  key = tolower(key);
  switch (key) {
    case '\r':
    case '\n':
      return;  // touche Entrée : ignorée, pour ne pas arrêter par erreur après une commande
    case 'h':
    case '?':
      printHelp();
      return;
    case 'a': drive = Drive::BOTH_FORWARD; break;
    case 'r': drive = Drive::BOTH_BACKWARD; break;
    case 'g': drive = Drive::LEFT_ONLY; break;
    case 'd': drive = Drive::RIGHT_ONLY; break;
    case '+': speedPct += TEST_BENCH_PWM_STEP_PCT; break;
    case '-': speedPct -= TEST_BENCH_PWM_STEP_PCT; break;
    case '>': speedPct += TEST_BENCH_PWM_FINE_STEP_PCT; break;
    case '<': speedPct -= TEST_BENCH_PWM_FINE_STEP_PCT; break;
    case 'x': speedPct = MOTOR_MAX_PWM_PCT; break;
    default: drive = Drive::STOPPED; break;  // s, espace, ou touche inconnue : STOP
  }
  speedPct = constrain(speedPct, 0, MOTOR_MAX_PWM_PCT);
  lastCommandMs = nowMs;
  applyDrive();  // une nouvelle vitesse s'applique tout de suite si une roue tourne
}

}  // namespace

void setup() {
  // En tout premier : moteurs freinés.
  motorDriver::begin();

  Serial.begin(SERIAL_BAUD);
  Serial.println();
  Serial.println("[BANC] ===== BANC DE TEST MOTEURS (etape 2) =====");
  Serial.println("[BANC] PAS D'EVITEMENT : NE JAMAIS UTILISER EN MATCH.");
  Serial.println("[BANC] Apres le test : pio run -t upload (remet le firmware).");
  printResetReason();
  printHelp();
  Serial.printf("[MOT] Moteurs arretes. Vitesse de depart : %d %%\n", speedPct);
}

void loop() {
  const uint32_t nowMs = millis();

  while (Serial.available() > 0) {
    handleKey((char)Serial.read(), nowMs);
  }

  // Sécurité : si personne n'a tapé de commande depuis 10 s, on arrête.
  if (drive != Drive::STOPPED && nowMs - lastCommandMs >= TEST_BENCH_AUTO_STOP_MS) {
    drive = Drive::STOPPED;
    Serial.println("[MOT] Arret automatique (10 s sans commande).");
    applyDrive();
  }
}

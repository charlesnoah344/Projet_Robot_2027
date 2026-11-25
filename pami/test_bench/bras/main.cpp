// Banc de test du bras d'attaque : servo SG90 (GPIO 13), attaque à 120°, 3 s, puis retour (D021).
//
// !!! PROGRAMME DE TEST : AUCUN MOUVEMENT DES ROUES. NE JAMAIS L'UTILISER EN MATCH. !!!
// Après le test, remettre le firmware : pio run -t upload (sans fil : pio run -e esp32dev_wifi -t upload)
//
// Téléverser : pio run -e test_bras -t upload -t monitor (maintenir BOOT pendant « Connecting »)
// Sans fil (D010) : pio run -e test_bras_wifi -t upload, puis pio device monitor -e test_bras_wifi.
//   Téléversement accepté seulement bras au repos (ni attaque, ni mode défense).
//
// Ce banc utilise le VRAI code du match : la séquence d'attaque (logic/arm_attack.h), le calcul de
// l'impulsion (logic/servo_pulse.h) et les ultrasons (sensors.cpp). Seule différence avec le match :
// ici, pas de minuteur, le bras bouge dès qu'on tape une commande.
//
// MATÉRIEL ET MONTAGE
// - Servo SG90 sur le connecteur du servo (GPIO 13), alimenté par « Servo + » du PCB, JAMAIS par
//   le 3,3 V de l'ESP32 : un servo qui démarre tire un pic de courant qui ferait redémarrer l'ESP32.
// - Pour la commande « u » : les ultrasons branchés (au moins un des deux).
// - Les moteurs restent freinés en permanence.
//
// SÉCURITÉ
// - Aucun PWM sur le servo avant la première commande : il ne bouge pas à l'allumage.
// - Doigts hors de la course du bras pendant les essais.
// - « c » coupe le servo : le bras devient libre, sans force.
//
// COMMANDES : une lettre, un nombre si besoin, puis Entrée.
//   r        bras au repos (ARM_REST_DEG = 0°)
//   a        une attaque : 120°, 3 s, puis retour à 0°, comme en match
//   x 45     aller à un angle au choix, de 0 à 180° (pour régler les angles)
//   u        mode défense : attaque dès qu'un ultrason voit un objet à moins de 60 mm,
//            exactement comme dans la douve en match. N'importe quelle touche arrête ce mode.
//   c        couper le servo (bras libre)
//   i        réglages       h  aide
//
// PROTOCOLE PROPOSÉ
//  1. Servo seul, sans bras : « r », puis « x 90 » et « x 120 ». Vérifier au rapporteur le sens de
//     rotation et les angles. Angle faux : régler SERVO_MIN_PULSE_US / SERVO_MAX_PULSE_US (config.h).
//  2. Bras monté, au repos (« r ») : il doit rester dans le périmètre du PAMI (≤ 600 mm au départ).
//  3. « a » : le bras va à 120°, y reste 3 s, revient. Vérifier :
//     - qu'il ne touche aucune partie du PAMI pendant sa course ;
//     - le périmètre bras déployé : ≤ 700 mm (règlement général F.7) ; la hauteur : ≤ 350 mm ;
//     - la durée affichée (3 s) et que le bras revient bien à 0°.
//  4. « u » : approcher une main devant un ultrason, à moins de 6 cm. Attaque, 3 s, retour.
//     Main laissée devant : nouvelle attaque après le retour. Main retirée : le bras reste au repos.
//  5. Pendant 3 et 4, regarder la ligne « Cause du redemarrage » : un BROWNOUT veut dire que
//     l'alimentation du servo fait chuter celle de l'ESP32.
//
// CRITÈRES DE RÉUSSITE
// - Angles à ± 5° (0° et 120°), tenue de 3,0 s, retour complet à 0°.
// - Aucun contact du bras avec le PAMI, périmètre déployé ≤ 700 mm.
// - Mode défense : attaque à moins de 60 mm, jamais au-delà de 10 cm.
// - Aucun redémarrage de l'ESP32.
#include <Arduino.h>
#include <ctype.h>
#include <esp_system.h>
#include <stdlib.h>

#include "config.h"
#include "console.h"
#include "logic/arm_attack.h"
#include "logic/servo_pulse.h"
#include "motor_driver.h"
#include "sensors.h"
#include "wireless.h"

using sensors::UltrasoundId;

namespace {

// Ce que fait le banc en ce moment.
enum class Mode {
  IDLE,    // le bras tient sa position (ou il est libre), on attend une commande
  ATTACK,  // une attaque commandée par « a » : 120°, 3 s, retour
  DEFEND   // « u » : attaque dès qu'un ultrason voit un objet à moins de ARM_TRIGGER_DISTANCE_MM
};

Mode mode = Mode::IDLE;
ArmAttack attack;
bool attached = false;  // le servo reçoit du PWM
float currentAngleDeg = -1.0f;  // dernier angle commandé (-1 : aucun)
uint32_t lastLogMs = 0;
constexpr uint32_t DEFEND_LOG_PERIOD_MS = 500;  // une ligne de suivi toutes les 0,5 s en mode « u »

// --- Ligne de commande tapée ---
char line[24];
size_t lineLength = 0;
bool lastWasCr = false;

// Affiche la cause du dernier redémarrage. « BROWNOUT » = chute de tension (servo mal alimenté ?).
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
  Console.printf("[BANC] Cause du redemarrage : %s\n", reason);
}

void printSettings() {
  Console.printf("[BRAS] Servo GPIO %u, canal LEDC %u, %lu Hz. Impulsion %lu us a 0 deg, %lu us a 180 deg.\n", PIN_SERVO,
                 LEDC_CHANNEL_SERVO, (unsigned long)SERVO_PWM_FREQ_HZ, (unsigned long)SERVO_MIN_PULSE_US,
                 (unsigned long)SERVO_MAX_PULSE_US);
  Console.printf("[BRAS] Repos %.0f deg, attaque %.0f deg pendant %.1f s, retour en %.1f s.\n", ARM_REST_DEG,
                 ARM_ATTACK_DEG, ARM_ATTACK_HOLD_MS / 1000.0f, ARM_RETURN_MS / 1000.0f);
  Console.printf("[BRAS] Mode defense : attaque si un ultrason voit un objet a moins de %u mm.\n",
                 ARM_TRIGGER_DISTANCE_MM);
}

void printHelp() {
  Console.println("[BRAS] Commandes : une lettre, un nombre si besoin, puis Entree.");
  Console.println("[BRAS]   r : repos (0 deg)      a : une attaque (120 deg, 3 s, retour)");
  Console.println("[BRAS]   x 45 : aller a 45 deg (0 a 180)");
  Console.println("[BRAS]   u : mode defense (ultrasons), n'importe quelle touche pour l'arreter");
  Console.println("[BRAS]   c : couper le servo    i : reglages    h : aide");
}

// --- Servo ---

void moveTo(float angleDeg) {
  if (!attached) {
    ledcSetup(LEDC_CHANNEL_SERVO, SERVO_PWM_FREQ_HZ, SERVO_PWM_RESOLUTION_BITS);
    ledcAttachPin(PIN_SERVO, LEDC_CHANNEL_SERVO);
    attached = true;
  }
  if (angleDeg != currentAngleDeg) {
    currentAngleDeg = angleDeg;
    Console.printf("[BRAS] Servo -> %.0f deg (impulsion %lu us)\n", angleDeg,
                   (unsigned long)(SERVO_MIN_PULSE_US + angleDeg / 180.0f * (SERVO_MAX_PULSE_US - SERVO_MIN_PULSE_US)));
  }
  ledcWrite(LEDC_CHANNEL_SERVO, servoDutyForAngle(angleDeg));
}

void detachServo() {
  if (attached) {
    ledcDetachPin(PIN_SERVO);
    attached = false;
  }
  pinMode(PIN_SERVO, OUTPUT);
  digitalWrite(PIN_SERVO, LOW);
  currentAngleDeg = -1.0f;
  Console.println("[BRAS] Servo coupe : bras libre.");
}

// --- Ultrasons (mode défense) ---

// La plus courte distance vue par un ultrason qui marche. Comme en match (frontDistanceMm), mais un
// capteur débranché est ignoré : on peut tester avec un seul ultrason.
// Renvoie false si aucun ultrason ne mesure.
bool nearestDistanceMm(uint16_t& distanceMm) {
  bool found = false;
  distanceMm = US_NO_OBSTACLE_MM;
  const UltrasoundId ids[] = {UltrasoundId::LEFT, UltrasoundId::RIGHT};
  for (const UltrasoundId id : ids) {
    if (!sensors::isFaulty(id) && sensors::distanceMm(id) < distanceMm) {
      distanceMm = sensors::distanceMm(id);
    }
    found = found || !sensors::isFaulty(id);
  }
  return found;
}

// --- Les modes ---

void startAttack(uint32_t nowMs) {
  attack = ArmAttack();  // séquence neuve
  attack.update(true, nowMs);  // « adversaire à portée » : l'attaque commence tout de suite
  Console.printf("[BRAS] Attaque : %.0f deg pendant %.1f s, puis retour\n", ARM_ATTACK_DEG,
                 ARM_ATTACK_HOLD_MS / 1000.0f);
  mode = Mode::ATTACK;
}

void startDefend(uint32_t nowMs) {
  uint16_t distanceMm = 0;
  if (!nearestDistanceMm(distanceMm)) {
    Console.println("[BRAS] Aucun ultrason ne mesure : mode defense impossible. Verifiez-les (test_ultrason).");
    return;
  }
  attack = ArmAttack();
  moveTo(attack.angleDeg());  // repos
  lastLogMs = nowMs;
  Console.printf("[BRAS] MODE DEFENSE : attaque si un objet est a moins de %u mm. Une touche = arret.\n",
                 ARM_TRIGGER_DISTANCE_MM);
  mode = Mode::DEFEND;
}

void stopMode(const char* why) {
  mode = Mode::IDLE;
  attack = ArmAttack();
  moveTo(ARM_REST_DEG);
  Console.printf("[BRAS] %s : bras au repos.\n", why);
}

void updateAttack(uint32_t nowMs) {
  const bool changed = attack.update(false, nowMs);
  moveTo(attack.angleDeg());
  if (changed && attack.phase() == ArmPhase::RETURNING) {
    Console.println("[BRAS] Fin de la tenue : retour");
  }
  if (changed && attack.phase() == ArmPhase::READY) {
    Console.println("[BRAS] Attaque terminee.");
    mode = Mode::IDLE;
  }
}

void updateDefend(uint32_t nowMs) {
  uint16_t distanceMm = 0;
  if (!nearestDistanceMm(distanceMm)) {
    stopMode("Plus aucun ultrason ne mesure");
    return;
  }
  const bool adversaryNear = distanceMm < ARM_TRIGGER_DISTANCE_MM;
  const bool changed = attack.update(adversaryNear, nowMs);
  moveTo(attack.angleDeg());
  if (changed) {
    switch (attack.phase()) {
      case ArmPhase::ATTACKING:
        Console.printf("[BRAS] Objet a %u mm : attaque %u\n", distanceMm, attack.attackCount());
        break;
      case ArmPhase::RETURNING:
        Console.println("[BRAS] Fin de la tenue : retour");
        break;
      case ArmPhase::READY:
        Console.println("[BRAS] Pret pour une nouvelle attaque");
        break;
    }
  }
  if (nowMs - lastLogMs >= DEFEND_LOG_PERIOD_MS) {
    lastLogMs = nowMs;
    Console.printf("[BRAS] US G %4u mm (%s) | D %4u mm (%s) | plus proche %4u mm | %u attaque(s)\n",
                   sensors::distanceMm(UltrasoundId::LEFT), sensors::isFaulty(UltrasoundId::LEFT) ? "panne" : "ok",
                   sensors::distanceMm(UltrasoundId::RIGHT), sensors::isFaulty(UltrasoundId::RIGHT) ? "panne" : "ok",
                   distanceMm, attack.attackCount());
  }
}

// --- Lecture de la ligne tapée ---

char* skipSpaces(char* text) {
  while (*text == ' ') {
    text++;
  }
  return text;
}

void executeLine(uint32_t nowMs) {
  line[lineLength] = '\0';
  char* text = skipSpaces(line);
  if (*text == '\0') {
    return;
  }
  const char command = (char)tolower(*text);
  char* arguments = skipSpaces(text + 1);
  switch (command) {
    case 'r':
      moveTo(ARM_REST_DEG);
      break;
    case 'a':
      startAttack(nowMs);
      break;
    case 'x': {
      char* end = nullptr;
      const float angleDeg = strtof(arguments, &end);
      if (end == arguments || *skipSpaces(end) != '\0' || angleDeg < 0.0f || angleDeg > 180.0f) {
        Console.println("[BRAS] Tapez un angle de 0 a 180, par exemple : x 45");
        return;
      }
      moveTo(angleDeg);
      break;
    }
    case 'u':
      startDefend(nowMs);
      break;
    case 'c':
      detachServo();
      break;
    case 'i':
      printSettings();
      break;
    case 'h':
    case '?':
      printHelp();
      break;
    default:
      Console.println("[BRAS] Commande inconnue.");
      printHelp();
      break;
  }
}

void handleChar(char c, uint32_t nowMs) {
  // Beaucoup de moniteurs envoient \r puis \n pour une seule touche Entrée : on n'en compte qu'une.
  if (c == '\n' && lastWasCr) {
    lastWasCr = false;
    return;
  }
  lastWasCr = (c == '\r');

  // Pendant une attaque ou le mode défense, n'importe quelle touche arrête et ramène le bras au repos.
  if (mode != Mode::IDLE) {
    stopMode("Arret (touche)");
    lineLength = 0;
    return;
  }
  if (c == '\r' || c == '\n') {
    Console.println();
    executeLine(nowMs);
    lineLength = 0;
    return;
  }
  if (c == '\b' || c == 127) {  // effacer le dernier caractère
    if (lineLength > 0) {
      lineLength--;
      Console.print("\b \b");
    }
    return;
  }
  if (c < 32 || c > 126 || lineLength >= sizeof(line) - 1) {
    return;
  }
  line[lineLength++] = c;
  Console.print(c);  // écho : le moniteur n'affiche pas ce qu'on tape
}

}  // namespace

void setup() {
  // En tout premier : moteurs freinés. Ce banc ne les fait jamais tourner.
  motorDriver::begin();
  // Pas de PWM sur le servo avant la première commande : il ne bouge pas à l'allumage.
  pinMode(PIN_SERVO, OUTPUT);
  digitalWrite(PIN_SERVO, LOW);

  Serial.begin(SERIAL_BAUD);
  Console.println();
  Console.println("[BANC] ===== BANC DE TEST DU BRAS (servo d'attaque) =====");
  Console.println("[BANC] Roues freinees. Apres le test : remettre le firmware (pio run -t upload, ou -e esp32dev_wifi).");
  printResetReason();

  sensors::begin();
  printSettings();
  printHelp();
  wireless::begin();
}

void loop() {
  const uint32_t nowMs = millis();

  while (Console.available() > 0) {
    handleChar((char)Console.read(), nowMs);
  }

  sensors::update(nowMs);
  switch (mode) {
    case Mode::ATTACK:
      updateAttack(nowMs);
      break;
    case Mode::DEFEND:
      updateDefend(nowMs);
      break;
    case Mode::IDLE:
      break;
  }

  // Téléversement sans fil accepté seulement bras au repos, hors attaque et hors mode défense.
  wireless::update(nowMs, mode == Mode::IDLE);
}

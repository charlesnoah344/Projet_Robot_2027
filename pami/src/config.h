// config.h — toutes les constantes du PAMI : broches, durées, seuils, vitesses.
//
// Règles :
// - l'unité est dans le nom (_MS, _MM, _PCT, _HZ...) ;
// - les broches sont recopiées de docs/materiel.md, qui reste la source de vérité ;
// - une valeur calibrée porte en commentaire le numéro du test qui l'a mesurée (// Txxx) ;
// - ce fichier ne dépend pas d'Arduino, car il sert aussi aux tests sur PC.
#pragma once

#include <stdint.h>

// ---------------------------------------------------------------------------
// Journal série
// ---------------------------------------------------------------------------
constexpr uint32_t SERIAL_BAUD = 115200;  // doit être égal à monitor_speed (platformio.ini)
// false : plus aucun log pendant le fonctionnement (seul le message de démarrage reste).
constexpr bool DEBUG_LOGS = true;
// Un message d'état par seconde : assez pour suivre, sans saturer le port série.
constexpr uint32_t STATUS_LOG_PERIOD_MS = 1000;

// ---------------------------------------------------------------------------
// Broches (copie de docs/materiel.md)
// ---------------------------------------------------------------------------
// Driver moteurs MDD3A : 2 entrées par moteur (D006).
constexpr uint8_t PIN_MOTOR_LEFT_A = 25;   // M1A
constexpr uint8_t PIN_MOTOR_LEFT_B = 26;   // M1B
constexpr uint8_t PIN_MOTOR_RIGHT_A = 33;  // M2A
constexpr uint8_t PIN_MOTOR_RIGHT_B = 32;  // M2B

// Encodeurs à effet Hall (étape B). Pont diviseur si leurs signaux sont en 5 V.
constexpr uint8_t PIN_ENCODER_LEFT_A = 27;
constexpr uint8_t PIN_ENCODER_LEFT_B = 14;
constexpr uint8_t PIN_ENCODER_RIGHT_A = 13;
constexpr uint8_t PIN_ENCODER_RIGHT_B = 4;

// Ultrason HC-SR04 (étape C). ECHO passe par un pont diviseur 5 V -> 3,3 V.
constexpr uint8_t PIN_US_TRIG = 18;
constexpr uint8_t PIN_US_ECHO = 19;

// Capteurs IR (étape F). Sortie à l'état bas = obstacle.
constexpr uint8_t PIN_IR_FRONT_LEFT = 34;
constexpr uint8_t PIN_IR_FRONT_RIGHT = 35;

// Départ : bouton-poussoir externe, l'autre patte à GND. Appui = état bas (D007).
// Le bouton BOOT de la carte n'est pas accessible sur le PAMI monté.
// Plus tard, la tirette prendra cette même broche : tirette en place = état bas, retirée = état haut.
constexpr uint8_t PIN_START_BUTTON = 16;

// Sélecteur de couleur : interrupteur, l'autre patte à GND. Ouvert = bleu, fermé = jaune.
constexpr uint8_t PIN_COLOR_SELECTOR = 17;

// Servo SG90 du bras du défenseur (bibliothèque ESP32Servo).
constexpr uint8_t PIN_SERVO = 23;

// Pas encore utilisées.
constexpr uint8_t PIN_EMERGENCY_STOP_SENSE = 39;  // lecture de l'état du BAU (optionnel)
constexpr uint8_t PIN_I2C_SDA = 21;               // gyroscope (optionnel)
constexpr uint8_t PIN_I2C_SCL = 22;

// ---------------------------------------------------------------------------
// Moteurs (driver MDD3A)
// ---------------------------------------------------------------------------
// 20 kHz : au-dessus de ce que l'oreille entend (les moteurs ne sifflent pas),
// et c'est le maximum accepté par le MDD3A.
constexpr uint32_t MOTOR_PWM_FREQ_HZ = 20000;
// 10 bits = 1024 niveaux de vitesse. À 20 kHz, l'ESP32 permet au plus 11 bits.
constexpr uint8_t MOTOR_PWM_RESOLUTION_BITS = 10;
// Canaux du générateur PWM de l'ESP32 (LEDC), un par entrée du driver.
constexpr uint8_t LEDC_CHANNEL_MOTOR_LEFT_A = 0;
constexpr uint8_t LEDC_CHANNEL_MOTOR_LEFT_B = 1;
constexpr uint8_t LEDC_CHANNEL_MOTOR_RIGHT_A = 2;
constexpr uint8_t LEDC_CHANNEL_MOTOR_RIGHT_B = 3;
// true si la roue tourne à l'envers quand on lui demande d'avancer.
// À mesurer à l'étape 2 (T002). On corrige ici plutôt que de recâbler.
constexpr bool MOTOR_LEFT_INVERTED = false;
constexpr bool MOTOR_RIGHT_INVERTED = false;
// PWM maximale autorisée, appliquée par motor_driver, même dans les bancs de test.
// Alimentation de labo à 6 V = tension nominale des FIT0450 : 100 %.
// À revoir avec la batterie : avec 8,4 V, il faudra environ 70 % pour rester vers 6 V en moyenne.
constexpr int MOTOR_MAX_PWM_PCT = 100;

// ---------------------------------------------------------------------------
// Encodeurs à effet Hall des FIT0450 (D004)
// ---------------------------------------------------------------------------
// 960 impulsions par tour de roue et par voie (fiche DFRobot). On compte les 2 fronts de la voie A :
// 1920 ticks par tour. PROVISOIRE : à vérifier en tournant une roue 10 tours à la main.
constexpr int32_t ENCODER_TICKS_PER_WHEEL_TURN = 1920;
// 60 mm : diamètre donné par l'équipe (09/10/2026).
// À affiner avec la ligne droite de 1 m et le test du carré (étapes D et E) : quelques dixièmes
// de mm d'écart (pneu écrasé par le poids du PAMI) donnent déjà plusieurs mm d'erreur par mètre.
constexpr float WHEEL_DIAMETER_MM = 60.0f;
// Distance parcourue par la roue pour un tick : périmètre de la roue / ticks par tour.
constexpr float MM_PER_TICK = 3.14159265f * WHEEL_DIAMETER_MM / ENCODER_TICKS_PER_WHEEL_TURN;
// true si les voies A et B ont besoin du pull-up interne : à la mesure du guide (§ 4), le signal
// reste à 0 V (sortie « collecteur ouvert »). false si elles passent par un pont diviseur.
// PROVISOIRE : à régler après la mesure, AVANT de brancher les encodeurs sur l'ESP32.
constexpr bool ENCODER_USE_INTERNAL_PULLUP = false;
// true si le compteur diminue quand on pousse le PAMI en avant à la main.
// PROVISOIRE : à mesurer à l'étape B. On corrige ici plutôt que de recâbler.
constexpr bool ENCODER_LEFT_INVERTED = false;
constexpr bool ENCODER_RIGHT_INVERTED = false;

// ---------------------------------------------------------------------------
// Mouvement (motion) : régulation de vitesse et trajectoires
// ---------------------------------------------------------------------------
// Toutes ces valeurs sont PROVISOIRES : à régler aux étapes D et E, sur le vrai PAMI.
// Écart entre les points de contact des deux roues. Sert à mesurer les angles :
// angle (rad) = (distance roue droite - distance roue gauche) / WHEEL_BASE_MM.
// 100 mm : entraxe donné par l'équipe (09/10/2026). À affiner avec la mission ROTATION et le carré.
constexpr float WHEEL_BASE_MM = 100.0f;
// La régulation tourne toutes les 20 ms : assez souvent pour corriger, assez rarement pour que
// chaque mesure de vitesse compte plusieurs ticks.
constexpr uint32_t MOTION_PERIOD_MS = 20;
// Vitesse de croisière en ligne droite. Prudente au début : on l'augmentera après les tests.
constexpr float DRIVE_SPEED_MM_S = 200.0f;
// Vitesse de chaque roue pendant une rotation sur place.
constexpr float TURN_SPEED_MM_S = 120.0f;
// Accélération et freinage : progressifs, pour ne pas patiner ni créer de pic de courant.
constexpr float MOTION_ACCEL_MM_S2 = 400.0f;
// En dessous de cette vitesse, les moteurs risquent de caler (zone morte) : on ne descend pas plus bas.
constexpr float MOTION_MIN_SPEED_MM_S = 40.0f;
// On considère la cible atteinte à moins de 3 mm (ligne droite) ou 2 mm d'arc de roue (rotation).
constexpr float DRIVE_TOLERANCE_MM = 3.0f;
constexpr float TURN_TOLERANCE_MM = 2.0f;

// Régulateur de vitesse de chaque roue : PWM = anticipation + correction proportionnelle + intégrale.
// Anticipation : PWM nécessaire pour décoller (zone morte) + PWM par mm/s voulu.
constexpr float MOTOR_DEADBAND_PCT = 10.0f;     // PROVISOIRE : zone morte à mesurer (T002 incomplet)
constexpr float MOTOR_PCT_PER_MM_S = 0.2f;      // PROVISOIRE : environ 100 % pour 500 mm/s
constexpr float SPEED_KP_PCT_PER_MM_S = 0.05f;  // 100 mm/s d'écart -> 5 % de PWM en plus
constexpr float SPEED_KI_PCT_PER_MM = 0.3f;     // corrige l'écart qui dure (pente, frottements)
constexpr float SPEED_INTEGRAL_MAX_PCT = 30.0f;  // l'intégrale ne peut pas « s'emballer »
// Ligne droite : si la roue gauche a parcouru 1 mm de plus que la droite, on la ralentit de
// 5 mm/s et on accélère la droite d'autant. Même gain pour garder la rotation bien sur place.
constexpr float HEADING_KP_MM_S_PER_MM = 5.0f;

// Roue bloquée ou encodeur inversé : une roue qui reçoit au moins 35 % de PWM mais qui ne tourne pas
// (ou tourne à l'envers) pendant 0,6 s. Le PAMI passe en FAULT et s'arrête.
// Sans ça, un encodeur inversé ferait accélérer le régulateur jusqu'à la pleine vitesse.
constexpr float STALL_MIN_PCT = 35.0f;
constexpr float STALL_MAX_SPEED_MM_S = 15.0f;
constexpr uint32_t STALL_TIME_MS = 600;

// ---------------------------------------------------------------------------
// Missions (logic/missions.h)
// ---------------------------------------------------------------------------
// Une mission par démonstration de la grille du labo.
enum class MissionId {
  STRAIGHT_LINE,  // ligne droite de STRAIGHT_LINE_LENGTH_MM
  ROTATION,       // un tour complet sur place, par quarts de tour
  SQUARE,         // carré de SQUARE_SIDE_MM de côté, retour au départ
  DEFENDER        // va dans la zone, puis défend avec le bras
};
// La mission jouée. Pour en changer : modifier cette ligne, puis recompiler et téléverser.
// Ordre conseillé pour les tests : STRAIGHT_LINE, ROTATION, SQUARE, puis DEFENDER.
constexpr MissionId ACTIVE_MISSION = MissionId::STRAIGHT_LINE;

// Il n'y a que 9,5 s de mouvement (de 5 s à 14,5 s) : chaque mission doit tenir dedans,
// avec de la marge pour un détour. PROVISOIRE : à ajuster d'après les vitesses réelles.
constexpr float STRAIGHT_LINE_LENGTH_MM = 1000.0f;
constexpr float SQUARE_SIDE_MM = 250.0f;
// Zone du défenseur, dans le repère de départ (x devant, y à gauche), pour le bleu.
// PROVISOIRE : à fixer quand on connaîtra la table et la zone de l'évaluation.
constexpr float DEFENDER_ZONE_X_MM = 700.0f;
constexpr float DEFENDER_ZONE_Y_MM = 300.0f;
// Cap à prendre une fois dans la zone, pour que l'ultrason et le bras regardent vers l'adversaire.
constexpr float DEFENDER_FACING_DEG = 0.0f;

// Point atteint à moins de 30 mm. Cap atteint à moins de 3°.
// Si le PAMI est mal orienté, il se retourne vers le point avant d'avancer.
constexpr float GOTO_REACHED_TOLERANCE_MM = 30.0f;
constexpr float HEADING_TOLERANCE_DEG = 3.0f;

// ---------------------------------------------------------------------------
// Ultrason HC-SR04 et sécurité
// ---------------------------------------------------------------------------
// Au moins 60 ms entre deux mesures : l'écho de la mesure précédente doit avoir disparu.
constexpr uint32_t US_PERIOD_MS = 60;
// Impulsion de déclenchement demandée par la fiche du HC-SR04.
constexpr uint32_t US_TRIGGER_PULSE_US = 10;
// Écho plus long que 12 000 µs (environ 2 m) : « rien dans la portée ». C'est normal, pas une panne.
constexpr uint32_t US_MAX_ECHO_US = 12000;
// Distance renvoyée quand il n'y a rien dans la portée.
constexpr uint16_t US_NO_OBSTACLE_MM = 2000;
// Le son fait 343 m/s, soit 0,343 mm/µs. Il fait l'aller-retour : 0,1715 mm par µs d'écho.
constexpr float US_MM_PER_ECHO_US = 0.1715f;
// Aucune mesure valide pendant 1 s : le capteur est considéré en panne (fil débranché, ECHO bloqué).
// Sécurité par défaut : panne = obstacle, le PAMI s'arrête.
constexpr uint32_t SENSOR_FAULT_TIMEOUT_MS = 1000;

// Obstacle plus proche que ce seuil devant : safety freine, quel que soit l'état.
// PROVISOIRE, volontairement large : à régler d'après la distance de freinage mesurée (étape C).
constexpr uint16_t OBSTACLE_STOP_DISTANCE_MM = 150;
// L'obstacle n'est considéré parti qu'au-delà de seuil + 30 mm.
// Sans cette marge, une mesure qui oscille autour du seuil ferait avancer et freiner en boucle.
constexpr uint16_t OBSTACLE_CLEAR_MARGIN_MM = 30;

// Capteurs IR (sortie tout-ou-rien, état bas = obstacle) : lus toutes les 10 ms.
// Obstacle si au moins 2 des 3 dernières lectures le disent : un parasite isolé est ignoré.
constexpr uint32_t IR_PERIOD_MS = 10;

// ---------------------------------------------------------------------------
// Évitement (états AVOID_STOP et DETOUR)
// ---------------------------------------------------------------------------
// Niveau 1, arrêt : safety freine dès qu'un obstacle est devant (ultrason ou IR).
// Niveau 2, évite : obstacle toujours là après 1 s -> détour. Ce délai laisse à un robot qui passe
// le temps de s'en aller.
constexpr uint32_t AVOID_WAIT_MS = 1000;
// Le détour : quart de tour du côté libre, puis pas de côté.
constexpr float DETOUR_TURN_DEG = 90.0f;
constexpr float DETOUR_SIDE_MM = 200.0f;  // PROVISOIRE : un peu plus que la largeur d'un PAMI
// Niveau 3, contourne : après le détour, la mission vise de nouveau son point.
// Au-delà de 3 détours, on ne fait plus qu'attendre que la voie se libère (le temps est compté).
constexpr uint8_t AVOID_MAX_DETOURS = 3;

// ---------------------------------------------------------------------------
// Bras du défenseur (servo SG90, D008)
// ---------------------------------------------------------------------------
// Servo piloté directement par le générateur PWM de l'ESP32 (LEDC), sans bibliothèque :
// ESP32Servo choisirait elle-même un canal et pourrait prendre celui d'un moteur (canaux 0 à 3).
// Canal 4 : il utilise un autre minuteur (timer 2) que les moteurs (timers 0 et 1),
// donc ses 50 Hz ne changent pas les 20 kHz des moteurs.
constexpr uint8_t LEDC_CHANNEL_SERVO = 4;
constexpr uint32_t SERVO_PWM_FREQ_HZ = 50;         // un servo attend une impulsion toutes les 20 ms
constexpr uint8_t SERVO_PWM_RESOLUTION_BITS = 16;  // réglage fin de la largeur d'impulsion
// Largeur d'impulsion pour 0° et pour 180° (fiche du SG90, à vérifier au test).
constexpr uint32_t SERVO_MIN_PULSE_US = 500;
constexpr uint32_t SERVO_MAX_PULSE_US = 2400;
// Positions du bras. PROVISOIRES : à régler quand le bras sera monté.
constexpr float ARM_RETRACTED_DEG = 0.0f;  // replié, dans le périmètre du PAMI
constexpr float ARM_DEPLOYED_DEG = 90.0f;  // déployé vers l'avant
// Le bras se déploie quand l'ultrason voit l'adversaire à moins de 60 mm.
// Le bras ne peut dépasser que d'environ 50 mm (grille : périmètre déployé + 10 cm au plus).
// PROVISOIRE : à régler d'après la longueur réelle du bras.
constexpr uint16_t ARM_TRIGGER_DISTANCE_MM = 60;

// ---------------------------------------------------------------------------
// Sélecteur de couleur
// ---------------------------------------------------------------------------
// Lu toutes les 100 ms avant le départ : un interrupteur ne change pas plus vite.
constexpr uint32_t COLOR_READ_PERIOD_MS = 100;

// ---------------------------------------------------------------------------
// Minuteur de match
// ---------------------------------------------------------------------------
// Profil labo (D007, grille du Mechatronics Lab) : départ après 5 s, arrêt après 15 s.
// Les deux durées comptent depuis l'appui sur le bouton de départ.
// Valeurs Eurobot, pour plus tard (règle d'or n° 3, règles 2027 E.4) :
//   MATCH_MOVE_ALLOWED_MS = 85300 et MATCH_STOP_MS = 99500.
// Immobile jusqu'à 5 s après le départ.
constexpr uint32_t MATCH_MOVE_ALLOWED_MS = 5000;
// Arrêt total à 14,5 s : 0,5 s de marge avant les 15 s, comme les 99,5 s d'Eurobot.
constexpr uint32_t MATCH_STOP_MS = 14500;
// Un appui ou un relâchement doit durer au moins 50 ms pour compter :
// les contacts d'un bouton rebondissent pendant quelques millisecondes.
constexpr uint32_t START_BUTTON_DEBOUNCE_MS = 50;

// ---------------------------------------------------------------------------
// Bancs de test (test_bench/) : jamais utilisés en match
// ---------------------------------------------------------------------------
// Vitesse de départ prudente : la roue tourne lentement, ou pas du tout (zone morte).
constexpr int TEST_BENCH_START_PWM_PCT = 20;
constexpr int TEST_BENCH_PWM_STEP_PCT = 5;       // touches + et -
constexpr int TEST_BENCH_PWM_FINE_STEP_PCT = 1;  // touches > et < : zone morte au pourcent près
// Sécurité : les moteurs s'arrêtent seuls si personne n'a tapé de commande depuis 10 s.
constexpr uint32_t TEST_BENCH_AUTO_STOP_MS = 10000;

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

// Encodeurs (étape 3). Pont diviseur si leurs signaux sont en 5 V.
constexpr uint8_t PIN_ENCODER_LEFT_A = 27;
constexpr uint8_t PIN_ENCODER_LEFT_B = 14;
constexpr uint8_t PIN_ENCODER_RIGHT_A = 13;
constexpr uint8_t PIN_ENCODER_RIGHT_B = 4;

// Ultrason HC-SR04 (étape 5). ECHO passe par un pont diviseur 5 V -> 3,3 V.
constexpr uint8_t PIN_US_TRIG = 18;
constexpr uint8_t PIN_US_ECHO = 19;

// Capteurs IR (étape 6). Sortie à l'état bas = obstacle.
constexpr uint8_t PIN_IR_FRONT_LEFT = 34;
constexpr uint8_t PIN_IR_FRONT_RIGHT = 35;

// Départ provisoire : bouton BOOT de la carte, appui = état bas (D005).
// Remplacé par la tirette avant tout match.
constexpr uint8_t PIN_START_BUTTON = 0;

// Pas encore utilisées.
constexpr uint8_t PIN_SERVO = 23;
constexpr uint8_t PIN_PULL_CORD = 16;             // tirette
constexpr uint8_t PIN_COLOR_SELECTOR = 17;
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

// ---------------------------------------------------------------------------
// Minuteur de match
// ---------------------------------------------------------------------------
// Arrêt total 99,5 s après le départ (règle d'or n° 3) : 0,5 s de marge avant la fin des 100 s.
constexpr uint32_t MATCH_STOP_MS = 99500;
// Immobilité jusqu'à 85,3 s après la tirette (règles 2027, E.4).
// PAS ENCORE UTILISÉE : exception temporaire D005. À IMPLÉMENTER AVANT TOUT MATCH.
constexpr uint32_t MATCH_MOVE_ALLOWED_MS = 85300;
// Un appui ou un relâchement doit durer au moins 50 ms pour compter :
// les contacts d'un bouton rebondissent pendant quelques millisecondes.
constexpr uint32_t START_BUTTON_DEBOUNCE_MS = 50;

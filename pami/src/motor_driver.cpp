#include "motor_driver.h"

#include <Arduino.h>

#include "config.h"

namespace motorDriver {

namespace {

constexpr uint8_t PIN_COUNT = 4;
constexpr uint8_t pins[PIN_COUNT] = {PIN_MOTOR_LEFT_A, PIN_MOTOR_LEFT_B, PIN_MOTOR_RIGHT_A,
                                     PIN_MOTOR_RIGHT_B};
constexpr uint8_t channels[PIN_COUNT] = {LEDC_CHANNEL_MOTOR_LEFT_A, LEDC_CHANNEL_MOTOR_LEFT_B,
                                         LEDC_CHANNEL_MOTOR_RIGHT_A, LEDC_CHANNEL_MOTOR_RIGHT_B};

// Dernier rapport cyclique envoyé à chaque canal. On n'écrit que ce qui change :
// safety envoie une commande à chaque tour de loop(), et une écriture inutile prend du temps.
uint32_t lastDuty[PIN_COUNT] = {0, 0, 0, 0};

void writeDuty(uint8_t index, uint32_t duty) {
  if (duty != lastDuty[index]) {
    ledcWrite(channels[index], duty);
    lastDuty[index] = duty;
  }
}

// Rapport cyclique maximal : 1023 pour 10 bits.
constexpr uint32_t MAX_DUTY = (1UL << MOTOR_PWM_RESOLUTION_BITS) - 1;

// Pilote un moteur à travers ses deux entrées A et B.
// pct > 0 : avance (A = PWM, B = 0). pct < 0 : recule (A = 0, B = PWM). pct = 0 : frein.
void setMotor(uint8_t indexA, uint8_t indexB, float pct, bool inverted) {
  if (isnan(pct)) {
    pct = 0.0f;  // valeur invalide : on freine
  }
  if (inverted) {
    pct = -pct;
  }
  // Limite : protège les moteurs, même quand un banc de test demande plus.
  if (pct > MOTOR_MAX_PWM_PCT) {
    pct = MOTOR_MAX_PWM_PCT;
  }
  if (pct < -MOTOR_MAX_PWM_PCT) {
    pct = -MOTOR_MAX_PWM_PCT;
  }
  const uint32_t duty = (uint32_t)(fabsf(pct) / 100.0f * MAX_DUTY + 0.5f);

  // On coupe toujours l'entrée inactive AVANT d'alimenter l'autre.
  if (pct > 0.0f) {
    writeDuty(indexB, 0);
    writeDuty(indexA, duty);
  } else if (pct < 0.0f) {
    writeDuty(indexA, 0);
    writeDuty(indexB, duty);
  } else {
    writeDuty(indexA, 0);
    writeDuty(indexB, 0);
  }
}

}  // namespace

void begin() {
  // Avant toute chose : les 4 entrées à l'état bas. Pour le MDD3A, A = B = 0 veut dire « frein ».
  for (uint8_t i = 0; i < PIN_COUNT; i++) {
    pinMode(pins[i], OUTPUT);
    digitalWrite(pins[i], LOW);
  }
  // Ensuite seulement, on confie les broches au générateur PWM de l'ESP32 (API LEDC du core 2.x).
  // Rapport cyclique 0 : la broche reste à l'état bas.
  for (uint8_t i = 0; i < PIN_COUNT; i++) {
    ledcSetup(channels[i], MOTOR_PWM_FREQ_HZ, MOTOR_PWM_RESOLUTION_BITS);
    ledcAttachPin(pins[i], channels[i]);
    ledcWrite(channels[i], 0);
    lastDuty[i] = 0;
  }
}

void brake() {
  for (uint8_t i = 0; i < PIN_COUNT; i++) {
    writeDuty(i, 0);
  }
}

void setWheelPercent(float leftPct, float rightPct) {
  // Indices dans pins[] : 0 et 1 = moteur gauche (M1A, M1B), 2 et 3 = moteur droit (M2A, M2B).
  setMotor(0, 1, leftPct, MOTOR_LEFT_INVERTED);
  setMotor(2, 3, rightPct, MOTOR_RIGHT_INVERTED);
}

}  // namespace motorDriver

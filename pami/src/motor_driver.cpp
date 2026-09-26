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

}  // namespace motorDriver

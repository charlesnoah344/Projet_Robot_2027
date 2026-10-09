#include "encoders.h"

#include <Arduino.h>

#include "config.h"

namespace encoders {

namespace {

// volatile : ces compteurs changent dans les interruptions, le compilateur doit les relire à chaque fois.
volatile int32_t leftCount = 0;
volatile int32_t rightCount = 0;

// Verrou partagé entre les interruptions et read().
portMUX_TYPE countersMux = portMUX_INITIALIZER_UNLOCKED;

// Quadrature : juste après un front de A, si A et B sont égaux, la roue tourne dans un sens ;
// s'ils sont différents, dans l'autre. Le sens « avant » se règle avec ENCODER_*_INVERTED.
// IRAM_ATTR : la fonction est placée en RAM rapide, comme l'exige l'ESP32 pour une interruption.
void IRAM_ATTR onLeftA() {
  const bool sameLevel = (digitalRead(PIN_ENCODER_LEFT_A) == digitalRead(PIN_ENCODER_LEFT_B));
  const int32_t step = (sameLevel != ENCODER_LEFT_INVERTED) ? 1 : -1;
  portENTER_CRITICAL_ISR(&countersMux);
  leftCount += step;
  portEXIT_CRITICAL_ISR(&countersMux);
}

void IRAM_ATTR onRightA() {
  const bool sameLevel = (digitalRead(PIN_ENCODER_RIGHT_A) == digitalRead(PIN_ENCODER_RIGHT_B));
  const int32_t step = (sameLevel != ENCODER_RIGHT_INVERTED) ? 1 : -1;
  portENTER_CRITICAL_ISR(&countersMux);
  rightCount += step;
  portEXIT_CRITICAL_ISR(&countersMux);
}

}  // namespace

void begin() {
  // Pull-up seulement pour une sortie « collecteur ouvert » (mesure du guide pratique, § 4).
  const uint8_t mode = ENCODER_USE_INTERNAL_PULLUP ? INPUT_PULLUP : INPUT;
  pinMode(PIN_ENCODER_LEFT_A, mode);
  pinMode(PIN_ENCODER_LEFT_B, mode);
  pinMode(PIN_ENCODER_RIGHT_A, mode);
  pinMode(PIN_ENCODER_RIGHT_B, mode);

  // CHANGE : fronts montants ET descendants de A, soit 2 ticks par impulsion.
  attachInterrupt(digitalPinToInterrupt(PIN_ENCODER_LEFT_A), onLeftA, CHANGE);
  attachInterrupt(digitalPinToInterrupt(PIN_ENCODER_RIGHT_A), onRightA, CHANGE);
}

void read(int32_t& leftTicks, int32_t& rightTicks) {
  portENTER_CRITICAL(&countersMux);
  leftTicks = leftCount;
  rightTicks = rightCount;
  portEXIT_CRITICAL(&countersMux);
}

}  // namespace encoders

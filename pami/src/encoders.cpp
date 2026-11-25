#include "encoders.h"

#include <Arduino.h>
#include <driver/gpio.h>
#include <driver/pcnt.h>
#include <esp_timer.h>

#include "config.h"

namespace encoders {

namespace {

// Le filtre du PCNT se règle en périodes de l'horloge APB (80 MHz, soit 12,5 ns), sur 10 bits.
constexpr uint32_t APB_CLOCK_HZ = 80000000;
constexpr uint32_t FILTER_CYCLES = ENCODER_GLITCH_FILTER_NS * (APB_CLOCK_HZ / 1000000) / 1000;
static_assert(FILTER_CYCLES >= 1 && FILTER_CYCLES <= 1023,
              "ENCODER_GLITCH_FILTER_NS : entre 13 et 12 787 ns (filtre du PCNT sur 10 bits)");

// Tout ce qu'on sait d'une roue.
struct Wheel {
  explicit Wheel(pcnt_unit_t pcntUnit) : unit(pcntUnit) {}

  const pcnt_unit_t unit;  // compteur matériel utilisé (l'ESP32 en a 8)
  int16_t lastRaw = 0;     // dernière valeur lue dans le compteur matériel
  int32_t total = 0;       // ticks depuis l'allumage (32 bits : ne déborde jamais pendant un match)
};

Wheel left{PCNT_UNIT_0};
Wheel right{PCNT_UNIT_1};

// Verrou partagé entre le minuteur de ramassage et read().
portMUX_TYPE countersMux = portMUX_INITIALIZER_UNLOCKED;
esp_timer_handle_t collectTimer = nullptr;

// Configure un compteur matériel pour une roue.
// Décodage « x2 » : un tick à chaque front de A, montant ou descendant, comme avant (1920 ticks
// par tour de roue, les étalonnages restent valables). Le sens vient du niveau de B au moment du
// front de A, lu par le circuit lui-même, sans aucun retard :
//   front montant de A  + B haut -> +1 ;  front montant de A  + B bas  -> -1 ;
//   front descendant de A + B bas -> +1 ;  front descendant de A + B haut -> -1.
// « inverted » échange +1 et -1 (ENCODER_*_INVERTED).
void configureUnit(const Wheel& wheel, uint8_t pinA, uint8_t pinB, bool inverted) {
  pcnt_config_t config = {};
  config.pulse_gpio_num = pinA;              // on compte les fronts de A
  config.ctrl_gpio_num = pinB;               // B donne le sens
  config.channel = PCNT_CHANNEL_0;
  config.unit = wheel.unit;
  config.pos_mode = inverted ? PCNT_COUNT_DEC : PCNT_COUNT_INC;  // front montant de A
  config.neg_mode = inverted ? PCNT_COUNT_INC : PCNT_COUNT_DEC;  // front descendant de A
  config.hctrl_mode = PCNT_MODE_KEEP;        // B haut : on garde le sens ci-dessus
  config.lctrl_mode = PCNT_MODE_REVERSE;     // B bas : on l'inverse
  config.counter_h_lim = ENCODER_PCNT_LIMIT_TICKS;
  config.counter_l_lim = -ENCODER_PCNT_LIMIT_TICKS;
  pcnt_unit_config(&config);

  // Filtre anti-parasites : une impulsion plus courte que ENCODER_GLITCH_FILTER_NS est ignorée.
  pcnt_set_filter_value(wheel.unit, FILTER_CYCLES);
  pcnt_filter_enable(wheel.unit);

  // Résistances de tirage : réglées ici, après la configuration du compteur.
  const gpio_pull_mode_t pull = ENCODER_USE_INTERNAL_PULLUP ? GPIO_PULLUP_ONLY : GPIO_FLOATING;
  gpio_set_pull_mode(static_cast<gpio_num_t>(pinA), pull);
  gpio_set_pull_mode(static_cast<gpio_num_t>(pinB), pull);

  pcnt_counter_pause(wheel.unit);
  pcnt_counter_clear(wheel.unit);
  pcnt_counter_resume(wheel.unit);
}

// Le compteur matériel ne compte que de -ENCODER_PCNT_LIMIT_TICKS à +ENCODER_PCNT_LIMIT_TICKS :
// arrivé à la limite, il repart de 0. On ajoute donc régulièrement au total ce qu'il a compté
// depuis la dernière fois. Entre deux ramassages, une roue fait au plus quelques dizaines de ticks :
// un écart de plus d'une demi-plage veut forcément dire que le compteur est reparti de 0.
// À appeler avec le verrou pris.
void collect(Wheel& wheel) {
  int16_t raw = 0;
  pcnt_get_counter_value(wheel.unit, &raw);
  int32_t delta = static_cast<int32_t>(raw) - wheel.lastRaw;
  if (delta > ENCODER_PCNT_LIMIT_TICKS / 2) {
    delta -= ENCODER_PCNT_LIMIT_TICKS;  // reparti de 0 en reculant
  } else if (delta < -ENCODER_PCNT_LIMIT_TICKS / 2) {
    delta += ENCODER_PCNT_LIMIT_TICKS;  // reparti de 0 en avançant
  }
  wheel.lastRaw = raw;
  wheel.total += delta;
}

void collectBoth() {
  portENTER_CRITICAL(&countersMux);
  collect(left);
  collect(right);
  portEXIT_CRITICAL(&countersMux);
}

// Appelé toutes les ENCODER_COLLECT_PERIOD_MS par un minuteur de l'ESP32, même si personne
// n'appelle read() (par exemple pendant qu'on tourne une roue à la main).
void onCollectTimer(void*) {
  collectBoth();
}

}  // namespace

void begin() {
  configureUnit(left, PIN_ENCODER_LEFT_A, PIN_ENCODER_LEFT_B, ENCODER_LEFT_INVERTED);
  configureUnit(right, PIN_ENCODER_RIGHT_A, PIN_ENCODER_RIGHT_B, ENCODER_RIGHT_INVERTED);

  esp_timer_create_args_t timerArgs = {};
  timerArgs.callback = &onCollectTimer;
  timerArgs.name = "encoders";
  esp_timer_create(&timerArgs, &collectTimer);
  esp_timer_start_periodic(collectTimer, ENCODER_COLLECT_PERIOD_MS * 1000ULL);
}

void read(int32_t& leftTicks, int32_t& rightTicks) {
  // On ramasse d'abord, pour rendre la valeur la plus fraîche possible.
  portENTER_CRITICAL(&countersMux);
  collect(left);
  collect(right);
  leftTicks = left.total;
  rightTicks = right.total;
  portEXIT_CRITICAL(&countersMux);
}

}  // namespace encoders

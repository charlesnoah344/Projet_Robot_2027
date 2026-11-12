// La console du PAMI : remplace Serial pour tous les messages et toutes les touches (D010).
//
// - Chaque message part sur l'USB (Serial) ET dans le tampon du Wi-Fi (module wireless).
// - Les touches arrivent par l'USB ou par le moniteur Wi-Fi.
// S'utilise exactement comme Serial : Console.printf(...), Console.println(...), Console.read().
// Seul Serial.begin() reste sur Serial.
//
// Écrire dans la console ne bloque jamais loop() du côté Wi-Fi : c'est une simple copie en
// mémoire. C'est wireless::update() qui envoie ensuite, quand le réseau peut prendre les octets.
#pragma once

#include <Arduino.h>

#include "config.h"
#include "logic/byte_ring_buffer.h"

class PamiConsole : public Stream {
public:
  // Sortie : USB + tampon Wi-Fi.
  size_t write(uint8_t byte) override;
  size_t write(const uint8_t* buffer, size_t size) override;
  using Print::write;  // garde les autres formes de write() (texte, etc.)
  void flush() override;

  // Entrée : d'abord l'USB, puis le Wi-Fi.
  int available() override;
  int read() override;
  int peek() override;

  // Pour le module wireless seulement : messages à envoyer, touches reçues.
  ByteRingBuffer<WIFI_LOG_BUFFER_BYTES>& wifiOutput() { return wifiOutput_; }
  ByteRingBuffer<WIFI_KEY_BUFFER_BYTES>& wifiInput() { return wifiInput_; }

private:
  ByteRingBuffer<WIFI_LOG_BUFFER_BYTES> wifiOutput_;
  ByteRingBuffer<WIFI_KEY_BUFFER_BYTES> wifiInput_;
};

extern PamiConsole Console;

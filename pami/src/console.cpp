#include "console.h"

PamiConsole Console;

size_t PamiConsole::write(uint8_t byte) {
  return write(&byte, 1);
}

size_t PamiConsole::write(const uint8_t* buffer, size_t size) {
  Serial.write(buffer, size);      // USB : exactement comme avant
  wifiOutput_.push(buffer, size);  // Wi-Fi : copie en mémoire, envoyée par wireless::update()
  return size;
}

void PamiConsole::flush() {
  Serial.flush();
}

int PamiConsole::available() {
  return Serial.available() + (int)wifiInput_.size();
}

int PamiConsole::read() {
  if (Serial.available() > 0) {
    return Serial.read();
  }
  return wifiInput_.popByte();
}

int PamiConsole::peek() {
  if (Serial.available() > 0) {
    return Serial.peek();
  }
  return wifiInput_.peekByte();
}

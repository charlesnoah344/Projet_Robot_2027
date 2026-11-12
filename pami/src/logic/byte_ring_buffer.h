// Tampon circulaire d'octets (logique pure, testable sur PC).
//
// Sert à la console Wi-Fi (D010) : loop() y dépose les messages (une simple copie en mémoire,
// instantanée), et le module wireless les envoie au PC quand le réseau peut les prendre.
// Si le PC ne suit pas, le tampon se remplit : les nouveaux octets sont refusés et comptés.
// Ainsi loop() n'attend jamais le réseau.
//
// « Circulaire » : quand on arrive au bout du tableau, on repart au début. Les octets restent
// dans l'ordre d'arrivée, sans jamais rien décaler.
#pragma once

#include <stddef.h>
#include <stdint.h>

template <size_t CAPACITY>
class ByteRingBuffer {
public:
  // Ajoute jusqu'à `length` octets. Renvoie le nombre d'octets acceptés.
  // Ce qui ne rentre pas est perdu, et compté dans lostBytes().
  size_t push(const uint8_t* data, size_t length) {
    size_t accepted = 0;
    while (accepted < length && count_ < CAPACITY) {
      buffer_[(start_ + count_) % CAPACITY] = data[accepted];
      count_++;
      accepted++;
    }
    lostBytes_ += length - accepted;
    return accepted;
  }

  // Les plus anciens octets en attente qui se suivent dans le tableau (jusqu'au bout du tableau).
  // On peut les envoyer d'un seul coup, puis retirer ce qui est vraiment parti avec discard().
  size_t peekContiguous(const uint8_t*& data) const {
    data = &buffer_[start_];
    const size_t untilEnd = CAPACITY - start_;
    return (count_ < untilEnd) ? count_ : untilEnd;
  }

  // Retire les `length` plus anciens octets (ceux qui ont été envoyés).
  void discard(size_t length) {
    if (length > count_) {
      length = count_;
    }
    start_ = (start_ + length) % CAPACITY;
    count_ -= length;
  }

  // Lit un octet sans le retirer (-1 si le tampon est vide).
  int peekByte() const {
    return (count_ == 0) ? -1 : buffer_[start_];
  }

  // Retire et renvoie le plus ancien octet (-1 si le tampon est vide).
  int popByte() {
    const int byte = peekByte();
    if (byte >= 0) {
      discard(1);
    }
    return byte;
  }

  size_t size() const { return count_; }
  bool isEmpty() const { return count_ == 0; }
  size_t freeSpace() const { return CAPACITY - count_; }

  // Octets refusés parce que le tampon était plein, depuis le dernier resetLostBytes().
  uint32_t lostBytes() const { return lostBytes_; }
  void resetLostBytes() { lostBytes_ = 0; }

private:
  uint8_t buffer_[CAPACITY] = {};
  size_t start_ = 0;  // position du plus ancien octet
  size_t count_ = 0;  // nombre d'octets en attente
  uint32_t lostBytes_ = 0;
};

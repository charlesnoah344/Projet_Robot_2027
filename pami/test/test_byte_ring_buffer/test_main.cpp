// Tests du tampon circulaire de la console Wi-Fi (logic/byte_ring_buffer.h).
// Sur PC : pio test -e native. Sur l'ESP32 branché en USB : pio test -e logic_esp32.
#include <unity.h>

#include "logic/byte_ring_buffer.h"

void setUp() {}
void tearDown() {}

// Les octets ressortent dans l'ordre où ils sont entrés.
void test_bytes_come_out_in_order() {
  ByteRingBuffer<8> buffer;
  const uint8_t message[] = {'a', 'b', 'c'};
  TEST_ASSERT_EQUAL(3, buffer.push(message, 3));
  TEST_ASSERT_EQUAL(3, buffer.size());
  TEST_ASSERT_EQUAL('a', buffer.popByte());
  TEST_ASSERT_EQUAL('b', buffer.popByte());
  TEST_ASSERT_EQUAL('c', buffer.popByte());
  TEST_ASSERT_TRUE(buffer.isEmpty());
}

// Tampon vide : peekByte et popByte renvoient -1, sans rien casser.
void test_empty_buffer_returns_minus_one() {
  ByteRingBuffer<4> buffer;
  TEST_ASSERT_EQUAL(-1, buffer.peekByte());
  TEST_ASSERT_EQUAL(-1, buffer.popByte());
  TEST_ASSERT_EQUAL(0, buffer.size());
}

// Tampon plein : ce qui ne rentre pas est refusé et compté, ce qui était là est intact.
void test_full_buffer_refuses_and_counts_lost_bytes() {
  ByteRingBuffer<4> buffer;
  const uint8_t message[] = {1, 2, 3, 4, 5, 6};
  TEST_ASSERT_EQUAL(4, buffer.push(message, 6));
  TEST_ASSERT_EQUAL(2, buffer.lostBytes());
  TEST_ASSERT_EQUAL(0, buffer.freeSpace());
  TEST_ASSERT_EQUAL(1, buffer.popByte());
  TEST_ASSERT_EQUAL(2, buffer.popByte());
  TEST_ASSERT_EQUAL(3, buffer.popByte());
  TEST_ASSERT_EQUAL(4, buffer.popByte());
  buffer.resetLostBytes();
  TEST_ASSERT_EQUAL(0, buffer.lostBytes());
}

// Le tampon repart au début du tableau sans mélanger l'ordre des octets.
void test_wrap_around_keeps_order() {
  ByteRingBuffer<4> buffer;
  const uint8_t first[] = {'a', 'b', 'c'};
  buffer.push(first, 3);
  buffer.discard(2);  // 'a' et 'b' envoyés
  const uint8_t second[] = {'d', 'e', 'f'};
  TEST_ASSERT_EQUAL(3, buffer.push(second, 3));  // 'e' et 'f' repartent au début du tableau
  TEST_ASSERT_EQUAL('c', buffer.popByte());
  TEST_ASSERT_EQUAL('d', buffer.popByte());
  TEST_ASSERT_EQUAL('e', buffer.popByte());
  TEST_ASSERT_EQUAL('f', buffer.popByte());
}

// Envoi par morceaux : d'abord jusqu'au bout du tableau, puis la suite au début.
void test_peek_contiguous_then_discard_partial_send() {
  ByteRingBuffer<4> buffer;
  const uint8_t first[] = {'a', 'b', 'c'};
  buffer.push(first, 3);
  buffer.discard(2);
  const uint8_t second[] = {'d', 'e'};
  buffer.push(second, 2);  // dans le tableau : e . c d

  const uint8_t* data = nullptr;
  TEST_ASSERT_EQUAL(2, buffer.peekContiguous(data));  // c d (jusqu'au bout du tableau)
  TEST_ASSERT_EQUAL('c', data[0]);
  TEST_ASSERT_EQUAL('d', data[1]);
  buffer.discard(1);  // le réseau n'a pris qu'un octet : 'd' reste en tête

  TEST_ASSERT_EQUAL(1, buffer.peekContiguous(data));
  TEST_ASSERT_EQUAL('d', data[0]);
  buffer.discard(1);

  TEST_ASSERT_EQUAL(1, buffer.peekContiguous(data));  // la suite, au début du tableau
  TEST_ASSERT_EQUAL('e', data[0]);
  buffer.discard(5);  // retirer plus que ce qui reste ne casse rien
  TEST_ASSERT_TRUE(buffer.isEmpty());
}

int runAllTests() {
  UNITY_BEGIN();
  RUN_TEST(test_bytes_come_out_in_order);
  RUN_TEST(test_empty_buffer_returns_minus_one);
  RUN_TEST(test_full_buffer_refuses_and_counts_lost_bytes);
  RUN_TEST(test_wrap_around_keeps_order);
  RUN_TEST(test_peek_contiguous_then_discard_partial_send);
  return UNITY_END();
}

#ifdef ARDUINO
#include <Arduino.h>
void setup() {
  delay(2000);  // programme de test uniquement : laisse le temps au moniteur série de se connecter
  runAllTests();
}
void loop() {}
#else
int main() {
  return runAllTests();
}
#endif

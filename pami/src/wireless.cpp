#include "wireless.h"

#include <Arduino.h>
#include <ArduinoOTA.h>
#include <WiFi.h>
#include <errno.h>
#include <lwip/sockets.h>

#include "config.h"
#include "console.h"
#include "motor_driver.h"

// Nom et mot de passe du réseau : hors de GitHub (voir wifi_secrets.example.h).
#if __has_include("wifi_secrets.h")
#include "wifi_secrets.h"
#else
#warning "wifi_secrets.h absent : mot de passe d'exemple. Copiez wifi_secrets.example.h en wifi_secrets.h."
#include "wifi_secrets.example.h"
#endif

namespace wireless {

namespace {

WiFiServer telnetServer(WIFI_TELNET_PORT, 1);  // un seul PC à la fois
WiFiClient telnetClient;
bool radioOn = false;
bool radioFailed = false;  // réseau impossible à créer : on ne réessaie pas à chaque tour
uint32_t lastUpdateMs = 0;

// Envoie au PC ce qui attend dans le tampon, SANS JAMAIS ATTENDRE le réseau.
// WiFiClient::write() peut bloquer jusqu'à 10 s quand le réseau est lent (10 essais d'1 s,
// voir WiFiClient.cpp) : interdit dans loop() (règle d'or n° 4). send(..., MSG_DONTWAIT) prend
// tout de suite ce que le réseau accepte ; le reste attend dans le tampon le tour suivant.
void sendPendingOutput() {
  if (!telnetClient.connected()) {
    return;  // pas de PC : les messages restent pour le prochain
  }
  ByteRingBuffer<WIFI_LOG_BUFFER_BYTES>& output = Console.wifiOutput();
  // Le tampon est circulaire : au plus deux morceaux (jusqu'au bout du tableau, puis le début).
  for (int part = 0; part < 2 && !output.isEmpty(); part++) {
    const uint8_t* data = nullptr;
    const size_t length = output.peekContiguous(data);
    const int sent = send(telnetClient.fd(), data, length, MSG_DONTWAIT);
    if (sent < 0) {
      if (errno != EAGAIN && errno != EWOULDBLOCK) {
        telnetClient.stop();  // le PC est parti
      }
      return;  // réseau occupé : on réessaie au prochain passage
    }
    output.discard((size_t)sent);
    if ((size_t)sent < length) {
      return;
    }
  }
}

// Messages perdus (tampon plein, PC absent ou trop lent) : on le dit une fois, dès qu'il y a
// de nouveau de la place.
void reportLostMessages() {
  ByteRingBuffer<WIFI_LOG_BUFFER_BYTES>& output = Console.wifiOutput();
  const uint32_t lostBytes = output.lostBytes();
  if (lostBytes == 0 || output.freeSpace() < WIFI_LOG_BUFFER_BYTES / 2) {
    return;
  }
  output.resetLostBytes();
  Console.printf("[WIFI] %lu octets de messages perdus (tampon plein : PC absent ou trop lent).\n",
                 (unsigned long)lostBytes);
}

// Touches tapées dans le moniteur Wi-Fi. On garde les caractères imprimables et Entrée :
// un vrai client telnet envoie aussi des octets de négociation, qu'on ignore.
void receiveKeys() {
  while (telnetClient.available() > 0) {
    const int c = telnetClient.read();
    if ((c >= 32 && c <= 126) || c == '\r' || c == '\n') {
      const uint8_t byte = (uint8_t)c;
      Console.wifiInput().push(&byte, 1);
    }
  }
}

void acceptClient() {
  WiFiClient incoming = telnetServer.available();  // ne bloque pas (socket non bloquante)
  if (!incoming) {
    return;
  }
  telnetClient.stop();  // un seul PC à la fois : le dernier connecté remplace l'ancien
  telnetClient = incoming;
  telnetClient.setNoDelay(true);  // chaque message part tout de suite, sans attendre le suivant
  Console.println("[WIFI] PC connecte : d'abord les messages gardes en memoire, puis la suite en direct.");
}

void configureUpdate() {
  ArduinoOTA.setMdnsEnabled(false);  // adresse fixe : pas besoin de mDNS pour trouver le PAMI
  ArduinoOTA.onStart([]() {
    // Pendant un téléversement, loop() ne tourne plus : on serre les freins tout de suite.
    // C'est la seule exception au chemin « safety -> driver », justement parce que loop() est figée.
    // Double sécurité : le téléversement n'est accepté que PAMI immobile (updateAllowed).
    motorDriver::brake();
    Console.println("[WIFI] Televersement sans fil : moteurs freines, ne coupez pas l'alimentation.");
    sendPendingOutput();
  });
  ArduinoOTA.onEnd([]() {
    Console.println("[WIFI] Televersement termine : redemarrage.");
    sendPendingOutput();
  });
  ArduinoOTA.onError([](ota_error_t error) {
    Console.printf("[WIFI] Televersement rate (erreur %u) : le programme actuel continue.\n",
                   (unsigned)error);
  });
}

void startRadio() {
  WiFi.mode(WIFI_AP);
  if (!WiFi.softAP(WIFI_AP_SSID, WIFI_AP_PASSWORD, WIFI_AP_CHANNEL)) {
    Console.println("[WIFI] ERREUR : reseau non cree. Mot de passe d'au moins 8 caracteres ?");
    WiFi.mode(WIFI_OFF);
    radioFailed = true;
    return;
  }
  telnetServer.begin();
  telnetServer.setNoDelay(true);
  ArduinoOTA.begin();
  radioOn = true;
  Console.printf("[WIFI] Reseau %s, adresse %s, moniteur sur le port %u. Televersement : PAMI immobile.\n",
                 WIFI_AP_SSID, WiFi.softAPIP().toString().c_str(), (unsigned)WIFI_TELNET_PORT);
}

void stopRadio() {
  Console.println("[WIFI] Radio coupee pendant le match (reglement F.6). Messages gardes en memoire.");
  sendPendingOutput();  // dernière chance de partir avant la coupure
  telnetClient.stop();
  telnetServer.end();
  ArduinoOTA.end();
  WiFi.softAPdisconnect(true);
  WiFi.mode(WIFI_OFF);
  radioOn = false;
}

}  // namespace

void begin() {
  configureUpdate();
  startRadio();
  lastUpdateMs = millis();
}

void update(uint32_t nowMs, bool updateAllowed) {
  if (!radioOn || nowMs - lastUpdateMs < WIFI_UPDATE_PERIOD_MS) {
    return;
  }
  lastUpdateMs = nowMs;

  acceptClient();
  if (telnetClient.connected()) {
    receiveKeys();
    sendPendingOutput();
  }
  reportLostMessages();

  // Téléversement seulement PAMI immobile. Sinon la demande reste sans réponse, et PlatformIO
  // affiche « No response from the ESP » : il suffit de recommencer une fois le PAMI arrêté.
  if (updateAllowed) {
    ArduinoOTA.handle();
  }
}

void setRadioOn(bool on) {
  if (on == radioOn || (on && radioFailed)) {
    return;
  }
  if (on) {
    startRadio();
  } else {
    stopRadio();
  }
}

}  // namespace wireless

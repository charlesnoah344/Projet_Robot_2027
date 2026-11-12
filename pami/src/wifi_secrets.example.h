// Modèle du fichier wifi_secrets.h : nom et mot de passe du réseau Wi-Fi du PAMI (D010).
//
// 1. Copiez ce fichier en wifi_secrets.h, dans le même dossier (pami/src/).
// 2. Changez le mot de passe (8 caractères au moins). Pour le 2e PAMI, changez aussi le nom.
//
// wifi_secrets.h n'est pas envoyé sur GitHub (.gitignore) : le mot de passe reste dans l'équipe.
// Il protège aussi le téléversement sans fil : seuls les appareils connectés au réseau du PAMI
// peuvent lui envoyer un programme.
// Tant que wifi_secrets.h n'existe pas, la compilation utilise ce modèle et affiche un avertissement.
#pragma once

constexpr const char* WIFI_AP_SSID = "PAMI-1";
constexpr const char* WIFI_AP_PASSWORD = "changez-moi";

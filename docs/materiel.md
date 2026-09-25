# Matériel du PAMI

> Source de vérité pour le matériel et le brochage. `pami/src/config.h` recopie les broches d'ici.
> Quand une référence exacte est connue, remplace « à préciser ».

## Inventaire actuel
| Composant | Qté | Référence exacte | Rôle |
|---|---|---|---|
| Partie mobile (châssis) | 1 | à préciser | Structure et roues |
| ESP32 | 1 | à préciser (ex. DevKit V1 WROOM-32) | Contrôleur principal |
| Émetteur/récepteur infrarouge | 2 | à préciser | Détection des obstacles dans les angles avant |
| Bouton d'arrêt d'urgence | 1 | à préciser (rouge, Ø ≥ 20 mm) | Coupe la puissance des moteurs |
| Servomoteur | 1 | à préciser | Actionneur d'attaque (plus tard) |
| Capteur ultrason | 1 | à préciser (HC-SR04 ? HC-SR04P ?) | Détection des obstacles devant |
| Bouton d'alimentation | 1 | à préciser | Interrupteur général |
| Micro moteur DC | 2 | à préciser (avec ou sans encodeur ?) | Propulsion gauche / droite |
| Driver moteur DC | 1 | à préciser (L298N ? TB6612FNG ? DRV8833 ?) | Pont en H |

## Pièces manquantes
| Pièce | Priorité | Pourquoi |
|---|---|---|
| Batterie + support + chargeur | Bloquant | Aucune alimentation dans l'inventaire. NiMH ou LiFePO4 : pas de conditions. LiPo : sac ignifuge et chargeur présentés à l'homologation. |
| Tirette de départ (microrupteur + goupille, ou prise jack) + cordon ≥ 50 cm | Bloquant | Seul démarrage manuel autorisé. |
| Sélecteur de couleur (interrupteur) | Bloquant | Le PAMI doit jouer bleu et jaune. |
| Gyroscope / IMU (ex. MPU6050) | Fortement conseillé | Sans encodeurs, c'est le seul moyen fiable d'aller droit et de tourner précisément. |
| Moteurs avec encodeurs | Optionnel | Mesurer la distance parcourue plutôt que l'estimer au temps. |
| Résistances pour pont diviseur | Selon le capteur | Echo d'un HC-SR04 en 5 V vers l'ESP32 en 3,3 V. Lecture de l'état du BAU. |

## Brochage ESP32 (proposition à valider)
Proposition pour un ESP32 DevKit V1 (WROOM-32). Colonne « Validé » : à cocher après un test isolé réussi.

| Fonction | Broche proposée | Remarque | Validé |
|---|---|---|---|
| Moteur gauche PWM | GPIO 25 | | [ ] |
| Moteur gauche sens 1 / sens 2 | GPIO 26 / 27 | | [ ] |
| Moteur droit PWM | GPIO 33 | | [ ] |
| Moteur droit sens 1 / sens 2 | GPIO 32 / 13 | | [ ] |
| Ultrason TRIG | GPIO 18 | | [ ] |
| Ultrason ECHO | GPIO 19 | Pont diviseur si capteur en 5 V | [ ] |
| IR avant-gauche | GPIO 34 | Entrée seule, pas de pull-up interne | [ ] |
| IR avant-droit | GPIO 35 | Entrée seule, pas de pull-up interne | [ ] |
| Servomoteur | GPIO 23 | Bibliothèque ESP32Servo | [ ] |
| Tirette | GPIO 16 | INPUT_PULLUP | [ ] |
| Sélecteur de couleur | GPIO 17 | INPUT_PULLUP | [ ] |
| Lecture de l'état du BAU (optionnel) | GPIO 39 | Entrée seule, pont diviseur obligatoire | [ ] |
| Gyroscope I2C SDA / SCL | GPIO 21 / 22 | | [ ] |

**Broches à éviter**
- GPIO 6 à 11 : reliées à la mémoire flash, inutilisables.
- GPIO 0, 2, 12, 15 : broches de démarrage (strapping). GPIO 12 à l'état haut au démarrage empêche l'ESP32 de démarrer.
- ADC2 (GPIO 0, 2, 4, 12 à 15, 25 à 27) : pas de lecture analogique quand le Wi-Fi ou ESP-NOW est actif.

## Alimentation (schéma de principe)
```
Batterie ──> Interrupteur général ──┬──> Régulateur / broche 5V ──> ESP32 + capteurs
                                    └──> BAU (en série) ──> Driver moteurs ──> 2 moteurs
Toutes les masses (GND) sont reliées entre elles.
```

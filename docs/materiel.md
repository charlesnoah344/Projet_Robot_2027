# Matériel du PAMI

> Source de vérité pour le matériel et le brochage. `pami/src/config.h` recopie les broches d'ici.
> Quand une référence exacte est connue, remplace « à préciser ».

## Inventaire actuel
| Composant | Qté | Référence exacte | Rôle |
|---|---|---|---|
| Partie mobile (châssis) | 1 | à préciser | Structure et roues |
| ESP32 | 1 | ESP-32U (puce ESP32-D0WD-V3, révision 3.1). **Ne passe pas toute seule en mode téléversement** : maintenir BOOT pendant « Connecting » (T001) | Contrôleur principal |
| Émetteur/récepteur infrarouge | 2 | MH-Sensor-Series (Flying-Fish), 3 broches : VCC, GND, OUT. Sortie tout-ou-rien (comparateur LM393), niveau bas = obstacle. Portée réglée au potentiomètre | Détection des obstacles dans les angles avant |
| Bouton d'arrêt d'urgence | 1 | à préciser (rouge, Ø ≥ 20 mm) | Coupe la puissance des moteurs |
| Servomoteur | 1 | tower pro micro servo 9g sg90  | Actionneur d'attaque (plus tard) |
| Capteur ultrason | 1 | HC-SR04 (alimenté en 5 V) | Détection des obstacles devant |
| Bouton d'alimentation | 1 | à préciser | Interrupteur général |
| Micro moteur DC | 2 | FIT0450 (DFRobot) : 6 V nominal (3 à 7,5 V), réduction 120:1, 160 tr/min à vide sous 6 V, 2,8 A bloqué | Propulsion gauche / droite |
| Encodeur (intégré au moteur) | 2 | Encodeur à effet Hall du FIT0450, 2 voies A/B. Alimentation 4,5 à 7,5 V. Fiche : 960 impulsions par tour de roue et par voie | Mesure la rotation de chaque roue : distance parcourue, vitesse, angle de virage (D004) |
| Driver moteur DC | 1 | MDD3A (Cytron) : 4 à 16 V, 3 A par voie, 2 entrées par moteur (M1A/M1B, M2A/M2B), entrées compatibles 3,3 V, PWM jusqu'à 20 kHz | Pont en H (D006) |
| Alimentation de labo | 1 | réglée à 6 V | Alimente les moteurs pendant les tests, en attendant la batterie |

## Pièces manquantes
| Pièce | Priorité | Pourquoi |
|---|---|---|
| Batterie + support + chargeur | Bloquant | Aucune alimentation embarquée dans l'inventaire. NiMH ou LiFePO4 : pas de conditions. LiPo : sac ignifuge et chargeur présentés à l'homologation. |
| Tirette de départ (microrupteur + goupille, ou prise jack) + cordon ≥ 50 cm | Bloquant | Seul démarrage manuel autorisé. Le bouton BOOT n'est qu'un départ provisoire de développement (D005). |
| Sélecteur de couleur (interrupteur) | Bloquant | Le PAMI doit jouer bleu et jaune. |
| Résistances | Bloquant pour les étapes 3 à 6 | Ponts diviseurs : ECHO du HC-SR04 (5 V → 3,3 V), voies A/B des encodeurs si leurs signaux sont en 5 V (4 ponts, voir « Points à vérifier »), lecture de l'état du BAU. Plus 2 × 100 kΩ entre la sortie des IR et la masse. |
| Gyroscope / IMU (ex. MPU6050) | Optionnel (plus tard) | Les encodeurs suffisent pour les distances et la ligne droite (D004). Un gyroscope aiderait si les virages dérivent à cause du glissement des roues. |

## Brochage ESP32 (proposition à valider)
Proposition pour un ESP32 DevKit V1 (WROOM-32). Colonne « Validé » : à cocher après un test isolé réussi.

| Fonction | Broche proposée | Remarque | Validé |
|---|---|---|---|
| Moteur gauche M1A / M1B | GPIO 25 / 26 | MDD3A : A = PWM et B = 0 pour avancer, A = 0 et B = PWM pour reculer, A = B = 0 pour freiner | [ ] |
| Moteur droit M2A / M2B | GPIO 33 / 32 | Même logique que le moteur gauche | [ ] |
| Encodeur gauche A / B | GPIO 27 / 14 | Interruption sur A. Pont diviseur si le signal est en 5 V | [ ] |
| Encodeur droit A / B | GPIO 13 / 4 | Interruption sur A. Pont diviseur si le signal est en 5 V | [ ] |
| Ultrason TRIG | GPIO 18 | | [ ] |
| Ultrason ECHO | GPIO 19 | Pont diviseur obligatoire (HC-SR04 en 5 V) | [ ] |
| IR avant-gauche | GPIO 34 | Entrée seule, pas de pull-up interne. Module alimenté en 3,3 V. 100 kΩ vers la masse : fil débranché = « obstacle » | [ ] |
| IR avant-droit | GPIO 35 | Idem IR avant-gauche | [ ] |
| Servomoteur | GPIO 23 | Bibliothèque ESP32Servo | [ ] |
| Départ provisoire (bouton BOOT de la carte) | GPIO 0 | Bouton déjà soudé sur la carte, appui = niveau bas. Provisoire (D005), remplacé par la tirette. Ne pas appuyer pendant l'allumage : la carte passerait en mode téléversement | [ ] |
| Tirette | GPIO 16 | INPUT_PULLUP | [ ] |
| Sélecteur de couleur | GPIO 17 | INPUT_PULLUP | [ ] |
| Lecture de l'état du BAU (optionnel) | GPIO 39 | Entrée seule, pont diviseur obligatoire | [ ] |
| Gyroscope I2C SDA / SCL (optionnel) | GPIO 21 / 22 | Réservées pour plus tard | [ ] |

Pourquoi ces broches pour les encodeurs :
- elles acceptent les interruptions ;
- ce ne sont ni des broches de démarrage (0, 2, 5, 12, 15), ni les broches du port série (1, 3) ;
- ce ne sont pas GPIO 36 ou 39.

**Broches à éviter**
- GPIO 6 à 11 : reliées à la mémoire flash, inutilisables.
- GPIO 0, 2, 12, 15 : broches de démarrage (strapping). GPIO 12 à l'état haut au démarrage empêche l'ESP32 de démarrer.
- GPIO 1 et 3 : port série USB (téléversement et moniteur série).
- GPIO 36 et 39 : peuvent déclencher de fausses interruptions (errata de l'ESP32). À éviter pour les encodeurs.
- ADC2 (GPIO 0, 2, 4, 12 à 15, 25 à 27) : pas de lecture analogique quand le Wi-Fi ou ESP-NOW est actif.

## Points à vérifier
Cocher après vérification, et noter la mesure dans `docs/journal-tests.md`.

**Encodeurs**
- [ ] Tension d'alimentation : la fiche DFRobot demande 4,5 à 7,5 V. On les alimente en 5 V, pas en 3,3 V.
- [ ] Niveau des signaux A et B. **À mesurer avant tout branchement sur l'ESP32**, qui n'accepte pas plus de 3,3 V. Méthode :
  - alimenter l'encodeur en 5 V ;
  - tourner la roue lentement à la main ;
  - mesurer la tension de A au multimètre.
  Selon la mesure :
  - le signal alterne entre 0 et 5 V → un pont diviseur sur chaque voie (par exemple 10 kΩ en série et 20 kΩ vers la masse) ;
  - le signal reste à 0 V → sortie « collecteur ouvert », le pull-up interne de l'ESP32 suffit.
- [ ] Ticks par tour de roue : 960 impulsions par voie d'après la fiche, soit 1920 avec notre comptage (2 fronts de A). À vérifier en tournant la roue 10 fois à la main (étape 3).
- [ ] Couleurs et ordre des fils : 6 fils (moteur +, moteur −, A, B, GND encodeur, + encodeur). La FAQ DFRobot signale des étiquettes inversées sur la révision 2.0 de la carte : vérifier au multimètre.

**Capteurs IR**
- [ ] Alimentés en 3,3 V, pour que la sortie ne dépasse pas 3,3 V.
- [ ] Résistance de 100 kΩ entre la sortie et la masse, côté ESP32.
- [ ] Portée réglée au potentiomètre, puis mesurée (étape 6).

**Ultrason HC-SR04**
- [ ] Alimenté en 5 V, pont diviseur sur ECHO.
- [ ] Monté à moins de 70 mm de haut, pour voir les bordures de la table (règles 2027 : bordures de 70 mm).

**Alimentation pendant les tests**
- [ ] Moteurs : alimentation de labo à 6 V sur l'entrée puissance du MDD3A, à travers le BAU.
- [ ] ESP32 par USB. Encodeurs et HC-SR04 sur la broche 5 V de la carte.
- [ ] Masse commune : GND de l'alimentation de labo = GND du MDD3A = GND de l'ESP32 = GND des capteurs.

## Alimentation (schéma de principe)
```
Batterie ──> Interrupteur général ──┬──> Régulateur / broche 5V ──> ESP32 + capteurs
                                    └──> BAU (en série) ──> Driver moteurs ──> 2 moteurs
Toutes les masses (GND) sont reliées entre elles.
```

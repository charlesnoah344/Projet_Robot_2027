# Matériel du PAMI

> Source de vérité pour le matériel et le brochage. `pami/src/config.h` recopie les broches d'ici.
> Quand une référence exacte est connue, remplace « à préciser ».

## Inventaire actuel
| Composant | Qté | Référence exacte | Rôle |
|---|---|---|---|
| Partie mobile (châssis) | 1 | à préciser. Roues de 60 mm de diamètre, entraxe de 100 mm | Structure et roues |
| ESP32 | 1 | ESP-32U (puce ESP32-D0WD-V3, révision 3.1). **Ne passe pas toute seule en mode téléversement** : maintenir BOOT pendant « Connecting » (T001) | Contrôleur principal |
| PCB du PAMI | 1 | « Test PCB 2026 5EO » (D015, remplace le premier PCB de D012) : ESP32, entrées du driver, connecteurs des moteurs, des encodeurs, de 2 ultrasons (TRIG commun), du servo et d'un accéléromètre I2C. D'après le pinout, une sortie OUT+ / OUT− alimente ultrasons, encodeurs et servo : tension à mesurer | Remplace le câblage sur plaque d'essai |
| Bouton d'arrêt d'urgence | 1 | à préciser (rouge, Ø ≥ 20 mm) | Coupe la puissance des moteurs |
| Servomoteur | 1 | tower pro micro servo 9g sg90  | Bras du PAMI : touche le PAMI adverse (D007) |
| Capteur ultrason | 2 | HC-SR04 (alimentés en 5 V), à l'avant gauche et à l'avant droit, TRIG commun | Détection des obstacles devant. Les deux sont combinés par un OU : un seul qui voit un obstacle suffit (D012, D015) |
| Interrupteur 2 positions | 1 | à préciser. Pas prévu sur le pinout du PCB : à câbler sur GPIO 27 | Côté de départ : gauche ou droite de la table (D014) |
| Bouton d'alimentation | 1 | à préciser | Interrupteur général |
| Micro moteur DC | 2 | FIT0450 (DFRobot) : 6 V nominal (3 à 7,5 V), réduction 120:1, 160 tr/min à vide sous 6 V, 2,8 A bloqué | Propulsion gauche / droite |
| Encodeur (intégré au moteur) | 2 | Encodeur à effet Hall du FIT0450, 2 voies A/B. Alimentation 4,5 à 7,5 V. Fiche : 960 impulsions par tour de roue et par voie | Mesure la rotation de chaque roue : distance parcourue, vitesse, angle de virage (D004) |
| Driver moteur DC | 1 | MDD3A (Cytron) : 4 à 16 V, 3 A par voie, 2 entrées par moteur (M1A/M1B, M2A/M2B), entrées compatibles 3,3 V, PWM jusqu'à 20 kHz | Pont en H (D006) |
| Alimentation de labo | 1 | réglée à 6 V | Alimente les moteurs pendant les tests, en attendant la batterie |

## Pièces manquantes
| Pièce | Priorité | Pourquoi |
|---|---|---|
| Batterie + support + chargeur | Bloquant | Aucune alimentation embarquée dans l'inventaire. NiMH ou LiFePO4 : pas de conditions. LiPo : sac ignifuge et chargeur présentés à l'homologation. |
| Démarrage homologable (cordon ≥ 50 cm, ou départ donné par le robot principal) | Bloquant (Eurobot) | Au labo, le départ se donne en tapant `go` dans le moniteur Wi-Fi (D013). C'est interdit à Eurobot (règlement général F.6) : il faudra un cordon, ou un départ envoyé par le robot principal. Aucune broche n'est prévue pour un cordon sur le PCB. |
| Régulateur 5 V abaisseur (≥ 2 A) | Bloquant (sur batterie) | Alimente l'ESP32, les capteurs, les encodeurs et le servo. La sortie 5V0 du MDD3A ne suffit pas pour le servo (`docs/plan-logiciel-pami.md`, § 5). |
| Condensateurs 470 µF et 100 µF | Recommandé | 470 µF près du servo, 100 µF près de VB+/VB− du MDD3A : absorbent les pics de courant. |
| Gyroscope / IMU (ex. MPU6050) | Optionnel (plus tard) | Les encodeurs suffisent pour les distances et la ligne droite (D004). Un gyroscope aiderait si les virages dérivent à cause du glissement des roues. Le PCB a déjà un connecteur « Accel » (I2C, GPIO 22 / 23) : référence du module à préciser. |

## Brochage ESP32 (PCB « Test PCB 2026 5EO », D015)
Brochage du pinout du PCB « Test PCB 2026 5EO » et du programme de test de l'équipe (21/11/2026).
Il remplace celui du premier PCB (D012). Colonne « Validé » : à cocher après un test isolé réussi
**avec le firmware du PAMI** (les sens des moteurs et des encodeurs sont à revérifier).
Broches utilisables d'après le pinout : 2, 4, 5, 12 à 19, 21 à 23, 25 à 27, 32, 33.

| Fonction | Broche | Remarque | Validé |
|---|---|---|---|
| Moteur gauche M1A / M1B | GPIO 32 / 33 | MDD3A : A = PWM et B = 0 pour avancer, A = 0 et B = PWM pour reculer, A = B = 0 pour freiner | [ ] |
| Moteur droit M2A / M2B | GPIO 19 / 18 | Même logique que le moteur gauche | [ ] |
| Encodeur gauche A / B | GPIO 25 / 26 | « COD G B » / « COD G V » sur le pinout. Interruption sur A. Pull-up interne activé | [ ] |
| Encodeur droit A / B | GPIO 5 / 17 | « COD D B » / « COD D V » sur le pinout. Interruption sur A. Pull-up interne activé | [ ] |
| TRIG des 2 ultrasons | GPIO 21 | **Commun aux deux capteurs** : ils émettent toujours en même temps (D015) | [ ] |
| ECHO ultrason gauche | GPIO 2 | Broche de démarrage : ECHO au repos à l'état bas, sans risque. Sur certaines cartes, GPIO 2 porte aussi une LED : vérifier que l'écho est bien lu | [ ] |
| ECHO ultrason droit | GPIO 15 | Broche de démarrage : ECHO au repos à l'état bas, sans risque | [ ] |
| Servomoteur (bras) | GPIO 13 | Piloté par LEDC, canal 4 (D008). Alimentation « Servo + » du PCB, jamais le 3V3 de l'ESP32 | [ ] |
| Interrupteur de côté (gauche / droite) | GPIO 27 | **Pas sur le pinout : à câbler** entre GPIO 27 et 3,3 V. Pull-down : ouvert = 0 = GAUCHE, fermé = 1 = DROITE (D014, D015). Marquer « G » et « D » à côté de l'interrupteur | [ ] |
| Accéléromètre I2C SDA / SCL | GPIO 22 / 23 | Prévu sur le PCB (« Accel + » sur le 3,3 V). **Pas encore utilisé** par le firmware (D004 : optionnel) | [ ] |
| ~~Départ provisoire (bouton BOOT de la carte)~~ | GPIO 0 | Sert seulement au téléversement | [x] T001 |
| Départ | — | Plus de bouton ni de tirette : commande `go` tapée dans le moniteur, Wi-Fi ou USB (D013) | [ ] |
| Lecture de l'état du BAU (optionnel) | à définir | Entrée seule conseillée (34, 36 ou 39), pont diviseur obligatoire | [ ] |

Broches encore libres d'après le pinout : 4, 12, 14, 16 (et les entrées seules 34, 35, 36, 39).
GPIO 12 est une broche de démarrage : rien ne doit la tirer à l'état haut à l'allumage.

Anciens brochages, **obsolètes** :
- premier PCB (D012) : moteurs 26/27 et 17/16, encodeurs 33/32 et 18/19, ultrasons 25/35 et 5/4,
  interrupteur 22 ;
- plaque d'essai : `docs/pdf/cablage complet pami.pdf` et `docs/cablage-pami.html`.

Remarques sur les broches du PCB :
- GPIO 2, 5 et 15 sont des broches de démarrage. Un ECHO (2, 15) est à l'état bas au repos ; l'encodeur
  droit (5) a un pull-up. Si la carte refuse de démarrer ou de se téléverser avec tout branché,
  débrancher les ultrasons et l'encodeur droit pour vérifier ;
- GPIO 16 et 17 sont libres sur notre ESP32 (pas de mémoire PSRAM).

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
- [x] Niveau des signaux A et B : mesurés par l'équipe, ils alternent entre 0 et 3,3 V (encodeur alimenté en 5 V). Branchement direct. Méthode utilisée :
  - alimenter l'encodeur en 5 V ;
  - tourner la roue lentement à la main ;
  - mesurer la tension de A au multimètre.
  Selon la mesure :
  - le signal alterne entre 0 et 5 V → un pont diviseur sur chaque voie (par exemple 10 kΩ en série et 20 kΩ vers la masse) ;
  - le signal reste à 0 V → sortie « collecteur ouvert », le pull-up interne de l'ESP32 suffit.
- [ ] Ticks par tour de roue : 960 impulsions par voie d'après la fiche, soit 1920 avec notre comptage (2 fronts de A). À vérifier en tournant la roue 10 fois à la main (étape 3).
- [ ] Couleurs et ordre des fils : 6 fils (moteur +, moteur −, A, B, GND encodeur, + encodeur). La FAQ DFRobot signale des étiquettes inversées sur la révision 2.0 de la carte : vérifier au multimètre.

- [ ] Sens de comptage avec le PCB 2026 5EO (D015) : roue tournée vers l'avant à la main → son
  compteur doit augmenter (banc `test_odometrie`). Sinon, inverser `ENCODER_*_INVERTED` dans `config.h`.
- Les capteurs IR de sol ont été retirés (D012) : le PAMI trouve la douve par l'odométrie seule.

**Ultrasons HC-SR04 (2, D012, D015)**
- [ ] Alimentés en 5 V. Pont diviseur sur chaque ECHO : à confirmer sur le schéma du PCB.
- [ ] Montés à moins de 70 mm de haut, pour voir les bordures de la table (règles 2027 : bordures de 70 mm).
- [ ] Chacun voit un obstacle placé devant lui seul (banc `test_ultrason`).
- [ ] TRIG commun : les deux émettent en même temps. Rien devant pendant 10 s → aucune mesure sous
  300 mm (un capteur n'entend pas le son de l'autre). Banc `test_ultrason`, protocole 6.

**Interrupteur de côté (D014)**
- [ ] Le moniteur affiche `[COTE] GAUCHE` interrupteur ouvert, `[COTE] DROITE` fermé.
  Marquer « G » et « D » à côté de l'interrupteur sur le PAMI.

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
Le câblage des signaux est maintenant sur le PCB (D012). Le schéma d'alimentation détaillé
(régulateur 5 V, servo) reste dans `docs/plan-logiciel-pami.md`, § 5 ; les broches qui y figurent
sont celles de l'ancien montage.

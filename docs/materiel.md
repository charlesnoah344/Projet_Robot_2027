# Matériel du PAMI

> Source de vérité pour le matériel et le brochage. `pami/src/config.h` recopie les broches d'ici.
> Quand une référence exacte est connue, remplace « à préciser ».

## Inventaire actuel
| Composant | Qté | Référence exacte | Rôle |
|---|---|---|---|
| Partie mobile (châssis) | 1 | à préciser. Roues de 60 mm de diamètre, entraxe de 100 mm | Structure et roues |
| ESP32 | 1 | ESP-32U (puce ESP32-D0WD-V3, révision 3.1). **Ne passe pas toute seule en mode téléversement** : maintenir BOOT pendant « Connecting » (T001) | Contrôleur principal |
| PCB du PAMI | 1 | Brochage du 25/11/2026 (D018, presque celui du premier PCB de D012 ; remplace « Test PCB 2026 5EO », D015) : ESP32, entrées du driver, connecteurs des moteurs, des encodeurs, de 2 ultrasons (un TRIG chacun) et du servo. D'après le pinout, une sortie OUT+ / OUT− alimente ultrasons, encodeurs et servo : tension à mesurer | Remplace le câblage sur plaque d'essai |
| Bouton d'arrêt d'urgence | 1 | à préciser (rouge, Ø ≥ 20 mm) | Coupe la puissance des moteurs |
| Servomoteur | 1 | tower pro micro servo 9g sg90  | Bras du PAMI : touche le PAMI adverse (D007) |
| Capteur ultrason | 2 | HC-SR04 (alimentés en 5 V), à l'avant gauche et à l'avant droit, TRIG commun | Détection des obstacles devant. Les deux sont combinés par un OU : un seul qui voit un obstacle suffit (D012, D015) |
| ~~Interrupteur 2 positions~~ | 0 | Plus nécessaire au labo : le côté se choisit avec la commande de départ (D020). À Eurobot, il en faudra peut-être un (départ Wi-Fi interdit) | Côté GAUCHE / DROITE (D014, D017) |
| Bouton d'alimentation | 1 | à préciser | Interrupteur général |
| Micro moteur DC | 2 | FIT0450 (DFRobot) : 6 V nominal (3 à 7,5 V), réduction 120:1, 160 tr/min à vide sous 6 V, 2,8 A bloqué | Propulsion gauche / droite |
| Encodeur (intégré au moteur) | 2 | Encodeur à effet Hall du FIT0450, 2 voies A/B. Alimentation 4,5 à 7,5 V. Fiche : 960 impulsions par tour de roue et par voie | Mesure la rotation de chaque roue : distance parcourue, vitesse, angle de virage (D004) |
| Driver moteur DC | 1 | MDD3A (Cytron) : 4 à 16 V, 3 A par voie, 2 entrées par moteur (M1A/M1B, M2A/M2B), entrées compatibles 3,3 V, PWM jusqu'à 20 kHz | Pont en H (D006) |
| Alimentation de labo | 1 | réglée à 6 V | Alimente les moteurs pendant les tests, en attendant la batterie |

## Pièces manquantes
| Pièce | Priorité | Pourquoi |
|---|---|---|
| Batterie + support + chargeur | Bloquant | Aucune alimentation embarquée dans l'inventaire. NiMH ou LiFePO4 : pas de conditions. LiPo : sac ignifuge et chargeur présentés à l'homologation. |
| Démarrage homologable (cordon ≥ 50 cm, ou départ donné par le robot principal) | Bloquant (Eurobot) | Au labo, le départ se donne en tapant `go g` ou `go d` dans le moniteur Wi-Fi (D013, D020). C'est interdit à Eurobot (règlement général F.6) : il faudra un cordon, ou un départ envoyé par le robot principal. Aucune broche n'est prévue pour un cordon : en prendre une libre (voir le brochage). |
| Régulateur 5 V abaisseur (≥ 2 A) | Bloquant (sur batterie) | Alimente l'ESP32, les capteurs, les encodeurs et le servo. La sortie 5V0 du MDD3A ne suffit pas pour le servo (`docs/plan-logiciel-pami.md`, § 5). |
| Condensateurs 470 µF et 100 µF | Recommandé | 470 µF près du servo, 100 µF près de VB+/VB− du MDD3A : absorbent les pics de courant. |
| Gyroscope / IMU (ex. MPU6050) | Optionnel (plus tard) | Les encodeurs suffisent pour les distances et la ligne droite (D004). Un gyroscope aiderait si les virages dérivent à cause du glissement des roues. Le brochage du 25/11/2026 (D018) ne prévoit plus d'I2C : GPIO 22 / 23 sont libres pour en brancher un. |

## Brochage ESP32 (brochage du 25/11/2026, D018)
Brochage donné par l'équipe le 25/11/2026. C'est presque celui du premier PCB (D012) ; seul
l'ultrason droit change (TRIG 5 / ECHO 4, T016). Il remplace celui du PCB « Test PCB 2026 5EO » (D015).
Colonne « Validé » : à cocher après un test isolé réussi **avec le firmware du PAMI** (les sens des
moteurs et des encodeurs sont à revérifier).

| Fonction | Broche | Remarque | Validé |
|---|---|---|---|
| Moteur gauche M1 : IN1 (A) / IN2 (B) | GPIO 27 / 26 | MDD3A : A = PWM et B = 0 pour avancer, A = 0 et B = PWM pour reculer, A = B = 0 pour freiner | [ ] |
| Moteur droit M2 : IN1 (A) / IN2 (B) | GPIO 16 / 17 | Même logique que le moteur gauche. 16 et 17 sont libres sur un module ESP32-WROOM (sans PSRAM) | [ ] |
| Encodeur gauche A / B | GPIO 33 / 32 | Interruption sur A. Pull-up interne activé | [ ] |
| Encodeur droit A / B | GPIO 19 / 18 | Interruption sur A. Pull-up interne activé | [ ] |
| Ultrason gauche TRIG / ECHO | GPIO 25 / 35 | « Sensor 1 » du programme de l'équipe. 35 est une entrée seule : parfait pour un ECHO. Pont diviseur sur ECHO | [ ] |
| Ultrason droit TRIG / ECHO | GPIO 5 / 4 | Connecteur droit du PCB (T016 : rien sur 14 / 12, qui est le connecteur du bas). GPIO 5 est une broche de démarrage : sans risque pour un TRIG (sortie). Pont diviseur sur ECHO | [x] T016 |
| Servomoteur (bras) | GPIO 13 | Piloté par LEDC, canal 4 (D008). Alimentation « Servo + » du PCB, jamais le 3V3 de l'ESP32 | [ ] |
| Côté GAUCHE / DROITE | — | Plus d'interrupteur : choisi avec la commande de départ, `go g` ou `go d` (D020) | [ ] |
| ~~Départ provisoire (bouton BOOT de la carte)~~ | GPIO 0 | Sert seulement au téléversement | [x] T001 |
| Départ | — | Plus de bouton ni de tirette : commande `go g` ou `go d` tapée dans le moniteur, Wi-Fi ou USB (D013, D020) | [ ] |
| Lecture de l'état du BAU (optionnel) | à définir | Entrée seule conseillée (34, 36 ou 39), pont diviseur obligatoire | [ ] |

Broches libres : 2, 12, 14, 15, 21, 22, 23 (et les entrées seules 34, 36, 39). Pour un cordon de
départ (obligatoire à Eurobot), préférer 14, 21, 22 ou 23 : 2, 12 et 15 sont des broches de démarrage.

Anciens brochages, **obsolètes** :
- PCB « Test PCB 2026 5EO » (D015) : moteurs 32/33 et 19/18, encodeurs 25/26 et 5/17, TRIG commun 21,
  ECHO 2 et 15, interrupteur 27, I2C 22/23 ;
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

- [ ] Parasites des moteurs sur les encodeurs (T021, D022) : une résistance de 4,7 kΩ entre chaque fil
  d'encodeur (A et B, 4 fils) et le 3,3 V, un condensateur céramique de 100 nF entre les deux bornes de
  chaque moteur, fils d'encodeur éloignés des fils de moteur, fils de chaque moteur torsadés.
- [ ] Sens de comptage avec le brochage du 25/11/2026 (D018) : roue tournée vers l'avant à la main → son
  compteur doit augmenter (banc `test_odometrie`). Sinon, inverser `ENCODER_*_INVERTED` dans `config.h`.
- Les capteurs IR de sol ont été retirés (D012) : le PAMI trouve la douve par l'odométrie seule.

**Ultrasons HC-SR04 (2, D012, D018)**
- [ ] Alimentés en 5 V. Pont diviseur 1 kΩ / 2 kΩ sur chaque ECHO (obligatoire : l'ESP32 supporte 3,3 V).
- [ ] Gauche = TRIG 25 / ECHO 35, droit = TRIG 5 / ECHO 4 : une main devant le capteur gauche seul
  fait réagir G (banc `test_ultrason`). Sinon, échanger les broches dans `config.h`.
- [ ] Montés à moins de 70 mm de haut, pour voir les bordures de la table (règles 2027 : bordures de 70 mm).
- [ ] Chacun voit un obstacle placé devant lui seul (banc `test_ultrason`).
- [ ] Les deux émettent en même temps (choix du firmware, D018). Rien devant pendant 10 s → aucune mesure sous
  300 mm (un capteur n'entend pas le son de l'autre). Banc `test_ultrason`, protocole 6.

**Côté GAUCHE / DROITE (D017, D020)**
- Plus d'interrupteur : le côté se choisit avec la commande de départ. `go g` affiche
  `[COTE] GAUCHE (bord a gauche, centre a droite)`, `go d` affiche `[COTE] DROITE (bord a droite, centre a gauche)`.
  `go` seul est refusé.
- Côté selon l'écurie : on regarde où est le bord de la table quand le PAMI est posé, tourné vers
  l'adversaire. Ce n'est **pas** le côté vu du public (`docs/douves-coordonnees.md`, § 3) :

  | Écurie | Côté (commande) |
  |---|---|
  | Jaune avant | DROITE (`go d`) |
  | Jaune fond | GAUCHE (`go g`) |
  | Bleue avant | GAUCHE (`go g`) |
  | Bleue fond | DROITE (`go d`) |

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

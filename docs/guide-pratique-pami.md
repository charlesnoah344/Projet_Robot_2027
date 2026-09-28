# Guide pratique du PAMI : téléverser, câbler, tester

> À garder ouvert pendant les séances sur le vrai PAMI.
> Le brochage de référence reste `docs/materiel.md`. Ce guide ne fait que l'appliquer.
> Les broches ne sont pas encore validées : cochez la colonne « Validé » de `materiel.md` après chaque test réussi.

## 1. Avant de commencer
- VS Code avec l'extension PlatformIO. Les commandes `pio` se tapent dans le terminal PlatformIO.
  Sinon, utilisez le chemin complet : `~/.platformio/penv/Scripts/pio.exe`.
- Un câble USB **de données**. Certains câbles ne font que charger : aucun port COM n'apparaît.
- Si aucun port COM n'apparaît dans le Gestionnaire de périphériques de Windows, installez le pilote
  de la puce USB de la carte. C'est CP210x ou CH340 : le nom est écrit sur la petite puce près du
  connecteur USB.

## 2. Téléverser
Toutes les commandes se lancent depuis le dossier `pami/`.

| Je veux… | Commande |
|---|---|
| Compiler le firmware | `pio run` |
| Téléverser le firmware | `pio run -t upload` |
| Voir les messages de la carte | `pio device monitor -b 115200` (Ctrl+C pour quitter) |
| Téléverser un banc de test | `pio run -e test_moteurs -t upload` (remplacez par le nom du banc) |
| Lancer les tests de logique sur la carte | `pio test -e logic_esp32` |
| Lancer les tests de logique sur le PC | `pio test -e native` (g++ nécessaire) |

- **Fermez le moniteur série avant de téléverser.** Sinon : « port busy » ou « Access denied ».
- **Après un banc de test, re-téléversez toujours le firmware** (`pio run -t upload`).
  Un banc de test n'a pas forcément d'évitement.
- **Avec notre carte, il faut maintenir BOOT à chaque téléversement**, y compris avec `pio test` (T001).
  Dès que `Connecting...` s'affiche, maintenez BOOT jusqu'à l'apparition de `Writing at 0x...`,
  puis relâchez. Si vous oubliez, l'erreur est `Wrong boot mode detected (0x13)`.
  Correction matérielle possible si ça devient pénible : un condensateur de 10 µF entre EN et GND.
- Après le téléversement, appuyez sur EN (reset) pour voir les messages de démarrage.
  Avec `pio test`, appuyez sur EN si aucun résultat ne s'affiche dans les 10 s.

## 3. Câbler
**Règle n° 1 : on câble hors tension (USB débranché, alimentation de labo éteinte), et on vérifie
avant d'allumer.**

### Alimentation pendant les tests (en attendant la batterie)
```
Alim de labo (+) 6 V ──> BAU ──> MDD3A, bornier d'alimentation (+)
Alim de labo (−) ──────────────> MDD3A, bornier d'alimentation (−)
PC ──USB──> ESP32 ──┬── broche 5V (ou VIN) ──> encodeurs + HC-SR04
                    └── broche 3V3 ─────────> capteurs IR
Masse commune : GND ESP32 = GND MDD3A = (−) alim de labo = GND des capteurs
```
- Alimentation de labo réglée à **6 V**, limite de courant à **2 A environ** pour les premiers
  essais. Un moteur bloqué tire jusqu'à 2,8 A : la limite protège le montage.
- L'ESP32 est alimenté **seulement par l'USB**. On n'utilise pas encore la sortie 5 V du MDD3A.
- Le BAU coupe les moteurs, pas l'ESP32 (D003). Les capteurs restent donc alimentés.

### Tableau de câblage
| Composant | Fil | Va sur | Attention |
|---|---|---|---|
| MDD3A | entrées M1A / M1B | GPIO 25 / 26 | moteur gauche |
| MDD3A | entrées M2A / M2B | GPIO 33 / 32 | moteur droit |
| MDD3A | GND (côté commande) | GND de l'ESP32 | masse commune obligatoire |
| MDD3A | sorties moteur 1 / moteur 2 | moteur gauche / moteur droit | roue qui tourne à l'envers : on corrige dans `config.h` (étape 2), pas besoin de recâbler |
| Encodeurs (×2) | + / GND | 5V / GND | 4,5 à 7,5 V : **jamais en 3,3 V** |
| Encodeur gauche | A / B | GPIO 27 / 14 | **pas avant la mesure du § 4** |
| Encodeur droit | A / B | GPIO 13 / 4 | **pas avant la mesure du § 4** |
| HC-SR04 | VCC / GND | 5V / GND | |
| HC-SR04 | TRIG | GPIO 18 | branchement direct |
| HC-SR04 | ECHO | GPIO 19 **via le pont diviseur** | jamais en direct |
| IR avant-gauche | VCC / GND / OUT | 3V3 / GND / GPIO 34 | résistance de 100 kΩ entre GPIO 34 et GND |
| IR avant-droit | VCC / GND / OUT | 3V3 / GND / GPIO 35 | résistance de 100 kΩ entre GPIO 35 et GND |
| BAU | — | en série sur le (+) de l'alimentation du MDD3A | |

Les repères exacts des borniers sont écrits sur le MDD3A : suivez-les.

**Fils du moteur FIT0450.** D'après la fiche, il y a 6 fils : moteur +, moteur −, voie A, voie B,
GND encodeur, + encodeur. Vérifiez au multimètre : la FAQ DFRobot signale des étiquettes inversées
sur certaines cartes.

### Pont diviseur (5 V → 3,3 V)
```
Signal 5 V ──[ 1 kΩ ]──┬──> broche de l'ESP32
                       │
                    [ 2 kΩ ]
                       │
                      GND
```
Tension en sortie : 5 V × 2 / (1 + 2) = 3,3 V. Le couple 10 kΩ + 20 kΩ marche aussi.
**Mesurez la sortie au multimètre avant de la brancher : elle doit être au plus de 3,3 V.**

## 4. Mesurer avant de brancher
Ces 5 minutes de mesures peuvent sauver la carte.
1. **Signaux des encodeurs.**
   - Alimentez l'encodeur en 5 V, sans le relier à l'ESP32.
   - Tournez la roue lentement à la main.
   - Mesurez la voie A par rapport à GND.

   | Ce que vous mesurez | Ce qu'on fait |
   |---|---|
   | Ça alterne entre 0 et environ 5 V | pont diviseur sur A et sur B |
   | Ça alterne entre 0 et au plus 3,3 V | branchement direct |
   | Ça reste à 0 V | sortie « collecteur ouvert » : on activera un pull-up dans le code, prévenez-nous |
2. **Broche 5V (ou VIN) de l'ESP32 branché en USB.** Si elle mesure moins de 4,5 V, les encodeurs
   sont hors de leur plage de fonctionnement : signalez-le.
3. **Sortie de chaque pont diviseur** : au plus 3,3 V.
4. **Masses** : le multimètre en mode « bip » doit sonner entre le GND de l'ESP32, le GND du MDD3A
   et le (−) de l'alimentation de labo.

Notez ces mesures dans `docs/journal-tests.md`.

## 5. Tester
**Méthode** : un composant à la fois, avec son programme de test isolé. Le firmware complet vient ensuite.
1. Lisez l'en-tête du programme de test : montage, sécurité, protocole, critère de réussite.
2. Téléversez le programme, ouvrez le moniteur, puis tapez les commandes à une lettre.
   L'aide s'affiche au démarrage.
3. Notez les mesures pendant le test.
4. Consignez le résultat dans `docs/journal-tests.md` (modèle Txxx), ou envoyez-le pour qu'on le fasse.
5. Re-téléversez le firmware.

| Étape | Ce qu'on teste | Programme |
|---|---|---|
| 1 | Squelette : bouton BOOT, arrêt à 99,5 s | firmware (`esp32dev`) |
| 2 | Moteurs : sens, vitesse minimale, arrêt | `test_moteurs` |
| 3 | Encodeurs : ticks par tour, mm par tick | `test_encodeurs` |
| 4 | Ligne droite régulée sur 1 m | `test_ligne_droite` |
| 5 | Ultrason + arrêt devant un obstacle | `test_ultrason`, `test_arret_obstacle` |
| 6 | IR + virages | `test_ir`, `test_virage` |
| 7 | Machine à états complète | firmware (`esp32dev`) |

**Test de l'étape 1** (ESP32 seul en USB, rien d'autre de branché) :
- [ ] `pio test -e logic_esp32` affiche `7 test cases: 7 succeeded`.
- [ ] Au démarrage : avertissement « attente de 85 s DESACTIVEE », puis « Pret : appuyez sur BOOT ».
- [ ] Pendant 30 s sans toucher à rien, l'état reste `IDLE`.
- [ ] Un appui bref sur BOOT affiche `DEPART !`. Lancez le chronomètre en même temps.
- [ ] Un 2ᵉ appui sur BOOT ne fait rien.
- [ ] `FIN DU MATCH` apparaît entre 99,2 et 99,8 s au chronomètre.
- [ ] Toujours au moins 1000 tours/s. Refaites le tout 3 fois.

## 6. Points d'attention sur la carte réelle
**Ce qui peut détruire du matériel**
- Jamais plus de 3,3 V sur une broche de l'ESP32. Les risques : ECHO du HC-SR04, encodeurs en 5 V,
  IR alimentés en 5 V par erreur.
- Pas de court-circuit : câblez hors tension, isolez les fils dénudés, vérifiez avant d'allumer.
- Ne branchez ni ne débranchez jamais un fil sous tension.
- N'alimentez pas l'ESP32 par deux sources à la fois (USB + un 5 V extérieur).
- Le MDD3A est protégé contre l'inversion de polarité. L'ESP32 et les capteurs ne le sont pas.

**Ce qui empêche la carte de démarrer ou de téléverser**
- BOOT (GPIO 0) appuyé au moment du reset : la carte passe en mode téléversement et le programme
  ne tourne pas. C'est normal.
- GPIO 12 à l'état haut au démarrage empêche la carte de démarrer : ne branchez rien dessus.
- GPIO 6 à 11 (mémoire flash) et GPIO 1 et 3 (port USB) : ne branchez rien dessus.
- Moniteur série ouvert : téléversement impossible.

**Ce qui cause des comportements bizarres**
- **L'ESP32 redémarre quand les moteurs démarrent** : c'est une chute de tension, pas un bug.
  Pistes : limite de courant trop basse, masse mal serrée, condensateur à ajouter près du MDD3A,
  rampes d'accélération (étape 4).
- **Masse pas commune** : mesures aléatoires, moteurs qui ne répondent pas.
- **GPIO 34 et 35 n'ont pas de pull-up interne.** Sans la résistance de 100 kΩ, un IR débranché
  donne des valeurs au hasard. Avec elle, il lit « obstacle » : c'est voulu (sécurité par défaut).
- **IR** : une lumière forte peut les aveugler (les rencontres sont très éclairées), et les objets
  noirs sont mal vus. Réglez le potentiomètre, puis testez sous une lampe.
- **HC-SR04** :
  - il ne voit rien à moins de 2 ou 3 cm ;
  - il voit mal les objets en biais ou mous ;
  - il doit être monté à moins de 70 mm de haut pour voir les bordures.
- **Faux ticks d'encodeur** : éloignez les fils A/B des fils des moteurs, et torsadez chaque paire.

**Sécurité pendant les essais**
- Premiers essais moteurs **sur cales**, roues en l'air, à vitesse faible.
- BAU câblé et à portée de main. Le bouton de sortie de l'alimentation de labo sert de 2ᵉ coupure.
- **Boutons de test du MDD3A** : ils font tourner les moteurs directement, sans l'ESP32 ni `safety`.
  N'y touchez pas pendant un essai. Ils servent seulement à vérifier le câblage des moteurs, sur cales.
- Les LED du MDD3A montrent l'état des sorties moteur. C'est pratique pour vérifier une commande
  sans faire tourner les roues.
- Rien ne doit bouger à l'allumage. Si quelque chose bouge : BAU, puis on cherche pourquoi avant
  de continuer.

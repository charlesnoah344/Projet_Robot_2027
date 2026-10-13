# Plan du logiciel du PAMI défenseur (grille du Mechatronics Lab)

> Plan adopté le 09/10/2026. Il guide l'écriture du firmware du PAMI pour l'évaluation du labo.
> Le brochage de référence reste `docs/materiel.md` : ce plan propose, `materiel.md` décide.
> Rien de ce qui est décrit ici n'est validé tant que ce n'est pas testé sur le vrai PAMI (§ 9).

## Contexte
La grille d'évaluation du labo fixe ce que le PAMI doit savoir faire :
- se déplacer ;
- bien bouger : ligne droite, rotation, carré ;
- détecter les obstacles : arrêt, puis évitement, puis contournement ;
- respecter le temps : départ après 5 s, arrêt après 15 s ;
- arrêt d'urgence ;
- choix de la couleur ;
- rôle **défenseur** : toucher un adversaire, et atteindre une zone précisément.

L'équipe ne fait pas le rôle attaquant. Les tests sur le matériel sont confiés à un autre membre.

Choix de l'équipe :
- minuteur au **profil labo seulement** pour l'instant (5 s / 15 s) ;
- défenseur = **aller à une zone, puis bras servo** ;
- départ par un **bouton-poussoir externe sur GPIO 16**, remplacé plus tard par la tirette.
  Le bouton BOOT n'est pas accessible sur le PAMI monté ;
- encodeurs = **capteurs à effet Hall intégrés aux moteurs FIT0450**.

On garde la base existante : couches séparées, `loop()` non bloquante, chemin unique
`state_machine → motion → safety → motor_driver`, et la logique pure dans `logic/`.
Aujourd'hui, seuls le minuteur (T001) et le driver moteurs (T002, partiel) fonctionnent.

## 1. Lien entre la grille et le code

| Critère de la grille | Pts | Où dans le code | Matériel nécessaire |
|---|---|---|---|
| Se déplace | 1 | `motor_driver` (fait), `safety`, `motion` | ok |
| Ligne droite, rotation, carré | 2 | `encoders`, `motion` (régulation), `logic/odometry`, mission « carré » | ok |
| Obstacle : arrêt / évite / contourne | 3 | `sensors` (ultrason), `safety` (arrêt), états d'évitement | pont diviseur sur ECHO |
| Départ 5 s, arrêt 15 s | 1 | `match_timer`, état `WAIT_DELAY` | bouton-poussoir (GPIO 16) |
| Arrêt d'urgence | 1 | matériel (D003), lecture optionnelle sur GPIO 39 | BAU |
| Mécaniquement fermé | 2 | pas de code | — |
| Choix de la couleur | — | `color`, miroir dans `logic/mission_step.h` | interrupteur (GPIO 17) |
| Défenseur : atteindre une zone | 2* | `logic/odometry` + étapes `GOTO`, puis `REACH_ZONE` avec les capteurs de sol (D009) | 2 IR tournés vers le sol |
| Défenseur : toucher un adversaire | 2* | `arm` (servo SG90, GPIO 23) + état `DEFEND` | bras de couleur distincte |

\* La répartition 0/2/4 du rôle défenseur est à confirmer avec l'encadrant.

## 2. Démarrage : pourquoi un état WAIT_START
`WAIT_START` veut dire : « allumé, immobile, j'attends le signal de départ ».
Il sépare **l'allumage** du **départ**. C'est utile pour trois raisons :
1. **On a le temps de préparer.** On allume, on pose le PAMI, on choisit la couleur, puis on donne
   le départ quand on veut. Les 5 s ne démarrent qu'au signal.
2. **Un redémarrage ne relance jamais un départ.** Si l'ESP32 redémarre en plein essai
   (chute de tension au démarrage des moteurs), il revient en `WAIT_START` et reste immobile.
   Sans cet état, il repartirait seul 5 s plus tard.
3. **La couleur est lue pendant l'attente**, puis verrouillée au départ.
   Toucher l'interrupteur pendant le trajet ne change rien.

**Le signal : un bouton-poussoir sur GPIO 16.**
- On réutilise la logique déjà testée de `logic/start_button` (T001) :
  - le départ n'est armé qu'après avoir vu le bouton relâché ;
  - un appui doit durer au moins 50 ms ;
  - un seul départ par allumage.
- Plus tard, la tirette se branche **sur la même broche** avec la même logique :
  « bouton relâché » devient « tirette en place », « appui » devient « tirette retirée ».
  Il n'y aura qu'une ligne à changer dans `match_timer.cpp`.
- Bonus : une tirette absente ou un fil coupé à l'allumage empêche le départ d'être armé.
  C'est la sécurité par défaut.
- Le bouton BOOT (GPIO 0) ne sert plus au départ. Il ne sert plus qu'au téléversement.

## 3. Architecture cible

### Fichiers (tels qu'écrits le 09/10/2026)
```
src/
  main.cpp            setup() + loop() : minuteur, capteurs, couleur, machine à états, motion, safety, bras
  config.h            toutes les constantes (les valeurs non mesurées sont marquées PROVISOIRE)
  match_timer.*       départ par GPIO 16, isMoveAllowed() entre 5 s et 14,5 s, isOver() à 14,5 s
  encoders.*          encodeurs Hall des FIT0450, comptage par interruption (§ 4)
  sensors.*           ultrason (mesure par interruption, médiane de 3) + 2 capteurs de sol IR (D009)
  motion.*            driveDistance(mm), turnAngle(deg), stop(), régulation, odométrie, pose()
  safety.*            filtre réel : temps, panne, obstacle (ultrason), pas de recul
  color.*             lit le sélecteur, verrouille la couleur au départ
  arm.*               servo du bras, piloté par LEDC sur le canal 4 (D008), coupé à la fin
  mission.*           déroule la liste d'étapes de ACTIVE_MISSION, une à la fois
  state_machine.*     états ci-dessous
  logic/              logique pure, sans matériel (testable sur PC) :
    pami_state.h        liste des états
    match_clock.h       calculs du minuteur (isMatchOver, isInMovePhase)
    start_button.*      anti-rebond et armement du bouton de départ (T001)
    median3.h           médiane de 3 mesures ultrason
    safety_filter.h     règles de safety (obstacle avec marge, rotation sur place, pas de recul)
    speed_profile.h     profil de vitesse en trapèze (accélération, croisière, freinage)
    speed_controller.h  régulateur de vitesse d'une roue (anticipation + P + I)
    odometry.*          position (x, y, cap) à partir des ticks des roues
    mission_step.h      types d'étapes (GOTO, TURN_TO, DEFEND), couleur, miroir
    missions.h          les 4 missions, écrites pour le bleu
    next_move.h         choix du prochain mouvement : DONE, TURN ou DRIVE
    avoidance.h         côté du détour : gauche pour le bleu, droite pour le jaune
    zone_approach.h     bord de la zone : le sol a-t-il changé ? distance de recherche restante
```

### Machine à états
```
WAIT_START ──(bouton GPIO 16)──> WAIT_DELAY (immobile 5 s) ──> MISSION ──(arrivé à la zone)──> DEFEND (bras)
                                                                │  ^
                                                   obstacle     v  │ voie libre / détour fini
                                                            AVOID_STOP ──(toujours là)──> DETOUR

À tout moment : 14,5 s ──> END (moteurs freinés, servo détaché)       capteur en panne ──> FAULT
```
L'ancien état `IDLE` devient `WAIT_START`. `FORWARD`, `STOP`, `DECIDE`, `BACKUP` et `TURN`
(exploration) sont remplacés.

**Pourquoi des points de passage (GOTO x, y) plutôt que « avance 500 mm » ?**
- Le **contournement** devient presque gratuit. Après un détour, le PAMI recalcule le cap vers le
  même point et repart : il rejoint son trajet sans code spécial.
- **Atteindre une zone**, c'est atteindre le dernier point.
- Le **carré**, ce sont 4 points : (L, 0), (L, L), (0, L), (0, 0).
- **Jaune = miroir du bleu** : y devient −y et les angles changent de signe.
  Les trajets ne sont écrits qu'une fois (règle d'or n° 5).

**L'évitement en 3 niveaux**, chacun livrable seul :
1. **Arrêt** : `safety` freine si l'ultrason voit un obstacle sous `OBSTACLE_STOP_DISTANCE_MM`,
   quel que soit l'état.
2. **Évite** : si l'obstacle est encore là après `AVOID_WAIT_MS`, le PAMI passe en `DETOUR`.
   Il tourne de 90° (à gauche pour le bleu, à droite pour le jaune) et avance de `DETOUR_SIDE_MM`.
3. **Contourne** : retour à `MISSION`, qui vise de nouveau le même point.
   Le nombre de tentatives est limité.

**Atteindre la zone précisément (D009)** : les deux IR sont tournés vers le sol, à l'avant.
1. Le PAMI va au point d'approche, environ 10 cm avant la zone, grâce à l'odométrie.
2. Il se tourne vers la zone, note ce que voient les capteurs de sol, puis avance lentement (80 mm/s).
3. Dès que l'un des deux capteurs voit autre chose, il est au bord : il freine, avance encore de
   `ZONE_ENTRY_MM` (50 mm), et s'arrête. Sans bord vu après `ZONE_SEARCH_MAX_MM`, il s'arrête quand même.
- On compare au sol de départ : ça marche que la zone soit plus claire ou plus foncée que le sol.
- Le repère est le vrai bord de la zone, pas une position estimée : c'est plus précis que l'odométrie seule.

**Pas de marche arrière dans le firmware de match**, et `safety` la refuse.
- Nos capteurs ne regardent que vers l'avant.
- La grille et le règlement exigent une détection « dans tous les sens de déplacement possibles ».
- Tourner sur place reste permis : les capteurs balaient pendant la rotation.

**Le bras défenseur et l'évitement ne se contredisent pas.**
- L'évitement garde le **châssis** loin de l'adversaire ; c'est le **bras** qui le touche.
- Contrainte méca : la grille autorise 10 cm de périmètre en plus une fois déployé.
  Le bras ne peut donc dépasser que d'environ 50 mm.
- En `DEFEND`, le PAMI est immobile. Le bras se déploie quand l'ultrason mesure un adversaire à moins
  de `ARM_TRIGGER_DISTANCE_MM`, puis il reste déployé.
- À `END`, le servo est détaché : plus de PWM.

### Minuteur labo (décision D007)
- `MATCH_MOVE_ALLOWED_MS = 5000` : immobile 5 s après l'appui sur le bouton.
- `MATCH_STOP_MS = 14500` : arrêt total, avec 0,5 s de marge.
- Les deux durées comptent depuis l'appui.
- Valeurs Eurobot (85 300 / 99 500) en commentaire, pour plus tard.
- **Seulement 9,5 s de mouvement.** Le côté du carré et la distance jusqu'à la zone sont des
  constantes de `config.h`, choisies d'après la vitesse réelle mesurée.
- `safety` refuse toute commande moteur avant 5 s et après 14,5 s : c'est une deuxième barrière.

### Choix de la mission
- `config.h` contient `ACTIVE_MISSION` : `MISSION_SQUARE` (démonstration « carré ») ou
  `MISSION_DEFENDER`.
- Pour changer de mission, on modifie une ligne et on recompile.
- Pourquoi : c'est simple, et on ne peut pas se tromper de mission sur la table.

## 4. Les encodeurs à effet Hall des FIT0450
**Principe.**
- Un disque aimanté tourne avec l'arbre du moteur, devant deux capteurs à effet Hall.
- Chaque capteur donne un signal carré : les voies A et B.
- Les deux capteurs sont décalés d'un quart de période (signaux « en quadrature »).
  Selon que A change avant ou après B, on connaît le **sens** de rotation.

**Lecture dans le code** (D004, déjà décidé) :
- une interruption sur **chaque front de A** (montant et descendant) ;
- à chaque front, on lit B : si A == B on compte dans un sens, sinon dans l'autre ;
- 960 impulsions par tour de roue et par voie (fiche), donc **1920 ticks par tour** avec 2 fronts.
  À vérifier en tournant la roue 10 fois à la main ;
- débit maximal : 160 tr/min ≈ 2,7 tr/s, soit environ 5 100 interruptions/s par roue.
  C'est facile pour l'ESP32 ;
- fonction d'interruption courte, marquée `IRAM_ATTR`. Compteurs `volatile`, lus en section
  critique pour ne pas lire une valeur à moitié écrite ;
- plan B si des ticks se perdent : le compteur matériel PCNT de l'ESP32.

**Ce qui est propre à l'effet Hall :**
- pas de rebond de contact, contrairement à un encodeur mécanique : pas d'anti-rebond logiciel ;
- en revanche, les fils des moteurs rayonnent des parasites magnétiques et électriques.
  On torsade chaque paire A/B avec sa masse et on l'éloigne des fils moteurs ;
- beaucoup de capteurs Hall ont une sortie « collecteur ouvert » : ils tirent vers 0 V et ont besoin
  d'une résistance de rappel. Sur certaines cartes, elle est intégrée et tire vers le 5 V.
  D'où la **mesure obligatoire avant branchement** (`docs/guide-pratique-pami.md`, § 4) :
  - le signal monte à 5 V → pont diviseur (1 kΩ / 2 kΩ) sur A et sur B ;
  - le signal reste à 0 V → pull-up interne de l'ESP32 (`INPUT_PULLUP`), branchement direct.

## 5. Schéma de câblage (PAMI sur batterie)
Proposition fondée sur `docs/materiel.md`. À valider broche par broche après chaque test.

### Alimentation
```
                       ┌──> Régulateur 5 V (abaisseur, ≥ 2 A) ──┬──> ESP32, broche 5V/VIN
                       │                                        ├──> HC-SR04 (VCC)
Batterie ──> Interrupteur général ┤                             ├──> Encodeurs gauche + droit (+)
                       │                                        └──> Servo SG90 (rouge) + condensateur 470 µF
                       │
                       └──> BAU (contact NC) ──> MDD3A VB+ ──> moteurs
                                                 MDD3A VB− ──> masse

ESP32, broche 3V3 ──> capteurs de sol IR (VCC)
MASSE COMMUNE : batterie (−) = régulateur = MDD3A VB− et GND = ESP32 GND = tous les capteurs
```
- **Pourquoi deux branches ?** Le BAU coupe seulement les moteurs (D003, règle d'or n° 2).
  L'ESP32 reste allumé et ne perd pas son état.
- **Batterie** (pièce manquante) : le MDD3A accepte 4 à 16 V.
  - Avec 2 éléments LiPo ou LiFePO4, ou 6 NiMH, on obtient environ 7 à 8,4 V.
  - Il faut alors limiter la PWM vers 70 % pour rester vers 6 V moyens sur les FIT0450
    (`MOTOR_MAX_PWM_PCT`).
  - LiPo : sac ignifuge et chargeur à présenter (grille, ligne batteries).
- **Régulateur 5 V séparé** : la sortie 5V0 du MDD3A ne suffit pas pour le servo. Un SG90 qui
  force tire plusieurs centaines de mA ; une chute de tension ferait redémarrer l'ESP32.
- **Ne jamais brancher l'USB pendant que le régulateur alimente l'ESP32** : ce seraient deux
  sources à la fois. Pour lire les messages série sur batterie, débranchez d'abord le fil 5 V du
  régulateur vers l'ESP32.
- Condensateurs : 470 µF près du servo, 100 µF près de VB+/VB− du MDD3A. Ils absorbent les pics de
  courant.

### Signaux
```
                      ESP32
        ┌──────────────────────────────┐
MDD3A M1A ◄── GPIO 25          GPIO 34 ◄── OUT IR de sol gauche (+ 100 kΩ vers GND)
MDD3A M1B ◄── GPIO 26          GPIO 35 ◄── OUT IR de sol droit  (+ 100 kΩ vers GND)
MDD3A M2A ◄── GPIO 33          GPIO 18 ──► TRIG HC-SR04
MDD3A M2B ◄── GPIO 32          GPIO 19 ◄── ECHO HC-SR04 via pont 1 kΩ / 2 kΩ
Enc. G  A ──► GPIO 27 (*)      GPIO 23 ──► signal servo SG90 (orange)
Enc. G  B ──► GPIO 14 (*)      GPIO 16 ◄── bouton de départ ──> GND   (INPUT_PULLUP)
Enc. D  A ──► GPIO 13 (*)      GPIO 17 ◄── interrupteur couleur ──> GND (INPUT_PULLUP)
Enc. D  B ──► GPIO 4  (*)      GPIO 39 ◄── (optionnel) VB+ après le BAU, via 10 kΩ / 4,7 kΩ
        └──────────────────────────────┘
(*) pont diviseur 1 kΩ / 2 kΩ si le signal monte à 5 V (mesure du § 4)
```

| Composant | Fil | Va sur | Pourquoi / attention |
|---|---|---|---|
| Bouton de départ | une patte / l'autre | GPIO 16 / GND | Pull-up interne : relâché = 1, appuyé = 0. Pas de résistance à ajouter. Placé loin des roues. |
| Interrupteur couleur | commun / contact | GPIO 17 / GND | Ouvert = bleu, fermé = jaune (à noter sur le PAMI). Lu seulement avant le départ. |
| Servo SG90 | marron / rouge / orange | GND / 5 V du régulateur / GPIO 23 | Jamais sur le 3V3 de l'ESP32. Le signal 3,3 V suffit en général au SG90 : à vérifier au test. |
| Lecture BAU (optionnel) | VB+ côté MDD3A | pont 10 kΩ / 4,7 kΩ → GPIO 39 | 8,4 V donne 2,7 V : sans danger pour l'ESP32. Permet d'afficher « BAU appuyé » dans les logs. |
| Encodeurs, HC-SR04, IR, MDD3A | — | comme dans `docs/guide-pratique-pami.md`, § 3 | inchangé |

Pièces à ajouter dans `docs/materiel.md` :
- un bouton-poussoir ;
- un régulateur 5 V (2 A ou plus) ;
- des condensateurs de 470 µF et 100 µF ;
- les résistances des ponts diviseurs.

## 6. Ordre de travail
Une étape = un petit changement, compilé, commité, puis testé sur le vrai PAMI par le membre chargé
des tests. L'équipe a choisi d'écrire toutes les étapes d'abord et de tester ensuite (§ 7) :
le testeur suit donc les étapes dans l'ordre du tableau, et on corrige au fur et à mesure.
L'ultrason et `safety` passent **avant** la régulation. Aujourd'hui `safety` freine toujours :
le firmware ne peut pas faire rouler le PAMI tant qu'il n'a pas de capteur d'obstacle. Et rouler sans
capteur est interdit (règle d'or n° 1).

| # | Étape | Fichiers principaux | Point de la grille |
|---|---|---|---|
| A | Départ GPIO 16, minuteur labo 5 s / 14,5 s, `WAIT_DELAY`, D007 | `config.h`, `match_timer.*`, `state_machine.*`, `logic/pami_state.h`, `logic/match_clock.h` | Temps |
| B | Encodeurs Hall (interruptions, mm parcourus) | `encoders.*`, `config.h` | — |
| C | Ultrason + vrai filtre `safety` (arrêt, temps, pas de recul) | `sensors.*`, `safety.*` | Obstacle : arrêt |
| D | `motion` : vitesse régulée, ligne droite, rotation mesurée | `motion.*`, `logic/` (régulateur, rampe) | Se déplace, ligne droite, rotation |
| E | Odométrie + étapes de mission + mission « carré » | `logic/odometry`, `logic/mission_step.h`, `mission.*` | Carré |
| F | `AVOID_STOP` → `DETOUR` → retour à la mission (ultrason seul, D009) | `logic/avoidance.h`, `state_machine.*` | Évite, contourne |
| G | Sélecteur de couleur + miroir | `color.*`, `logic/mission_step.h` | Couleur |
| H | Mission défenseur : approche, bord de zone vu par les capteurs de sol, bras (`DEFEND`) | `arm.*` (D008), `mission.*`, `logic/zone_approach.h` (D009) | Défenseur |
| I | (Optionnel) Lecture de l'état du BAU sur GPIO 39 | `safety.*` | Arrêt d'urgence (le vrai arrêt est matériel) |

Remarques :
- Le carré sert aussi à **calibrer** `MM_PER_TICK` et `WHEEL_BASE_MM` : on mesure l'écart entre le
  point d'arrivée et le départ. C'est ce qui permet ensuite d'« atteindre une zone précisément ».
- La logique de `logic/` reste sans accès au matériel. Le membre chargé des tests pourra la tester
  sur PC (`pio test -e native`).

## 7. Exécution après accord : toutes les étapes, sans attendre les tests
Demande de l'équipe : enchaîner les étapes A à H sans attendre les tests sur le matériel
(un autre membre s'en charge). Règles appliquées pour que ça reste sûr :
- **un commit par étape**, et chaque étape compile (`pio run`) avant le commit.
  Les tests de `logic/` existants doivent compiler aussi (`pio test -e logic_esp32 --without-uploading
  --without-testing`, car g++ n'est pas installé sur ce PC) ;
- **rien n'est coché** dans `pami/CLAUDE.md`. Une fonction n'est pas « validée » tant qu'elle n'a pas
  été testée sur le vrai PAMI ;
- **valeurs non mesurées = prudentes** et marquées `// PROVISOIRE, à mesurer` dans `config.h` :
  vitesse faible, seuil d'arrêt large, `MM_PER_TICK` et `WHEEL_BASE_MM` estimés.
  Le réglage des encodeurs (pull-up ou non) attend la mesure du § 4 : une constante, false par défaut ;
- **l'évitement n'est jamais désactivable**, et les commandes moteurs passent toujours par `safety` ;
- à la fin : un tableau « à tester » par étape, dans `docs/plan-logiciel-pami.md`, pour le testeur ;
- l'étape I (lecture du BAU) n'est pas faite : le pont diviseur n'est pas câblé, et une entrée en l'air
  donnerait des valeurs au hasard.
- `test/test_match_timer` : le test qui vérifie 99 500 ms utilisera `MATCH_STOP_MS`, sinon il
  échouerait avec le profil labo.

Détail des deux premiers commits (les suivants suivent le tableau du § 6) :

**Commit 1 — `docs: plan du logiciel du PAMI défenseur et schéma de câblage`**
- `docs/plan-logiciel-pami.md` : ce plan.
- `docs/materiel.md` :
  - GPIO 16 devient « départ : bouton externe provisoire, puis tirette » ;
  - GPIO 0 ne sert plus au départ ;
  - nouvelles pièces à acheter (bouton, régulateur 5 V, condensateurs).
- `CLAUDE.md` (racine) : le nouveau fichier ajouté à la carte du projet.

**Commit 2 — `feat: départ par bouton GPIO 16 et minuteur profil labo (5 s / 14,5 s)`**
- `config.h` :
  - `PIN_START_BUTTON = 16` (au lieu de 0) ;
  - `MATCH_MOVE_ALLOWED_MS = 5000` et `MATCH_STOP_MS = 14500` ;
  - valeurs Eurobot en commentaire ;
  - `PIN_PULL_CORD` supprimée, puisque c'est la même broche.
- `logic/match_clock.h` : `isMoveAllowed(startMs, nowMs)`.
- `match_timer.*` : lecture de GPIO 16 et `matchTimer::isMoveAllowed()`.
- `logic/pami_state.h` : `WAIT_START`, `WAIT_DELAY`, `MISSION`, `AVOID_STOP`, `DETOUR`, `DEFEND`,
  `END`, `FAULT`.
- `state_machine.cpp` : `WAIT_START → WAIT_DELAY → MISSION`, et `END` prioritaire à 14,5 s.
- `main.cpp` : messages de démarrage mis à jour.
- `docs/decisions.md` : D007 « Profil labo du minuteur et bouton de départ externe ».
  D005 est marquée « remplacée par D007 ».
- `pami/CLAUDE.md` : bandeau d'exception, états et ordre des étapes mis à jour.
- `docs/guide-pratique-pami.md` : test de l'étape 1 réécrit (bouton GPIO 16, 5 s, 14,5 s).

Les moteurs restent freinés par `safety` pendant toute l'étape A : aucun mouvement possible.
Ils ne pourront tourner qu'à partir de l'étape C, quand l'ultrason et le vrai filtre de sécurité
existeront.

## 8. Vérification
- Après chaque étape : `cd pami && pio run` compile sans erreur ni nouvel avertissement.
- Tests sur le PAMI, faits plus tard par le membre chargé des tests, étape par étape (A d'abord,
  sur cales pour C et D). Exemple pour l'étape A, au moniteur série et au chronomètre :
  - au démarrage : `WAIT_START`, et rien ne se passe pendant 30 s ;
  - appui sur le bouton → `WAIT_DELAY` ;
  - `MISSION` à 5,0 s (± 0,1 s) ;
  - `END` à 14,5 s (± 0,1 s) ;
  - un 2ᵉ appui ne fait rien ;
  - bouton maintenu enfoncé pendant l'allumage → aucun départ ;
  - 3 redémarrages sans départ spontané.
  Résultat à consigner dans `docs/journal-tests.md` (T003).

## 9. À tester sur le vrai PAMI (pour le membre chargé des tests)
Tout le code des étapes A à H est écrit et compile. **Rien n'est encore validé.**
Suivre les étapes dans l'ordre. Chaque résultat va dans `docs/journal-tests.md`, et chaque valeur
mesurée remplace la valeur `PROVISOIRE` correspondante dans `config.h`.
Moniteur série : une ligne d'état par seconde (`[MAIN] ... etat ... US ... mm | ticks G ... D ...`).

| # | Montage | Ce qu'on vérifie | Réglages dans `config.h` |
|---|---|---|---|
| A | ESP32 seul + bouton GPIO 16 | Voir § 8 et le guide pratique, § 5 | — |
| B | Encodeurs branchés **après la mesure du guide, § 4**. Roues à la main, sans moteurs | Pousser le PAMI en avant : les deux compteurs augmentent. 10 tours de roue ≈ 19 200 ticks | `ENCODER_USE_INTERNAL_PULLUP`, `ENCODER_*_INVERTED`, `ENCODER_TICKS_PER_WHEEL_TURN` |
| C | + HC-SR04 (pont diviseur sur ECHO) | Distance affichée juste (règle à 100, 300, 1000 mm). Débrancher ECHO → `FAULT` en 1 s. Main devant pendant le mouvement → arrêt. Mesurer la distance de freinage | `OBSTACLE_STOP_DISTANCE_MM` (distance de freinage + marge) |
| D | PAMI **sur cales**, puis au sol. `ACTIVE_MISSION = STRAIGHT_LINE` | Roues au même régime, pas de `FAULT` au démarrage, arrêt progressif. Au sol : 1 m parcouru, écart latéral mesuré | `MOTOR_DEADBAND_PCT`, `MOTOR_PCT_PER_MM_S`, gains `SPEED_*`, `HEADING_KP_MM_S_PER_MM`, `WHEEL_DIAMETER_MM` |
| D | `ACTIVE_MISSION = ROTATION` | 4 quarts de tour, retour au cap de départ (marque au sol) | `WHEEL_BASE_MM` |
| E | `ACTIVE_MISSION = SQUARE` | Retour au point de départ : noter l'écart en x, en y et en cap. Tient dans les 9,5 s ? | `WHEEL_DIAMETER_MM`, `WHEEL_BASE_MM`, `SQUARE_SIDE_MM` |
| F | STRAIGHT_LINE avec une boîte au milieu | Boîte posée puis retirée en moins de 1 s : arrêt puis reprise. Boîte laissée : détour, puis retour vers le point | `AVOID_WAIT_MS`, `DETOUR_SIDE_MM` |
| G | + interrupteur GPIO 17 | La couleur s'affiche au démarrage et suit l'interrupteur jusqu'au départ, puis ne change plus. Jaune : le carré tourne de l'autre côté | — |
| H | + capteurs de sol, servo et bras, `ACTIVE_MISSION = DEFENDER` | `sol G` / `sol D` changent au passage sol → zone. `Bord de la zone`, puis arrêt 50 mm plus loin. Aucun mouvement du servo avant la zone. Main à moins de 60 mm → bras déployé. À 14,5 s : `[ARM] Servo coupe` | potentiomètre des IR, `DEFENDER_APPROACH_*`, `ZONE_*`, `ARM_*_DEG`, `ARM_TRIGGER_DISTANCE_MM` |

**À chaque essai où les moteurs tournent** : BAU à portée de main, premier essai de chaque étape sur
cales, vitesse prudente (`DRIVE_SPEED_MM_S = 200` au départ).

### Limites connues de cette première version
- **Les valeurs `PROVISOIRE` sont des estimations.** Avec une zone morte ou un diamètre de roue faux,
  le PAMI peut partir en `FAULT` (roue « bloquée ») ou mal mesurer ses distances. C'est voulu : il
  s'arrête plutôt que de rouler n'importe comment.
- **Ultrason seul pour l'évitement (D009)** : les angles avant ne sont plus surveillés. Un obstacle
  étroit, de biais ou sur le côté peut ne pas être vu. Risque à l'homologation (« couverture
  suffisante ») : parade possible, un 2ᵉ ultrason.
- **Détour** : le côté est fixe (gauche pour le bleu). Les détours sont limités à 3 par essai.
- **Approche de la zone** : si le PAMI doit faire un détour pendant l'approche, il peut repartir
  au-dessus d'un sol différent et « voir » un faux bord.
- **Zone près d'un bord** : le seuil d'arrêt (150 mm) empêche d'approcher un mur à moins de 150 mm.
  Si la zone est contre une bordure, il faudra réduire le seuil pendant l'approche finale, avec une
  vitesse plus basse. On ne coupera jamais la détection (règle d'or n° 1).
- **Le bras reste déployé** une fois sorti. Un balayage gauche-droite pourra venir ensuite si le
  contact n'est pas assez sûr.
- **Odométrie seule** : les glissements de roue s'accumulent. Le gyroscope (optionnel, D004)
  pourra corriger le cap plus tard.
- **Lecture de l'état du BAU (étape I)** : pas faite. Le pont diviseur n'est pas câblé.

## Questions pour l'encadrant du labo
1. Les « 15 s » comptent-elles depuis le départ, ou depuis la fin des 5 s d'attente ?
2. Défenseur « 0/2/4 » : toucher un adversaire = 2 et atteindre la zone = 4, ou 2 points chacun ?
3. Où est la zone à atteindre, et sur quelle table a lieu l'évaluation ?
   Il faut le savoir pour les coordonnées et l'axe du miroir.
4. « Contourne » : faut-il reprendre le trajet d'origine après l'obstacle, ou suffit-il de passer à côté ?

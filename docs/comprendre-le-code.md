# Comprendre le code du PAMI

> Ce document explique le firmware du PAMI fichier par fichier : à quoi sert chaque module, comment il
> marche, ce qu'il affiche dans le moniteur série, et quoi vérifier quand il ne fait pas ce qu'on attend.
> But : pouvoir trouver un bug soi-même, comme le problème de masse commune du 13/10/2026.
>
> **Pour s'en servir face à un bug** : commencer par le § 6 (symptôme → module), puis lire la section
> du module concerné. Le code fait foi : si ce document et le code divergent, c'est le code qui a raison,
> et il faut corriger ce document.
>
> À jour au 15/10/2026 (firmware défenseur, profil labo D007, IR tournés vers le sol D009).
> **L'avant du PAMI est le côté de l'ultrason.** Gauche et droite se voient depuis l'arrière, en
> regardant vers l'ultrason.

## 1. Les cinq idées à connaître avant de lire le code

### 1.1 La boucle ne s'arrête jamais
L'ESP32 exécute `setup()` une fois, puis `loop()` en boucle, environ **180 000 fois par seconde**.
- **Aucune fonction n'attend** : pas de `delay()`. Chaque module fait un petit travail et rend la main.
  Sinon, l'évitement réagirait en retard.
- **Pour faire quelque chose « toutes les 20 ms »**, on compare l'heure actuelle (`millis()`) avec
  l'heure de la dernière fois :
  ```cpp
  if (nowMs - lastUpdateMs < MOTION_PERIOD_MS) {
    return;  // pas encore l'heure
  }
  lastUpdateMs = nowMs;
  ```
  On écrit toujours une **soustraction** (`nowMs - lastUpdateMs`). Elle reste juste quand `millis()`
  repasse par zéro, au bout de 49 jours.
- Le moniteur affiche `[MAIN] … tours/s`. **Si ce nombre tombe sous 1 000**, quelque chose bloque la boucle.

### 1.2 Des couches, et un seul chemin vers les moteurs
Chaque fichier a **une seule responsabilité**. Les ordres de mouvement suivent toujours le même chemin :
```
state_machine  →  mission  →  motion  →  safety  →  motor_driver  →  MDD3A  →  moteurs
  (que faire)     (où aller)  (comment)  (a-t-on le droit ?)  (signaux PWM)
```
**Seul `safety` appelle `motor_driver`.** Ainsi, aucun bug ailleurs ne peut faire rouler le PAMI sans
passer par les vérifications de sécurité (temps de match, panne, obstacle, recul interdit).

### 1.3 Toutes les constantes sont dans `config.h`
Broches, vitesses, seuils, durées : tout est dans `pami/src/config.h`, avec l'unité dans le nom
(`_MM`, `_MS`, `_PCT`, `_DEG`…). **Pour régler le PAMI, on ne touche qu'à ce fichier.**
Les valeurs marquées `PROVISOIRE` n'ont pas encore été mesurées sur le vrai PAMI.

### 1.4 Le dossier `logic/` ne touche jamais au matériel
Les fichiers de `pami/src/logic/` ne contiennent que des calculs : filtres, régulateur, odométrie,
décisions. Ils ne lisent aucune broche. On peut donc les tester sur PC. Les fichiers de `pami/src/`
(hors `logic/`) font le lien avec le matériel et appellent ces calculs.

### 1.5 Les interruptions
Certains signaux sont trop rapides pour être lus par la boucle : les ticks des encodeurs (jusqu'à
plusieurs milliers par seconde) et l'écho de l'ultrason (à la microseconde près). Pour eux, on utilise
une **interruption** : l'ESP32 met la boucle en pause pendant quelques microsecondes, exécute une petite
fonction (marquée `IRAM_ATTR`), puis reprend.
- Les variables modifiées dans une interruption sont `volatile` (le compilateur doit les relire à
  chaque fois).
- On les lit dans une **section critique** (`portENTER_CRITICAL` … `portEXIT_CRITICAL`) : une
  interruption ne peut pas les modifier pendant la lecture.
- **On n'écrit jamais `Serial.print` dans une interruption.**

## 2. Un tour de boucle, et le trajet d'un ordre de mouvement

### 2.1 Ce que fait `loop()` à chaque tour (`main.cpp`)
```
nowMs = millis()
1. matchTimer::update     lit le bouton de départ, suit le temps (5 s, 14,5 s)
2. sensors::update        déclenche l'ultrason, convertit l'écho, lit les capteurs de sol
3. color::update          lit le sélecteur de couleur (jusqu'au départ)
4. stateMachine::update   décide quoi faire (attendre, rouler, éviter, défendre, s'arrêter)
5. motion::update         toutes les 20 ms : régule la vitesse des roues, met à jour la position
6. safety::apply          vérifie la commande et l'envoie au driver (ou freine)
7. arm::update            coupe le servo dès la fin du temps
   logStatus              une ligne [MAIN] par seconde
```
L'ordre compte : le minuteur passe en premier, pour que la fin du temps soit vue avant toute décision.

### 2.2 Exemple : du bouton de départ à la roue qui tourne
1. On appuie sur le bouton (GPIO 16) → `match_timer` valide l'appui → `isStarted()` devient vrai.
2. `state_machine` passe de `WAIT_START` à `WAIT_DELAY` et verrouille la couleur.
3. À 5 s, `isMoveAllowed()` devient vrai → `state_machine` appelle `mission::start()`, passe en `MISSION`.
4. `mission::update()` regarde la première étape (par exemple « aller au point (500, 0) ») et la
   position actuelle. `next_move.h` décide : « avancer de 500 mm » → `motion::driveDistance(500)`.
5. Toutes les 20 ms, `motion::update()` :
   - lit les ticks des encodeurs et calcule la vitesse de chaque roue ;
   - calcule la vitesse voulue (accélération progressive, puis croisière, puis freinage) ;
   - corrige l'écart entre les deux roues pour aller droit ;
   - transforme chaque vitesse voulue en pourcentage de PWM (régulateur) → `motion::command()`.
6. `safety::apply()` reçoit cette commande. Si le temps le permet, pas de panne, pas d'obstacle,
   pas de recul : il l'envoie à `motorDriver::setWheelPercent()`. Sinon : frein.
7. `motor_driver` règle les signaux PWM sur GPIO 25/26 (roue gauche) et 33/32 (roue droite).
8. Le MDD3A transforme ces signaux en courant pour les moteurs, pris sur **son alimentation
   (VB+ / VB−)**.
9. La roue tourne → l'encodeur envoie des fronts → l'interruption compte les ticks → retour à l'étape 5.

**Chaque maillon de cette chaîne peut casser.** Le § 6 dit comment trouver lequel.

## 3. Carte des fichiers

| Fichier (`pami/src/`) | Couche | Rôle en une phrase | Matériel | Préfixe des logs |
|---|---|---|---|---|
| `main.cpp` | orchestre | Appelle chaque module dans l'ordre, affiche l'état chaque seconde | — | `[MAIN]` |
| `config.h` | réglages | Toutes les constantes | — | — |
| `motor_driver.*` | bas niveau | Transforme « +50 % » en signaux PWM pour le MDD3A | GPIO 25, 26, 33, 32 | — |
| `encoders.*` | capteur | Compte les ticks de chaque roue | GPIO 27, 14, 13, 4 | — |
| `sensors.*` | capteur | Ultrason (distance devant) et capteurs de sol | GPIO 18, 19, 34, 35 | `[SENS]` |
| `safety.*` | sécurité | Dernier contrôle avant les moteurs | — | `[SAFE]` |
| `match_timer.*` | temps | Départ, 5 s d'attente, arrêt à 14,5 s | GPIO 16 | `[TIME]` |
| `color.*` | réglage match | Lit le sélecteur bleu / jaune | GPIO 17 | `[COUL]` |
| `motion.*` | mouvement | Fait avancer ou tourner de façon mesurée, suit la position | — | `[MOT]` |
| `mission.*` | stratégie | Déroule les étapes de la mission choisie | — | `[MISS]` |
| `state_machine.*` | décision | Choisit l'état du PAMI (attente, mission, évitement…) | — | `[FSM]` |
| `arm.*` | actionneur | Bras du défenseur (servo) | GPIO 23 | `[ARM]` |
| `logic/*` | calculs purs | Filtres, régulateur, odométrie, décisions (détail au § 5) | aucun | — |

## 4. Les modules un par un

### 4.1 `main.cpp` : le chef d'orchestre
- **`setup()`** :
  1. `motorDriver::begin()` **en tout premier**, pour que les moteurs soient freinés avant toute chose ;
  2. ouverture du port série et messages de démarrage ;
  3. `begin()` de chaque module.
- **`loop()`** : voir § 2.1.
- **`logStatus()`** : une ligne par seconde.
  ```
  [MAIN] 180000 tours/s | etat WAIT_START | t = 0.0 s | US 2000 mm | ticks G 0 D 0 | sol G 1 D 1
  ```
  - `tours/s` : santé de la boucle (doit rester au-dessus de 1 000) ;
  - `etat` : l'état de la machine à états ;
  - `t` : le temps depuis le départ ;
  - `US` : la distance filtrée devant (2000 = rien dans la portée, 0 = pas encore mesuré) ;
  - `ticks G / D` : les compteurs des encodeurs ;
  - `sol G / D` : les capteurs de sol (1 = surface claire, 0 = surface foncée).
- **Si ça ne marche pas** : rien ne s'affiche → vérifier `pio device monitor`, appuyer sur EN.
  Le moniteur affiche `waiting for download` → BOOT était enfoncé au démarrage (voir
  `docs/lecons-apprises.md`).

### 4.2 `config.h` : les réglages
Organisé en blocs : journal série, broches, moteurs, encodeurs, mouvement, missions, ultrason et
sécurité, évitement, bras, couleur, minuteur, bancs de test.

Les constantes qui changent le plus souvent le comportement :

| Constante | Effet | Valeur au 13/10 |
|---|---|---|
| `ACTIVE_MISSION` | La mission jouée par le firmware de match | `STRAIGHT_LINE` |
| `WHEEL_DIAMETER_MM` | Distance pour 1 tick → toutes les distances | 60 |
| `WHEEL_BASE_MM` | Écart entre les roues → tous les angles | 100 |
| `ENCODER_LEFT_INVERTED` / `ENCODER_RIGHT_INVERTED` | Sens de comptage de chaque encodeur | `false` / `true` |
| `MOTOR_LEFT_INVERTED` / `MOTOR_RIGHT_INVERTED` | Sens de rotation de chaque moteur | `true` / `true` |
| `DRIVE_SPEED_MM_S`, `TURN_SPEED_MM_S` | Vitesses de croisière | 150, 90 |
| `OBSTACLE_STOP_DISTANCE_MM` | Distance d'arrêt devant un obstacle | 150 |
| `MATCH_MOVE_ALLOWED_MS`, `MATCH_STOP_MS` | Départ et fin (profil labo) | 5000, 14500 |
| `DEBUG_LOGS` | Active les messages du moniteur | `true` |

**Après toute modification de `config.h`, il faut recompiler et téléverser.**

### 4.3 `motor_driver.*` : parler au MDD3A
**Rôle** : transformer une commande en pourcentage (−100 à +100 pour chaque roue) en signaux PWM.

| Fonction | Ce qu'elle fait |
|---|---|
| `begin()` | Met les 4 broches à l'état bas (frein), puis les confie au générateur PWM de l'ESP32 (LEDC, 20 kHz, 10 bits) |
| `brake()` | Les 4 sorties à 0 : frein |
| `setWheelPercent(gauche, droite)` | Applique la commande aux deux roues |

**Comment ça marche** : le MDD3A a 2 entrées par moteur (D006).
| Entrée A | Entrée B | Le moteur… |
|---|---|---|
| PWM | 0 | avance |
| 0 | PWM | recule |
| 0 | 0 | freine |

- Roue gauche : A = GPIO 25 (M1A), B = GPIO 26 (M1B). Roue droite : A = GPIO 33 (M2A), B = GPIO 32 (M2B).
- La PWM : le signal est haut pendant une partie de chaque cycle de 50 µs (20 kHz). +50 % = haut la
  moitié du temps. Le moteur « voit » en moyenne la moitié de la tension d'alimentation.
- Avant d'envoyer, le module inverse le sens si `MOTOR_*_INVERTED` est vrai, et limite à
  `MOTOR_MAX_PWM_PCT`.
- Il n'écrit dans le générateur PWM que si la valeur change, parce que `safety` l'appelle à chaque tour
  de boucle.

**Si ça ne marche pas** (tester avec le banc `test_moteurs`, qui n'utilise que ce module) :
- **Aucune roue ne tourne, LED du MDD3A éteintes** → le MDD3A n'est pas alimenté. Mesurer 5V0 ↔ GND
  (environ 5 V) et VB+ ↔ VB− (environ 6 V). Vécu le 13/10 : l'alim des moteurs n'était pas branchée.
- **Une roue ne tourne que dans un sens** → un seul des deux fils de commande arrive. Avancer utilise
  l'entrée A, reculer l'entrée B. Vécu le 13/10 : le fil GPIO 25 → M1A.
- **Une roue tourne à l'envers** → `MOTOR_LEFT_INVERTED` ou `MOTOR_RIGHT_INVERTED`.
- **Le PAMI part à l'opposé de l'ultrason** → l'avant du code n'est pas l'avant réel. Vérifier
  d'abord que la roue gauche (vue depuis l'arrière) fait bouger `ticks G`, puis inverser les sens
  des moteurs et des encodeurs. Vécu le 15/10 (voir `docs/lecons-apprises.md`).
- **Les ordres n'arrivent pas** → GND du connecteur de commande du MDD3A ↔ GND de l'ESP32.

### 4.4 `encoders.*` : compter les tours de roue
**Rôle** : compter les ticks des deux encodeurs à effet Hall (D004).

| Fonction | Ce qu'elle fait |
|---|---|
| `begin()` | Configure les 4 broches et attache une interruption à chaque front (montant et descendant) de la voie A |
| `read(gauche, droite)` | Copie les deux compteurs au même instant (section critique) |

**Comment ça marche** :
- Chaque encodeur a deux sorties, A et B, décalées d'un quart de période (« quadrature »).
- À chaque front de A, l'interruption (`onLeftA()` / `onRightA()`) lit A et B :
  - A et B au même niveau → +1 ;
  - A et B différents → −1 ;
  - `ENCODER_*_INVERTED` échange ces deux cas.
- 960 impulsions par tour de roue, 2 fronts comptés → **1 920 ticks par tour**.
  1 tick = π × 60 / 1920 ≈ **0,098 mm**.
- Les compteurs partent de 0 à l'allumage et ne sont jamais remis à zéro : `motion` travaille avec des
  différences.

**Si ça ne marche pas** (tester avec le banc `test_odometrie`, roues tournées à la main) :
- **Les compteurs bougent alors que rien ne tourne** (des milliers par seconde) → les signaux flottent.
  Vécu le 13/10 : la masse des encodeurs n'était pas reliée à celle de l'ESP32.
- **Un compteur reste à 0** → fil A débranché, encodeur non alimenté (5 V entre + et GND de l'encodeur).
- **Un compteur descend quand la roue avance** → `ENCODER_*_INVERTED`. Vécu le 13/10 : roue gauche.
- **La roue gauche fait bouger `ticks D`** → encodeurs croisés : échanger les fils. Vécu le 13/10.
- **Les deux compteurs ne font que monter, même sans mouvement** → A et B reçoivent le même signal
  (ils se touchent, ou flottent ensemble).

### 4.5 `sensors.*` : l'ultrason et les capteurs de sol
**Rôle** : mesurer la distance devant (évitement) et lire le sol (bord de la zone, D009).

| Fonction | Ce qu'elle renvoie |
|---|---|
| `frontDistanceMm()` | Distance filtrée devant, en mm. 2000 = rien dans la portée |
| `isFaulty()` | Vrai si l'ultrason n'a donné aucune mesure depuis 1 s. **Reste vrai jusqu'au redémarrage** |
| `floorLeftReflects()` / `floorRightReflects()` | Vrai si le sol renvoie la lumière IR (surface claire) |

**L'ultrason, étape par étape** :
1. Dès que 60 ms sont passées depuis le dernier déclenchement **et** qu'ECHO est au repos (état bas),
   `trigger()` envoie une impulsion de 10 µs sur TRIG (GPIO 18). C'est la seule attente du programme.
   Si ECHO est encore haut, on réessaie au tour de boucle suivant : environ 16 mesures par seconde.
2. Le HC-SR04 envoie un « bip » ultrason et passe ECHO à l'état haut. ECHO redescend quand l'écho
   revient (ou après environ 38 ms s'il ne revient pas).
3. L'interruption `onEchoChange()` (GPIO 19) note l'heure de la montée, puis calcule la durée à la
   descente.
4. `updateUltrasound()` convertit la durée en distance : durée × 0,1715 mm/µs (le son fait 343 m/s,
   aller et retour). Plus de 12 000 µs → 2000 mm (« rien »).
5. Il garde les 3 dernières mesures et prend la **médiane** (`logic/median3.h`) : une mesure aberrante
   isolée est écartée.
6. **Filtre de montée** (`logic/rise_filter.h`) : une distance qui **diminue** est crue tout de suite ;
   une distance qui **augmente de plus de 50 mm d'un coup** doit être vue 4 fois de suite (environ
   0,25 s). Pourquoi : le capteur rate parfois l'écho d'un obstacle proche (« pas d'écho », ou le mur du
   fond). Sans ce filtre, un seul écho raté faisait croire la voie libre et le PAMI repartait (15/10).
7. Aucune mesure depuis 1 s → `isFaulty()` devient vrai et le log affiche `[SENS] PANNE ultrason`.
   **Sécurité par défaut : panne = obstacle**, le PAMI ne roule plus.

Au démarrage, la distance vaut 0 (« obstacle ») tant qu'aucune mesure n'est arrivée.

**Les capteurs de sol** : toutes les 10 ms, lecture de GPIO 34 et 35 (état bas = surface claire).
On garde l'avis d'au moins 2 lectures sur 3, pour ignorer un parasite.

**Si ça ne marche pas** :
- **`[SENS] PANNE ultrason` au démarrage** → pas d'écho reçu sur GPIO 19 : alimentation 5 V du module,
  masse commune, TRIG sur GPIO 18, pont diviseur sur ECHO (1 kΩ côté ECHO, 2 kΩ côté GND).
  Non résolu au 13/10 (voir `docs/lecons-apprises.md`).
- **`US` ne suit pas la main** → le module ne détecte pas d'écho, ou il voit le sol ou la table.
- **`[SAFE] Obstacle devant` sans obstacle** → l'ultrason est penché vers le sol, ou trop bas.

### 4.6 `safety.*` : le dernier contrôle avant les moteurs
**Rôle** : décider ce qui est **vraiment** envoyé aux moteurs. Seul module qui appelle `motor_driver`.

| Fonction | Ce qu'elle fait |
|---|---|
| `apply(commande, nowMs)` | Met à jour « obstacle devant », filtre la commande, l'envoie au driver |
| `obstacleAhead()` | Vrai si l'ultrason voit un obstacle (avec la marge de libération) |
| `isBlocking()` | Vrai si la dernière commande a été bloquée. `main.cpp` le transmet à `motion` |

**Les règles** (dans `logic/safety_filter.h`, fonction `filterCommand()`). Le PAMI est freiné si :
1. on est **en dehors de la période de mouvement** (avant 5 s ou après 14,5 s) ;
2. **l'ultrason est en panne** ;
3. sinon, une **rotation sur place** est toujours permise, même devant un obstacle : c'est ce qui permet
   de s'en écarter. Rotation sur place = roues en sens opposés, et la part « avance » est petite
   devant la part « rotation » ;
4. **une roue reculerait** : interdit, aucun capteur ne regarde derrière ;
5. **le PAMI avancerait vers un obstacle**.

**L'obstacle et sa marge** (`isObstacleAhead()`) : un obstacle apparaît sous 150 mm, et n'est considéré
parti qu'au-delà de 180 mm. Sans cette marge, une mesure qui oscille autour de 150 mm ferait avancer et
freiner en boucle.

**Logs** : `[SAFE] Obstacle devant (95 mm)` et `[SAFE] Voie libre (200 mm)`, à chaque changement.

**Aucun réglage ne permet de désactiver ce filtre** (règle d'or n° 1).

### 4.7 `match_timer.*` : le temps du match
**Rôle** : savoir quand le match a commencé, quand on a le droit de bouger, quand tout doit s'arrêter.

| Fonction | Vrai quand… |
|---|---|
| `isStarted()` | le départ a été donné |
| `isMoveAllowed()` | entre 5 s et 14,5 s après le départ |
| `isOver()` | 14,5 s sont passées. **Reste vrai pour toujours** |
| `elapsedMs()` | (renvoie le temps depuis le départ) |

**Le bouton de départ** (`logic/start_button.*`, classe `StartButtonDetector`) :
- le départ n'est **armé** qu'après avoir vu le bouton relâché pendant au moins 50 ms. Un bouton coincé
  ou maintenu à l'allumage ne peut donc pas lancer le PAMI ;
- un appui doit durer au moins 50 ms (les contacts rebondissent) ;
- **un seul départ par allumage**.

**Les calculs de temps** (`logic/match_clock.h`) : `isMatchOver()` et `isInMovePhase()`, avec des
soustractions (§ 1.1).

**Logs** : `[TIME] Pret : appuyez sur le bouton…`, `[TIME] DEPART !`, `[TIME] Mouvement autorise.`,
`[TIME] FIN (14.5 s)`.

**Si ça ne marche pas** : rien ne démarre → le message `Pret` est-il apparu ? Bouton entre GPIO 16 et GND ?
(Un simple fil touché à GND suffit pour tester.)

### 4.8 `color.*` : bleu ou jaune
- `begin()` : lit l'interrupteur (GPIO 17, pull-up interne). Ouvert = **bleu**, fermé (relié à GND) = **jaune**.
  Sans interrupteur, le PAMI joue bleu.
- `update()` : relit toutes les 100 ms **jusqu'au départ**.
- `lock()` : appelé par la machine à états au départ. Ensuite la couleur ne change plus.
- **Logs** : `[COUL] Couleur : BLEU`, `[COUL] Couleur verrouillee : BLEU`.

### 4.9 `motion.*` : avancer et tourner de façon mesurée
**Rôle** : exécuter « avance de X mm » ou « tourne de X degrés » en régulant la vitesse de chaque roue
grâce aux encodeurs, et suivre la position du PAMI (odométrie).

| Fonction | Ce qu'elle fait |
|---|---|
| `driveDistance(mm, vitesse)` | Commence une ligne droite (jamais en arrière, jamais plus vite que `DRIVE_SPEED_MM_S`) |
| `turnAngle(deg)` | Commence une rotation sur place. Positif = vers la gauche |
| `stop()` | Arrête le mouvement en cours (frein) |
| `isDone()` / `isDriving()` | Le mouvement est fini / on est en ligne droite |
| `isFaulty()` | Une roue est bloquée ou un encodeur compte à l'envers. **Reste vrai jusqu'au redémarrage** |
| `pose()` / `resetPose()` | Position (x, y, cap) / remise à (0, 0, 0°) |
| `command()` | La commande calculée pour les roues, que `safety` va contrôler |
| `update(nowMs, bloqué)` | Le calcul, toutes les 20 ms |

**Ce que fait `update()` toutes les 20 ms** :
1. **Mesure** : lit les ticks, calcule la vitesse de chaque roue (ticks × 0,098 mm / 0,02 s).
2. **Odométrie** : met à jour la position, même à l'arrêt (si on pousse le PAMI, sa position change).
3. Si aucun mouvement n'est demandé, ou en panne : commande nulle.
4. **Si `safety` a bloqué la commande** (obstacle, temps) : tout repart de zéro. Quand la voie se
   libère, le PAMI ré-accélère doucement.
5. **Ligne droite** :
   - distance restante = cible − moyenne des distances des deux roues ; fini à moins de 3 mm ;
   - vitesse voulue donnée par le profil en trapèze (`logic/speed_profile.h`) ;
   - **correction de cap** : si la roue gauche a fait 1 mm de plus que la droite, elle ralentit de
     5 mm/s et la droite accélère d'autant (`HEADING_KP_MM_S_PER_MM`).
6. **Rotation** :
   - chaque roue doit parcourir un arc = angle (en radians) × entraxe / 2 ;
   - la droite avance et la gauche recule (vers la gauche), ou l'inverse ; fini à moins de 2 mm d'arc ;
   - correction si le PAMI se met à avancer pendant qu'il tourne.
7. **Régulateur de chaque roue** (`logic/speed_controller.h`) : vitesse voulue → pourcentage de PWM.
8. **Détection de roue bloquée** (`checkStall()`) : une roue reçoit au moins 35 % de PWM, mais avance
   à moins de 15 mm/s dans le bon sens, pendant 0,6 s → panne, log `[MOT] PANNE : roue bloquee ou
   encodeur inverse`, et le PAMI s'arrête.

**Le profil en trapèze** (`nextProfileSpeed()`) : la vitesse monte d'au plus 400 mm/s par seconde,
plafonne à la vitesse de croisière, et redescend juste à temps pour s'arrêter sur la cible
(v = √(2 × a × d)). Jamais sous 40 mm/s, pour ne pas caler.

**Le régulateur** (`SpeedController`) :
```
PWM = anticipation + Kp × erreur + intégrale
anticipation = 10 % (pour décoller) + 0,2 % par mm/s voulu
erreur       = vitesse voulue − vitesse mesurée
intégrale    = somme des erreurs dans le temps (corrige un écart qui dure), bornée à ±30 %
```
Vitesse voulue nulle → PWM 0 (frein) et intégrale remise à zéro.

**L'odométrie** (`logic/odometry.*`) : à chaque mise à jour,
- le centre du PAMI avance de la moyenne des distances des deux roues ;
- il tourne de (distance droite − distance gauche) / entraxe, en radians ;
- x et y avancent selon le cap.

Repère : départ en (0, 0), regard vers +x, y positif à gauche, cap positif vers la gauche.

**Si ça ne marche pas** :
- **`[MOT] PANNE` au démarrage d'un mouvement** → une roue ne suit pas sa commande. Regarder les lignes
  juste avant : 0 tick (moteurs pas alimentés) ? compteur qui part à l'envers (encodeur inversé ou
  croisé) ? Vécu deux fois le 13/10.
- **La distance réelle n'est pas la bonne** → `WHEEL_DIAMETER_MM` (ou `ENCODER_TICKS_PER_WHEEL_TURN`).
- **Les angles réels ne sont pas les bons** → `WHEEL_BASE_MM`.
- **Le PAMI dévie au sol alors que `y` reste proche de 0** → les deux roues n'ont pas exactement le même
  diamètre. L'odométrie ne peut pas le voir : il faut une correction par roue.
- **Les roues avancent par à-coups** → régulateur trop nerveux : baisser `SPEED_KP_PCT_PER_MM_S`,
  `SPEED_KI_PCT_PER_MM`.

### 4.10 `mission.*` : dérouler les étapes
**Rôle** : jouer une mission, c'est-à-dire une liste d'étapes, une à la fois.

| Fonction | Ce qu'elle fait |
|---|---|
| `start(couleur, mission)` | Remet la position à zéro et charge les étapes (par défaut `ACTIVE_MISSION`) |
| `update()` | Lance le mouvement suivant quand le précédent est fini |
| `isFinished()` / `isDefendReached()` | Toutes les étapes sont faites / on est arrivé à l'étape « défendre » |

**Les étapes** (`logic/mission_step.h`) :
| Étape | Signification |
|---|---|
| `gotoPoint(x, y)` | Aller au point (x, y) |
| `turnTo(cap)` | Se tourner vers un cap absolu |
| `reachZone(cap)` | Avancer lentement au cap donné jusqu'au bord de la zone (capteurs de sol) |
| `defend()` | Rester sur place, bras prêt |

**Les missions** (`logic/missions.h`), écrites pour le bleu :
| Mission | Étapes |
|---|---|
| `STRAIGHT_LINE` | aller en (500, 0) |
| `ROTATION` | se tourner vers 90°, 180°, −90°, puis 0° (un tour complet) |
| `SQUARE` | les 4 coins d'un carré de 200 mm, puis cap 0° |
| `DEFENDER` | point d'approche → cap vers la zone → approche jusqu'au bord → cap vers l'adversaire → défendre |

**Le jaune est le miroir du bleu** (`forColor()`) : y devient −y et les caps changent de signe
(règle d'or n° 5).

**Comment une étape est jouée** (`logic/next_move.h`, fonction `decideNextMove()`) : on ne mémorise pas
« où on en était ». À chaque fois, on regarde la position et la cible :
- point atteint (à moins de 30 mm) ou cap atteint (à moins de 3°) → étape suivante ;
- mal orienté → tourner vers la cible ;
- bien orienté → avancer de la distance restante.

**C'est ce qui rend le contournement simple** : après un détour, le même calcul ramène le PAMI vers son
point.

**L'approche de la zone** (`REACH_ZONE`, D009, `logic/zone_approach.h`) :
1. on note ce que voient les deux capteurs de sol au début de l'approche ;
2. on avance à 80 mm/s en surveillant le sol à chaque tour de boucle ;
3. dès que l'un des deux voit autre chose → bord de la zone : frein, puis on avance encore de 50 mm ;
4. sans bord vu après 300 mm, on s'arrête quand même.

**Logs** : `[MISS] Debut : 1 etapes, couleur BLEU`, `[MISS] Etape terminee | x = … y = … cap = …`,
`[MISS] Mission terminee`, `[MISS] Bord de la zone`, `[MISS] Zone atteinte, defense`.

**Si ça ne marche pas** :
- **Le PAMI fait de petits allers-retours de rotation à la fin d'une étape** → il dépasse la cible en
  freinant, puis corrige. Voir `HEADING_TOLERANCE_DEG` et `TURN_TOLERANCE_MM`.
- **Le PAMI va au mauvais endroit** → regarder les `x`, `y`, `cap` affichés : si l'odométrie croit être
  au bon endroit, c'est un réglage de `motion` (§ 4.9) ; sinon, c'est la mission dans `missions.h` ou
  `config.h`.

### 4.11 `state_machine.*` : la machine à états
**Rôle** : décider ce que fait le PAMI à chaque instant. C'est le « cerveau ».

**Les états** (`logic/pami_state.h`) :
| État | Le PAMI… | En sort quand… |
|---|---|---|
| `WAIT_START` | est allumé, immobile, attend le départ | le bouton est appuyé → `WAIT_DELAY` |
| `WAIT_DELAY` | attend 5 s, immobile | 5 s passées → `MISSION` |
| `MISSION` | joue sa mission | obstacle en ligne droite → `AVOID_STOP` ; étape « défendre » → `DEFEND` |
| `AVOID_STOP` | est arrêté devant un obstacle | l'obstacle part → `MISSION` ; toujours là après 1 s → `DETOUR` (3 fois au plus) |
| `DETOUR` | fait un quart de tour (gauche si bleu, droite si jaune), puis 200 mm de côté | détour fini → `MISSION`, qui vise de nouveau son point |
| `DEFEND` | est immobile dans la zone, déploie le bras si un adversaire est à moins de 60 mm | — |
| `END` | est arrêté pour toujours (14,5 s) | — |
| `FAULT` | est arrêté pour toujours (ultrason en panne ou roue bloquée) | redémarrage de la carte |

**Les priorités, à chaque tour, avant tout le reste** :
1. le temps est fini → `END`, quel que soit l'état (règle d'or n° 3) ;
2. un capteur ou une roue est en panne → `FAULT`.

**Un obstacle ne compte que si le PAMI avance vers lui** (`stopIfObstacle()`) : pendant une rotation sur
place, on continue.

**Logs** : `[FSM] WAIT_START -> WAIT_DELAY`, `[FSM] Detour 1 par la gauche`…

**Si ça ne marche pas** : lire la suite des lignes `[FSM]`. Elles racontent exactement ce que le PAMI a
décidé, et dans quel ordre.

### 4.12 `arm.*` : le bras du défenseur
**Rôle** : piloter le servo SG90 du bras (GPIO 23), directement avec le générateur PWM de l'ESP32
(canal 4, 50 Hz), sans bibliothèque (D008).

| Fonction | Ce qu'elle fait |
|---|---|
| `begin()` | Pas de signal : le servo ne bouge pas avant la défense |
| `retract()` / `deploy()` | Bras replié (0°) / déployé (90°). **Seulement entre 5 s et 14,5 s** |
| `release()` | Coupe le signal pour toujours : le bras ne bouge plus |
| `update()` | Coupe le signal dès la fin du temps, même si la machine à états l'oubliait |

**Comment ça marche** : un servo attend une impulsion toutes les 20 ms. Sa largeur donne l'angle :
500 µs = 0°, 2400 µs = 180°.

**Logs** : `[ARM] Bras deploye.`, `[ARM] Servo coupe.` (s'affiche à chaque fin de match, même sans servo).

## 5. Le dossier `logic/` en un coup d'œil
Calculs purs, sans matériel, testables sur PC.

| Fichier | Contenu | Utilisé par |
|---|---|---|
| `pami_state.h` | Liste des états et leurs noms pour les logs | `state_machine`, `main` |
| `wheel_command.h` | `WheelCommand` : pourcentage gauche et droit | `motion`, `safety` |
| `match_clock.h` | `isMatchOver()`, `isInMovePhase()`, `elapsedSinceMs()` | `match_timer` |
| `start_button.*` | `StartButtonDetector` : anti-rebond, armement, un seul départ | `match_timer` |
| `median3.h` | Médiane de 3 valeurs | `sensors` |
| `rise_filter.h` | `RiseFilter` : une distance qui s'allonge d'un coup doit être confirmée | `sensors` |
| `safety_filter.h` | `isObstacleAhead()`, `isTurnInPlace()`, `filterCommand()` | `safety` |
| `speed_profile.h` | `nextProfileSpeed()` : accélération, croisière, freinage | `motion` |
| `speed_controller.h` | `SpeedController` : vitesse voulue → PWM | `motion` |
| `odometry.*` | `Odometry` : position (x, y, cap) ; `normalizeDeg()` | `motion`, `next_move` |
| `mission_step.h` | Types d'étapes, couleur, miroir `forColor()` | `mission` |
| `missions.h` | Les 4 missions, écrites pour le bleu | `mission` |
| `next_move.h` | `decideNextMove()` : DONE, TURN ou DRIVE | `mission` |
| `zone_approach.h` | `floorChanged()`, `remainingSearchMm()` | `mission` |
| `avoidance.h` | `chooseDetourSide()` : côté du détour | `state_machine` |

## 6. Trouver un bug soi-même

### 6.1 La méthode : le matériel d'abord
Le 13/10, **tous** les problèmes venaient du câblage ou de l'alimentation, aucun du code. Dans l'ordre :
1. **Alimentation**, mesurée sur le composant lui-même (pas sur la source).
2. **Masse commune** : tous les GND reliés.
3. **Câblage** conforme à `docs/materiel.md`, sur les bonnes broches.
4. **Niveaux logiques** : jamais plus de 3,3 V sur une broche de l'ESP32.
5. **Le composant seul**, avec un banc de test.
6. **Seulement ensuite, le code.**

**Les bancs de test servent à séparer matériel et code :**
| Programme | Ce qu'il utilise | Si ça marche ici mais pas plus loin… |
|---|---|---|
| `test_moteurs` | `motor_driver` seul | …le driver et les moteurs vont bien |
| `test_odometrie` | + encodeurs, `motion`, `mission` | …le mouvement et l'odométrie vont bien |
| firmware (`esp32dev`) | tout, avec `safety`, minuteur, ultrason, machine à états | — |

Si le firmware échoue alors que `test_odometrie` marche, chercher du côté de l'ultrason, du minuteur,
de `safety` ou de la machine à états.

### 6.2 Du symptôme au module
| Ce que tu vois | Module à regarder | Première vérification |
|---|---|---|
| Rien ne s'affiche au moniteur | — | `pio device monitor`, appui sur EN, `waiting for download` = BOOT enfoncé |
| Reste en `WAIT_START` | `match_timer` (§ 4.7) | Message `Pret` affiché ? Fil entre GPIO 16 et GND |
| `FAULT` environ 1 s après l'allumage, `[SENS] PANNE ultrason` | `sensors` (§ 4.5) | Alim 5 V du HC-SR04, masse commune, pont sur ECHO |
| `[MOT] PANNE` au début d'un mouvement | `motion` (§ 4.9) | Ticks avant la panne : 0 → moteurs non alimentés ; à l'envers → encodeur inversé ou croisé |
| PWM affichée mais roues immobiles, LED du MDD3A éteintes | `motor_driver` (§ 4.3) | 5V0 ↔ GND du MDD3A : 5 V ? VB+ ↔ VB− : 6 V ? |
| Une roue ne tourne que dans un sens | `motor_driver` (§ 4.3) | Le fil de l'entrée A ou B de cette roue |
| Ticks qui bougent sans mouvement | `encoders` (§ 4.4) | Masse des encodeurs = masse de l'ESP32 |
| S'arrête sans obstacle, `[SAFE] Obstacle devant` | `sensors`, `safety` | Valeur `US` : l'ultrason voit-il le sol ? |
| Ne repart pas après un obstacle retiré | `safety` (marge de 30 mm) | `US` doit dépasser 180 mm |
| Distance parcourue fausse | `config.h` | `WHEEL_DIAMETER_MM`, `ENCODER_TICKS_PER_WHEEL_TURN` |
| Angles faux | `config.h` | `WHEEL_BASE_MM` |
| Dévie au sol alors que `y` ≈ 0 | `motion` | Diamètres des deux roues différents |
| L'ESP32 redémarre quand les moteurs démarrent | alimentation | `Cause du redemarrage : BROWNOUT` ? Courant moteurs hors breadboard et hors USB |
| Redémarrage puis lignes `load:0xd00…` et `mmu set…` | ESP32 | Rien sur GPIO 12 (ni 0, 2, 15) |
| `tours/s` sous 1 000 | `main` / code | Quelque chose bloque la boucle : `delay()`, trop de `Serial.print` |

### 6.3 Ajouter un message pour comprendre
On peut ajouter une ligne de log temporaire dans un module :
```cpp
if (DEBUG_LOGS) {
  Serial.printf("[MOT] gauche voulu %.0f mm/s, mesure %.0f mm/s\n", leftTargetMmS, leftMeasuredMmS);
}
```
- Préfixe du module entre crochets, pour savoir d'où vient le message.
- **Jamais dans une interruption** (fonctions `IRAM_ATTR`).
- **Pas à chaque tour de boucle** : 180 000 messages par seconde satureraient le port série et
  ralentiraient tout. Le mettre dans une fonction appelée toutes les 20 ms au plus (comme
  `motion::update`), ou derrière un test de temps (§ 1.1).
- Le retirer une fois le bug trouvé.

## 7. Programmes et environnements PlatformIO

| Environnement | Programme | Commande |
|---|---|---|
| `esp32dev` | Firmware de match (`src/main.cpp`) | `pio run -t upload -t monitor` |
| `test_moteurs` | `test_bench/moteurs/main.cpp` : touches `a` `r` `g` `d` `+` `-` `s` | `pio run -e test_moteurs -t upload -t monitor` |
| `test_odometrie` | `test_bench/odometrie/main.cpp` : `1` ligne, `2` rotation, `3` carré, sans ultrason | `pio run -e test_odometrie -t upload -t monitor` |
| `native` | Tests de `logic/` sur PC (`test/test_match_timer/`) | `pio test -e native` (g++ nécessaire) |
| `logic_esp32` | Les mêmes tests, sur l'ESP32 | `pio test -e logic_esp32` |

**Les bancs de test n'ont pas d'évitement** : ne jamais les laisser dans le PAMI. Après un banc,
remettre le firmware : `pio run -t upload`.

## 8. Petit glossaire
- **PWM** (modulation de largeur d'impulsion) : signal qui alterne très vite entre 0 et 3,3 V. Le
  pourcentage de temps à l'état haut règle la puissance moyenne envoyée au moteur.
- **LEDC** : le générateur de PWM intégré à l'ESP32. Il a des « canaux » (0 à 3 pour les moteurs,
  4 pour le servo).
- **Interruption** : petite fonction exécutée immédiatement quand une broche change d'état (§ 1.5).
- **`volatile`** : variable modifiée dans une interruption, que le compilateur doit relire à chaque fois.
- **Section critique** : moment où les interruptions ne peuvent pas modifier une variable partagée.
- **Quadrature** : deux signaux A et B décalés d'un quart de période ; l'ordre de leurs fronts donne le
  sens de rotation.
- **Tick** : un front compté sur la voie A d'un encodeur (environ 0,098 mm de roue).
- **Odométrie** : calcul de la position à partir des distances parcourues par chaque roue.
- **Régulateur** : calcul qui corrige la commande pour que la vitesse mesurée rejoigne la vitesse voulue.
- **Anticipation** : la part de la commande calculée directement à partir de la vitesse voulue, sans
  attendre l'erreur.
- **Intégrale** : la part qui corrige un écart qui dure (frottement, pente, batterie qui baisse).
- **Hystérésis / marge de libération** : deux seuils différents pour entrer et sortir d'un état, pour
  éviter d'osciller.
- **Médiane** : la valeur du milieu de 3 mesures ; une mesure aberrante est ignorée.
- **Pull-up** : résistance qui tire une broche à l'état haut quand rien ne la force à l'état bas.
- **Flottant** : une broche reliée à rien (ou sans masse commune) lit n'importe quoi.
- **Machine à états** : le PAMI est toujours dans un seul état ; des conditions précises le font passer
  d'un état à l'autre.

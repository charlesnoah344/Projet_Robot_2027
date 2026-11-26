# Comprendre le code du PAMI

> Ce document explique le firmware du PAMI fichier par fichier : à quoi sert chaque module, comment il
> marche, ce qu'il affiche dans le moniteur série, et quoi vérifier quand il ne fait pas ce qu'on attend.
> But : pouvoir trouver un bug soi-même, comme le problème de masse commune du 13/10/2026.
>
> **Pour s'en servir face à un bug** : commencer par le § 6 (symptôme → module), puis lire la section
> du module concerné. Le code fait foi : si ce document et le code divergent, c'est le code qui a raison,
> et il faut corriger ce document.
>
> À jour au 25/11/2026 (brochage D018, après D012 : 2 ultrasons en OU, un TRIG chacun, plus d'IR ;
> départ et côté par la commande `go g` / `go d` D013, D020 ; douve assignée à chaque PAMI D017 ;
> vitesse 350 mm/s D019 ; attaque du bras D021 ; profil labo D007).
> **L'avant du PAMI est le côté des ultrasons.** Gauche et droite se voient depuis l'arrière, en
> regardant vers les ultrasons.

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
1. matchTimer::update     lit la commande de départ (« go g » / « go d ») et donne le côté à side,
                          suit le temps (5 s, 14,5 s)
2. sensors::update        déclenche les 2 ultrasons (ensemble), convertit les échos
3. stateMachine::update   décide quoi faire (attendre, rouler, éviter, défendre, s'arrêter)
4. motion::update         toutes les 20 ms : régule la vitesse des roues, met à jour la position
5. safety::apply          vérifie la commande et l'envoie au driver (ou freine)
6. arm::update            coupe le servo dès la fin du temps
   logStatus              une ligne [MAIN] par seconde
```
L'ordre compte : le minuteur passe en premier, pour que la fin du temps soit vue avant toute décision.

### 2.2 Exemple : de la commande de départ à la roue qui tourne
1. On tape `go g` (ou `go d`) puis Entrée dans le moniteur → `match_timer` reconnaît la commande, donne
   le côté à `side` → `isStarted()` devient vrai.
2. `state_machine` passe de `WAIT_START` à `WAIT_DELAY`. Le côté ne changera plus.
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
7. `motor_driver` règle les signaux PWM sur GPIO 32/33 (roue gauche) et 19/18 (roue droite).
8. Le MDD3A transforme ces signaux en courant pour les moteurs, pris sur **son alimentation
   (VB+ / VB−)**.
9. La roue tourne → l'encodeur envoie des fronts → le compteur matériel (PCNT) compte les ticks → retour à l'étape 5.

**Chaque maillon de cette chaîne peut casser.** Le § 6 dit comment trouver lequel.

## 3. Carte des fichiers

| Fichier (`pami/src/`) | Couche | Rôle en une phrase | Matériel | Préfixe des logs |
|---|---|---|---|---|
| `main.cpp` | orchestre | Appelle chaque module dans l'ordre, affiche l'état chaque seconde | — | `[MAIN]` |
| `config.h` | réglages | Toutes les constantes | — | — |
| `motor_driver.*` | bas niveau | Transforme « +50 % » en signaux PWM pour le MDD3A | GPIO 27, 26, 16, 17 | — |
| `encoders.*` | capteur | Compte les ticks de chaque roue | GPIO 33, 32, 19, 18 | — |
| `sensors.*` | capteur | 2 ultrasons avant (gauche, droit) | GPIO 25 / 35 (G), 5 / 4 (D) | `[SENS]` |
| `safety.*` | sécurité | Dernier contrôle avant les moteurs | — | `[SAFE]` |
| `match_timer.*` | temps | Départ et côté (commande `go g` / `go d`), 5 s d'attente, arrêt à 14,5 s | — (console) | `[TIME]` |
| `side.*` | réglage match | Garde le côté GAUCHE / DROITE donné par la commande de départ | — | `[COTE]` |
| `motion.*` | mouvement | Fait avancer ou tourner de façon mesurée, suit la position | — | `[MOT]` |
| `mission.*` | stratégie | Déroule les étapes de la mission choisie | — | `[MISS]` |
| `state_machine.*` | décision | Choisit l'état du PAMI (attente, mission, évitement…) | — | `[FSM]` |
| `arm.*` | actionneur | Bras (servo) | GPIO 13 | `[ARM]` |
| `logic/*` | calculs purs | Filtres, régulateur, odométrie, décisions (détail au § 5) | aucun | — |

## 4. Les modules un par un

### 4.1 `main.cpp` : le chef d'orchestre
- **`setup()`** :
  1. `motorDriver::begin()` **en tout premier**, pour que les moteurs soient freinés avant toute chose ;
  2. ouverture du port série et messages de démarrage ;
  3. `begin()` de chaque module.
- **`loop()`** : voir § 2.1.
- **`logStatus()`** : une ligne par seconde, **seulement après le départ**. Avant, le moniteur reste
  calme pour qu'on puisse taper `go g` ou `go d` sans que des lignes défilent (les messages `[SAFE]`
  attendent aussi le départ ; la détection d'obstacle, elle, marche tout le temps).
  ```
  [MAIN] 180000 tours/s | etat WAIT_DELAY | t = 1.0 s | cote GAUCHE | US G 2000 D 2000 mm | ticks G 0 D 0
  ```
  - `tours/s` : santé de la boucle (doit rester au-dessus de 1 000) ;
  - `etat` : l'état de la machine à états ;
  - `t` : le temps depuis le départ ;
  - `cote` : le côté donné par la commande de départ (GAUCHE ou DROITE) ;
  - `US G / D` : la distance filtrée vue par chaque ultrason (2000 = rien dans la portée,
    0 = pas encore mesuré) ;
  - `ticks G / D` : les compteurs des encodeurs.
- **Si ça ne marche pas** : rien ne s'affiche → vérifier `pio device monitor`, appuyer sur EN.
  Le moniteur affiche `waiting for download` → BOOT était enfoncé au démarrage (voir
  `docs/lecons-apprises.md`).

### 4.2 `config.h` : les réglages
Organisé en blocs : journal série, broches, moteurs, encodeurs, mouvement, missions, ultrason et
sécurité, évitement, bras, côté de départ, minuteur, bancs de test.

Les constantes qui changent le plus souvent le comportement :

| Constante | Effet | Valeur au 13/10 |
|---|---|---|
| `ACTIVE_MISSION` | La mission jouée par le firmware de match | `STRAIGHT_LINE` |
| `WHEEL_DIAMETER_MM` | Diamètre moyen des roues : distance pour 1 tick → toutes les distances | 59,4 (T006) |
| `WHEEL_DIAMETER_RIGHT_TO_LEFT_RATIO` | Écart entre les deux roues : sans lui, le PAMI dérive en ligne droite (D023) | 1,0079 (T022) |
| `WHEEL_BASE_MM` | Écart entre les roues → tous les angles | 101,6 (T007) |
| `ENCODER_LEFT_INVERTED` / `ENCODER_RIGHT_INVERTED` | Sens de comptage de chaque encodeur | `true` / `false` (T014) |
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

- Roue gauche : A = GPIO 27 (M1_IN1), B = GPIO 26 (M1_IN2). Roue droite : A = GPIO 16 (M2_IN1),
  B = GPIO 17 (M2_IN2) (brochage du 25/11/2026, D018).
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
| `begin()` | Configure les deux compteurs matériels (PCNT) et un minuteur qui ramasse leurs valeurs toutes les 10 ms |
| `read(gauche, droite)` | Ramasse les deux compteurs et les copie au même instant (section critique) |

**Comment ça marche** :
- Chaque encodeur a deux sorties, A et B, décalées d'un quart de période (« quadrature »).
- Le comptage est fait par le **compteur matériel de l'ESP32 (PCNT)**, un par roue (D022). À chaque
  front de A, le circuit lit lui-même B, sans aucun retard :
  - A et B au même niveau → +1 ;
  - A et B différents → −1 ;
  - `ENCODER_*_INVERTED` échange ces deux cas.
- Pourquoi plus d'interruptions : moteurs alimentés, des parasites très courts arrivaient sur les fils
  des encodeurs, et une interruption un peu en retard (Wi-Fi) lisait B trop tard. À 350 mm/s, une roue
  tournait 32 % plus vite que l'autre alors que les encodeurs les annonçaient égales (T021).
- **Filtre anti-parasites** : le PCNT ignore les impulsions plus courtes que `ENCODER_GLITCH_FILTER_NS`
  (5 µs). Un vrai front arrive au plus vite toutes les 278 µs à 350 mm/s.
- Le PCNT compte sur 16 bits (de −30 000 à +30 000, puis il repart de 0). Toutes les 10 ms, un minuteur
  de l'ESP32 ajoute ce qu'il a compté à un total sur 32 bits (`collect()`). Entre deux ramassages, une roue
  fait au plus quelques dizaines de ticks : un saut de plus de 15 000 veut dire que le compteur est
  reparti de 0, et on le corrige.
- 960 impulsions par tour de roue, 2 fronts comptés → **1 920 ticks par tour**.
  1 tick ≈ π × 59,4 / 1920 ≈ **0,097 mm**, un peu différent pour chaque roue (`LEFT_MM_PER_TICK`,
  `RIGHT_MM_PER_TICK`) : les deux roues n'avancent pas exactement autant par tick (D023). Sans cette
  correction, à ticks égaux, le PAMI décrivait un grand arc de cercle sans le voir (T022).
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
- **À grande vitesse, le PAMI dérive alors que l'odométrie croit aller droit, et une roue demande
  beaucoup plus de PWM que l'autre même en l'air** → comptage faux, moteurs alimentés (T018 à T021).
  Vérifier avec une vidéo des roues sur cales : à vitesse annoncée égale, elles doivent tourner pareil.

### 4.5 `sensors.*` : les deux ultrasons
**Rôle** : mesurer la distance devant, avec deux HC-SR04 à l'avant, un à gauche et un à droite (D012).

| Fonction | Ce qu'elle renvoie |
|---|---|
| `distanceMm(UltrasoundId::LEFT)` / `(…::RIGHT)` | Distance filtrée vue par ce capteur, en mm. 2000 = rien dans la portée |
| `frontDistanceMm()` | La plus courte des deux : l'objet le plus proche devant (bras, messages) |
| `isFaulty(id)` / `isFaulty()` | Ce capteur / l'un OU l'autre n'a donné aucune mesure depuis 1 s. **Reste vrai jusqu'au redémarrage** |
| `lastRawMeasure(id)` | Détail de la dernière mesure, pour le banc `test_ultrason` |

**Les ultrasons, étape par étape** :
1. **Les deux capteurs émettent ensemble** : chacun a son TRIG (GPIO 25 à gauche, 5 à droite, D018),
   mais le firmware les déclenche en même temps, comme du temps du TRIG commun (D015).
   Dès que 60 ms sont passées depuis la dernière émission **et** que les deux ECHO sont au repos (état
   bas), `trigger()` envoie une impulsion de 10 µs sur les deux TRIG. C'est la seule attente du programme.
   Chaque capteur mesure donc environ 16 fois par seconde. Si un ECHO reste haut plus de 120 ms
   (capteur bloqué), on déclenche quand même : l'autre capteur continue de mesurer, et le capteur
   bloqué est déclaré en panne au bout de 1 s.
   Le revers : un capteur peut entendre le son de l'autre. Renvoyé par un vrai obstacle, ça ne change
   rien (OU, § 4.6). Passé directement d'un capteur à l'autre, ça ferait voir un faux obstacle tout
   proche : le PAMI s'arrêterait sans raison. À vérifier au banc `test_ultrason` (protocole 6).
2. Le HC-SR04 envoie un « bip » ultrason et passe ECHO à l'état haut. ECHO redescend quand l'écho
   revient (ou après environ 58 ms s'il ne revient pas).
3. Une interruption par capteur (`onLeftEcho()` sur GPIO 35, `onRightEcho()` sur GPIO 4) note l'heure
   de la montée, puis calcule la durée à la descente.
4. `collectEcho()` convertit la durée en distance : durée × 0,1715 mm/µs (le son fait 343 m/s,
   aller et retour). Plus de 12 000 µs → 2000 mm (« rien »).
5. Chaque capteur garde ses 3 dernières mesures et prend la **médiane** (`logic/median3.h`) : une
   mesure aberrante isolée est écartée.
6. **Filtre de montée** (`logic/rise_filter.h`), un par capteur : une distance qui **diminue** est crue
   tout de suite ; une distance qui **augmente de plus de 50 mm d'un coup** doit être vue 4 fois de
   suite (environ 0,25 s). Pourquoi : le capteur rate parfois l'écho d'un obstacle proche. Sans ce
   filtre, un seul écho raté faisait croire la voie libre et le PAMI repartait (15/10).
7. Aucune mesure d'un capteur depuis 1 s → `isFaulty()` devient vrai et le log affiche
   `[SENS] PANNE ultrason gauche` (ou `droit`). **Sécurité par défaut : panne = obstacle**, le PAMI ne
   roule plus, même si l'autre capteur marche.

Au démarrage, chaque distance vaut 0 (« obstacle ») tant qu'aucune mesure n'est arrivée.

**Si ça ne marche pas** :
- **`[SENS] PANNE ultrason droit` au démarrage** → pas d'écho reçu sur GPIO 4 : alimentation 5 V du
  module, connecteur, TRIG et ECHO inversés, pont diviseur, capteur. **Les deux en panne** → plutôt
  l'alimentation des ultrasons. Le banc `test_ultrason`, touche `r`, teste chaque capteur sans notre code.
- **`US G` ou `US D` ne suit pas la main** → le module ne détecte pas d'écho, ou il voit le sol ou la table.
- **`[SAFE] US G : obstacle devant` sans obstacle** → ce capteur est penché vers le sol, ou trop bas.

### 4.6 `safety.*` : le dernier contrôle avant les moteurs
**Rôle** : décider ce qui est **vraiment** envoyé aux moteurs. Seul module qui appelle `motor_driver`.

| Fonction | Ce qu'elle fait |
|---|---|
| `apply(commande, nowMs)` | Met à jour « obstacle devant », filtre la commande, l'envoie au driver |
| `obstacleAhead()` | Vrai si l'ultrason gauche OU le droit voit un obstacle (avec la marge de libération) |
| `isBlocking()` | Vrai si la dernière commande a été bloquée. `main.cpp` le transmet à `motion` |

**Les règles** (dans `logic/safety_filter.h`, fonction `filterCommand()`). Le PAMI est freiné si :
1. on est **en dehors de la période de mouvement** (avant 5 s ou après 14,5 s) ;
2. **un ultrason est en panne** ;
3. sinon, une **rotation sur place** est toujours permise, même devant un obstacle : c'est ce qui permet
   de s'en écarter. Rotation sur place = roues en sens opposés, et la part « avance » est petite
   devant la part « rotation » ;
4. **une roue reculerait** : interdit, aucun capteur ne regarde derrière ;
5. **le PAMI avancerait vers un obstacle**.

**L'obstacle, le OU et la marge** (`isObstacleAhead()`, appelée pour chaque capteur) : pour chaque
ultrason, un obstacle apparaît sous 150 mm, et n'est considéré parti qu'au-delà de 180 mm. Sans cette
marge, une mesure qui oscille autour de 150 mm ferait avancer et freiner en boucle. Puis
`obstacle = gauche OU droit` : un seul capteur qui voit l'obstacle suffit. Si l'un le rate, l'autre le
voit encore (D012).

**Logs** : `[SAFE] US G : obstacle devant (95 mm)` et `[SAFE] US G : voie libre (200 mm)`, à chaque
changement, pour chaque capteur.

**Aucun réglage ne permet de désactiver ce filtre** (règle d'or n° 1).

### 4.7 `match_timer.*` : le temps du match
**Rôle** : savoir quand le match a commencé, quand on a le droit de bouger, quand tout doit s'arrêter.

| Fonction | Vrai quand… |
|---|---|
| `isStarted()` | le départ a été donné |
| `isMoveAllowed()` | entre 5 s et 14,5 s après le départ |
| `isOver()` | 14,5 s sont passées. **Reste vrai pour toujours** |
| `elapsedMs()` | (renvoie le temps depuis le départ) |

**La commande de départ** (`logic/start_command.h`, classe `StartCommandDetector`, D013, D020) :
- on tape `go g` (PAMI GAUCHE) ou `go d` (PAMI DROITE) puis Entrée dans le moniteur, Wi-Fi
  (`pio device monitor -e esp32dev_wifi`) ou USB. La commande donne le départ **et** le côté ;
- `go` seul est refusé (message « Il manque le cote ») : le côté doit toujours être choisi exprès ;
- majuscules et espaces ne comptent pas (`GO G`, `gog` marchent aussi) ; tout autre mot est refusé ;
- un mot plutôt qu'une touche : une touche frappée par erreur ne lance pas le PAMI ;
- **un seul départ par allumage** ; le PAMI ne part jamais tout seul à l'allumage ;
- ⚠ **profil labo seulement** : à Eurobot, ce départ est interdit (règlement général F.6).

**Les calculs de temps** (`logic/match_clock.h`) : `isMatchOver()` et `isInMovePhase()`, avec des
soustractions (§ 1.1).

**Logs** : `[TIME] Pret.` puis l'aide `Pour le depart, tapez go g…`, `[COTE] GAUCHE (bord a gauche,
centre a droite)`, `[TIME] DEPART !`, `[TIME] Mouvement autorise.`, `[TIME] FIN (14.5 s)`,
`[TIME] Il manque le cote`, `[TIME] Commande inconnue.`, `[TIME] Depart deja donne : commande ignoree.`

**Si ça ne marche pas** : rien ne démarre → le moniteur est-il connecté au PAMI ? Avez-vous appuyé sur
Entrée ? Le message `Commande inconnue` s'affiche-t-il (faute de frappe) ?

### 4.8 `side.*` : PAMI gauche ou droite
- Le sens (D017) : le PAMI est posé dans son écurie, tourné vers l'adversaire. **GAUCHE** = le bord de
  la table est à sa gauche, donc le centre est à sa droite. **DROITE** = l'inverse. Ce n'est pas le
  côté vu du public : l'écurie jaune avant, par exemple, est DROITE (tableau dans `docs/materiel.md`).
- Plus d'interrupteur (D020) : `choose()` est appelé par `match_timer` quand la commande de départ
  arrive (`go g` → GAUCHE, `go d` → DROITE). Seul le premier appel compte : le côté ne change plus.
- `isChosen()` : vrai une fois le côté choisi. `current()` : le côté.
- Le côté sert à deux choses : les trajets, écrits pour la gauche, sont mis en miroir pour la droite
  (`forSide()`, voir `mission.*`) ; le détour se fait vers le centre de la table (`logic/avoidance.h`).
- **Logs** : `[COTE] GAUCHE (bord a gauche, centre a droite)` au départ.

### 4.9 `motion.*` : avancer et tourner de façon mesurée
**Rôle** : exécuter « avance de X mm » ou « tourne de X degrés » en régulant la vitesse de chaque roue
grâce aux encodeurs, et suivre la position du PAMI (odométrie).

| Fonction | Ce qu'elle fait |
|---|---|
| `driveDistance(mm, vitesse)` | Commence une ligne droite (jamais en arrière, jamais plus vite que `DRIVE_SPEED_MM_S`) |
| `turnAngle(deg)` | Commence une rotation sur place. Positif = vers la gauche |
| `stop()` | Arrête le mouvement en cours (frein) |
| `isDone()` / `isDriving()` | Le mouvement est fini / on est en ligne droite. « Fini » = cible atteinte **et** PAMI vraiment arrêté : après la cible, `motion` freine et attend que les deux roues restent sous 10 mm/s pendant 100 ms (D024). Sans ça, à 350 mm/s, la rotation démarrait pendant la glissade de fin de ligne et se bloquait (T027) |
| `isFaulty()` | Une roue est bloquée ou un encodeur compte à l'envers. **Reste vrai jusqu'au redémarrage** |
| `pose()` / `resetPose()` | Position (x, y, cap) / remise à (0, 0, 0°) |
| `command()` | La commande calculée pour les roues, que `safety` va contrôler |
| `update(nowMs, bloqué)` | Le calcul, toutes les 20 ms |

**Ce que fait `update()` toutes les 20 ms** :
1. **Mesure** : lit les ticks, calcule la vitesse de chaque roue (ticks × mm par tick de cette roue / 0,02 s).
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
| `start(côté, mission)` | Remet la position à zéro et charge les étapes (par défaut `ACTIVE_MISSION`) |
| `update()` | Lance le mouvement suivant quand le précédent est fini |
| `isFinished()` / `isDefendReached()` | Toutes les étapes sont faites / on est arrivé à l'étape « rester ici » (dans la douve) |

**Les étapes** (`logic/mission_step.h`) :
| Étape | Signification |
|---|---|
| `gotoPoint(x, y)` | Aller au point (x, y) |
| `turnTo(cap)` | Se tourner vers un cap absolu |
| `defend()` | Rester sur place, bras prêt |

**Les missions** (`logic/missions.h`), écrites pour un PAMI **GAUCHE** :
| Mission | Étapes |
|---|---|
| `STRAIGHT_LINE` | aller en (500, 0) |
| `ROTATION` | se tourner vers 90°, 180°, −90°, puis 0° (un tour complet) |
| `SQUARE` | les 4 coins d'un carré de 200 mm, puis cap 0° |
| `MOAT` | point de virage (1900, 0) → douve de son côté (2079, −219) → y rester, bras prêt (D017) |

**La mission `MOAT`** (D017) : chaque PAMI va à **sa** douve, la douve diagonale de son côté. Deux PAMI
ne visent donc jamais la même.
1. Les points sont écrits dans `config.h` dans le **repère de la table** vu par le PAMI
   (`docs/douves-coordonnees.md`) : x = avance depuis notre petit bord, y = distance au grand bord côté
   écurie. Ce repère est le même pour les 4 écuries. Départ (provisoire) `START_POINT_MM` = (100, 210),
   point de virage `MOAT_WAYPOINT_MM` = (2000, 210), douve `MOAT_NEAR_MM` = (2179, 429).
2. `gotoTablePoint()` les convertit dans le repère de l'odométrie (départ en (0, 0)) pour un PAMI GAUCHE :
   on retire le point de départ, et y change de signe, car pour GAUCHE le centre est à droite.
3. `MOAT_STEPS` : tout droit jusqu'à (1900, 0), virage d'environ 51° vers le centre, environ 283 mm
   jusqu'à la douve, puis `defend()`.
4. Le PAMI s'arrête quand l'**odométrie** dit qu'il est à moins de 30 mm du point. Plus de capteurs de
   sol : trop de lignes sur la table pouvaient le tromper (D012). La précision vient donc de l'étalonnage
   (diamètre des roues, entraxe : banc `test_trajectoire`).
Le trajet s'essaie au banc `test_parcours`, commande `d`. À 350 mm/s (D019), il dure environ 8,5 s, pour
9,5 s de mouvement au labo (16 s à l'ancienne vitesse de 150 mm/s).

**DROITE est le miroir de GAUCHE** (`forSide()`) : y devient −y et les caps changent de signe
(règle d'or n° 5). Pour le PAMI DROITE, la douve est donc en (2079, 219) : le virage se fait vers la
gauche, toujours vers le centre.

**Comment une étape est jouée** (`logic/next_move.h`, fonction `decideNextMove()`) : on ne mémorise pas
« où on en était ». À chaque fois, on regarde la position et la cible :
- point atteint (à moins de 30 mm) ou cap atteint (à moins de 3°) → étape suivante ;
- mal orienté → tourner vers la cible ;
- bien orienté → avancer de la distance restante.

**C'est ce qui rend le contournement simple** : après un détour, le même calcul ramène le PAMI vers son
point.

**Logs** : `[MISS] Debut : 3 etapes, depart a GAUCHE`, `[MISS] Etape terminee | x = … y = … cap = …`,
`[MISS] Mission terminee`, `[MISS] Arrive : on reste ici, bras pret`.

**Si ça ne marche pas** :
- **Le PAMI fait de petits allers-retours de rotation à la fin d'une étape** → il dépasse la cible en
  freinant, puis corrige. Voir `HEADING_TOLERANCE_DEG` et `TURN_TOLERANCE_MM`.
- **Le PAMI va au mauvais endroit** → regarder les `x`, `y`, `cap` affichés : si l'odométrie croit être
  au bon endroit, c'est un réglage de `motion` (§ 4.9) ; sinon, c'est la mission dans `missions.h` ou
  `config.h`.
- **Il part du mauvais côté (miroir inversé)** → la commande de départ : `go g` ou `go d` ? Lire `cote`
  dans la ligne `[MAIN]`.

### 4.11 `state_machine.*` : la machine à états
**Rôle** : décider ce que fait le PAMI à chaque instant. C'est le « cerveau ».

**Les états** (`logic/pami_state.h`) :
| État | Le PAMI… | En sort quand… |
|---|---|---|
| `WAIT_START` | est allumé, immobile, attend le départ | la commande `go g` ou `go d` est reçue → `WAIT_DELAY` |
| `WAIT_DELAY` | attend 5 s, immobile | 5 s passées → `MISSION` |
| `MISSION` | joue sa mission | obstacle en ligne droite → `AVOID_STOP` ; étape « rester ici » → `DEFEND` |
| `AVOID_STOP` | est arrêté devant un obstacle | l'obstacle part → `MISSION` ; toujours là après 1,5 s → `DETOUR` (2 contournements par match au plus) |
| `DETOUR` | contourne par le centre de la table (à droite pour un PAMI GAUCHE, à gauche pour un PAMI DROITE) : quart de tour, 200 mm de côté, quart de tour retour (parallèle à sa route), pause de 0,3 s, regard ; voie libre sur plus de 300 mm → il avance de 300 mm pour dépasser l'obstacle, sinon un pas de côté de plus (2 au plus). Logique dans `logic/detour.h` (D016) | obstacle dépassé → `MISSION`, qui vise de nouveau son point ; toujours bloqué, ou obstacle pendant une ligne droite → `AVOID_STOP` |
| `DEFEND` | est immobile dans la douve ; un adversaire à moins de 60 mm → le bras attaque (120°, 3 s, retour, D021) | — |
| `END` | est arrêté pour toujours (14,5 s) | — |
| `FAULT` | est arrêté pour toujours (un ultrason en panne ou une roue bloquée) | redémarrage de la carte |

**Les priorités, à chaque tour, avant tout le reste** :
1. le temps est fini → `END`, quel que soit l'état (règle d'or n° 3) ;
2. un capteur ou une roue est en panne → `FAULT`.

**Un obstacle ne compte que si le PAMI avance vers lui** (`stopIfObstacle()`) : pendant une rotation sur
place, on continue.

**Logs** : `[FSM] WAIT_START -> WAIT_DELAY`, `[FSM] Contournement 1 par la droite (vers le centre)`,
`[FSM] Contournement : pas de cote (200 mm)`, `[FSM] Contournement : voie libre (2000 mm), on depasse l'obstacle (300 mm)`,
`[FSM] Contournement reussi : reprise de la mission`…

**Si ça ne marche pas** : lire la suite des lignes `[FSM]`. Elles racontent exactement ce que le PAMI a
décidé, et dans quel ordre.

### 4.12 `arm.*` : le bras
**Rôle** : piloter le servo SG90 du bras (GPIO 13), directement avec le générateur PWM de l'ESP32
(canal 4, 50 Hz), sans bibliothèque (D008).

| Fonction | Ce qu'elle fait |
|---|---|
| `begin()` | Pas de signal : le servo ne bouge pas avant la défense |
| `retract()` | Bras en position de départ (0°), à l'arrivée dans la douve. **Seulement entre 5 s et 14,5 s** |
| `defend(adversaireProche, nowMs)` | Appelé à chaque tour en `DEFEND` : attaque à 120°, tenue 3 s, retour à 0°, puis prêt pour une nouvelle attaque (`logic/arm_attack.h`, D021) |
| `isAttacking()` | Vrai pendant une attaque (bras à 120°) |
| `release()` | Coupe le signal pour toujours : le bras ne bouge plus |
| `update()` | Coupe le signal dès la fin du temps, même si la machine à états l'oubliait |

**Comment ça marche** : un servo attend une impulsion toutes les 20 ms. Sa largeur donne l'angle :
500 µs = 0°, 2400 µs = 180° (calcul dans `logic/servo_pulse.h`, partagé avec le banc `test_bras`).
La séquence d'attaque (`logic/arm_attack.h`) compte le temps avec `millis()`, sans attendre : prête →
attaque (3 s) → retour (0,5 s laissées au servo) → prête. Une attaque commencée va au bout.
⚠ Règles 2027, E.4 : l'attaque ne compte que si le bras touche l'adversaire **à la fin du match** (D021).

**Logs** : `[ARM] Attaque 1 : bras a 120 deg pendant 3.0 s`, `[ARM] Retour a 0 deg`,
`[ARM] Pret pour une nouvelle attaque`, `[ARM] Servo coupe (bras au repos).` (à chaque fin de match).

## 5. Le dossier `logic/` en un coup d'œil
Calculs purs, sans matériel, testables sur PC.

| Fichier | Contenu | Utilisé par |
|---|---|---|
| `pami_state.h` | Liste des états et leurs noms pour les logs | `state_machine`, `main` |
| `wheel_command.h` | `WheelCommand` : pourcentage gauche et droit | `motion`, `safety` |
| `match_clock.h` | `isMatchOver()`, `isInMovePhase()`, `elapsedSinceMs()` | `match_timer` |
| `start_command.h` | `StartCommandDetector` : reconnaît « go g » / « go d » + Entrée, donne le côté, un seul départ | `match_timer` |
| `median3.h` | Médiane de 3 valeurs | `sensors` |
| `rise_filter.h` | `RiseFilter` : une distance qui s'allonge d'un coup doit être confirmée | `sensors` |
| `safety_filter.h` | `isObstacleAhead()`, `isTurnInPlace()`, `filterCommand()` | `safety` |
| `speed_profile.h` | `nextProfileSpeed()` : accélération, croisière, freinage | `motion` |
| `speed_controller.h` | `SpeedController` : vitesse voulue → PWM | `motion` |
| `odometry.*` | `Odometry` : position (x, y, cap) ; `normalizeDeg()` | `motion`, `next_move` |
| `mission_step.h` | Types d'étapes, côté de départ, miroir `forSide()` | `mission` |
| `missions.h` | Les 4 missions, écrites pour un PAMI GAUCHE ; `gotoTablePoint()` (repère de la table → odométrie) | `mission`, banc `test_parcours` |
| `next_move.h` | `decideNextMove()` : DONE, TURN ou DRIVE | `mission` |
| `avoidance.h` | `chooseDetourSide()` : côté du détour, vers le centre de la table | `state_machine` |
| `detour.h` | `Detour` : le contournement pas à pas (quart de tour, pas de côté, retour, regard, dépassement) | `state_machine`, banc `test_parcours` |
| `arm_attack.h` | `ArmAttack` : séquence du bras (attaque 120°, tenue 3 s, retour, prêt) | `arm`, banc `test_bras` |
| `servo_pulse.h` | `servoDutyForAngle()` : angle du servo → rapport cyclique PWM | `arm`, banc `test_bras` |

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
| Reste en `WAIT_START` | `match_timer` (§ 4.7) | Moniteur connecté ? `go g` ou `go d` puis Entrée ? Message `Il manque le cote` ou `Commande inconnue` ? |
| `FAULT` environ 1 s après l'allumage, `[SENS] PANNE ultrason` | `sensors` (§ 4.5) | Alim 5 V du HC-SR04, masse commune, pont sur ECHO |
| `[MOT] PANNE` au début d'un mouvement | `motion` (§ 4.9) | Ticks avant la panne : 0 → moteurs non alimentés ; à l'envers → encodeur inversé ou croisé |
| PWM affichée mais roues immobiles, LED du MDD3A éteintes | `motor_driver` (§ 4.3) | 5V0 ↔ GND du MDD3A : 5 V ? VB+ ↔ VB− : 6 V ? |
| Une roue ne tourne que dans un sens | `motor_driver` (§ 4.3) | Le fil de l'entrée A ou B de cette roue |
| Ticks qui bougent sans mouvement | `encoders` (§ 4.4) | Masse des encodeurs = masse de l'ESP32 |
| S'arrête sans obstacle, `[SAFE] US G : obstacle devant` | `sensors`, `safety` | Valeurs `US G` / `US D` : un ultrason voit-il le sol ? |
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
- **Tick** : un front compté sur la voie A d'un encodeur (environ 0,097 mm de roue).
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

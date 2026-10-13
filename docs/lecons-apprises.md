# Leçons apprises sur le PAMI

> Les erreurs et pièges rencontrés pendant les séances sur le vrai PAMI, pour ne pas les refaire.
> Une entrée = un symptôme, sa cause, la solution, et comment vérifier.
> Ajoutez une entrée à chaque nouveau piège.

## Checklist avant chaque séance
1. **Toutes les masses sur une seule rangée** de la breadboard :
   (−) de l'alim de labo, VB− et GND du MDD3A, GND de l'ESP32, GND des encodeurs et du HC-SR04.
2. **Le MDD3A est alimenté** : 6 V entre VB+ et VB−, et environ 5 V entre 5V0 et GND.
3. **Rien sur D0, D2, D12, D15** (broches de démarrage de la carte). Attention : D12 est collée à D13 et D14.
4. **Chaque roue tournée vers l'avant à la main fait monter SON compteur** (`ticks G` pour la gauche,
   `ticks D` pour la droite). À vérifier avant toute commande de mouvement.
5. **PAMI sur cales** pour le premier essai de chaque commande.
6. **Arrêt d'urgence à portée de main** : BAU, ou à défaut le bouton de sortie de l'alim de labo.
7. **Alimentation des moteurs coupée pendant les téléversements.**

## Téléverser et lire le moniteur série

### La carte refuse le téléversement
- **Symptôme** : `Wrong boot mode detected (0x13)! The chip needs to be in download mode.`
- **Cause** : notre carte ne passe pas toute seule en mode téléversement (déjà vu en T001).
- **Solution** : maintenir **BOOT** dès que `Connecting...` s'affiche, et le relâcher **dès que
  `Writing at 0x...` apparaît**.

### Le programme ne démarre pas après le téléversement
- **Symptôme** : le moniteur affiche `waiting for download`.
- **Cause** : BOOT était encore enfoncé au redémarrage. La carte attend un nouveau téléversement.
- **Solution** : relâcher BOOT, puis appuyer une fois sur **EN**.
- **Si ça revient sans toucher BOOT** : un fil tire GPIO 0 vers la masse. Rien ne doit être branché sur GPIO 0.

### Le téléversement s'arrête en plein milieu
- **Symptôme** : `Packet content transfer stopped (received 8 bytes)`.
- **Cause** : coupure de la liaison USB pendant l'envoi, souvent à cause de parasites.
- **Solution** : relancer. Couper l'alimentation des moteurs pendant les téléversements.

### Le moniteur série ne s'ouvre pas
- `pio run -t upload` téléverse seulement. Pour voir les messages :
  `pio device monitor`, ou en une seule commande `pio run -t upload -t monitor`.
- Appuyer sur **EN** pour voir les messages depuis le début.
- **Fermer le moniteur (Ctrl+C) avant de téléverser**, sinon le port COM5 est occupé.
- Des caractères illisibles (`����`) ou des lignes `rst:…`, `load:…` au démarrage : c'est normal.

### Faut-il téléverser à chaque fois ?
- **Non** : le programme reste dans l'ESP32, même débranché. Brancher, ouvrir le moniteur, appuyer sur EN.
- **Oui** après une modification du code (y compris `config.h`), ou après un banc de test
  (`test_moteurs`, `test_odometrie`) pour remettre le firmware de match : `pio run -t upload`.

## Alimentation et masse

### Les encodeurs comptent alors que rien ne bouge (faux ticks)
- **Symptôme** : roues immobiles, mais `ticks G` et `ticks D` montent de milliers par seconde
  (jusqu'à 30 000/s, soit l'équivalent de 15 tours de roue par seconde : physiquement impossible).
- **Cause** : **la masse des encodeurs n'était pas reliée à la masse de l'ESP32.** Les signaux A et B
  « flottaient » et l'ESP32 y voyait des fronts parasites.
- **Solution** : relier GND encodeurs = GND ESP32. Les compteurs sont restés à 0 au repos.
- **Pour vérifier** : au repos, les compteurs ne bougent pas. Multimètre en mode « bip » entre
  le GND de l'encodeur et le GND de l'ESP32 : ça sonne.

### Les roues ne tournent pas et les LED du MDD3A restent éteintes
- **Symptôme** : le programme envoie la commande (`gauche = +35 %`), rien ne tourne, LED éteintes.
  Dans `test_odometrie` : la PWM monte jusqu'à 86 %, 0 tick, puis `PANNE : roue bloquee`.
- **Cause** : **le MDD3A n'était pas alimenté.** L'USB alimente l'ESP32 et les capteurs, **jamais les
  moteurs**. Le MDD3A a besoin de sa propre alimentation, entre 4 et 16 V, sur VB+ et VB−.
- **Solution** : alim de labo réglée à 6 V et à environ 2 A, (+) sur VB+, (−) sur VB−, sortie activée.
- **Pour vérifier** :
  - **5V0 ↔ GND** du connecteur de commande du MDD3A : environ 5 V. C'est le meilleur témoin que
    le driver est vivant (on avait mesuré 0,1 mV) ;
  - **VB+ ↔ VB−** : environ 6 V (on avait mesuré 20 mV).

### La broche 5V0 du MDD3A
- C'est une **sortie** : le driver y fournit du 5 V. **Ne rien y brancher.** La laisser libre est normal.

### Le courant des moteurs ne passe pas par la breadboard
- Un moteur bloqué tire jusqu'à 2,8 A : trop pour les rails d'une breadboard.
- Alimentation de labo **directement dans le bornier à vis** du MDD3A, avec des fils épais.
- Relier le (−) de l'alim à la rangée de masse par **un seul fil**.

### Pile 9 V : non
- Une pile 9 V carrée (même rechargeable, 850 mAh) ne fournit pas les 2 à 3 A de pointe des moteurs :
  la tension s'effondre et l'ESP32 redémarre. 9 V dépasse aussi le maximum des FIT0450 (7,5 V).
- Recommandé : 2 éléments LiFePO4 (6,4 V), dispensés de sac ignifuge (règlement général, F.4).

## Moteurs et driver

### Une roue ne tourne que dans un sens
- **Symptôme** : roue gauche immobile avec `g` et `a` (avant), mais elle tourne avec `r` (arrière).
- **Cause** : avancer utilise l'entrée **M1A** (GPIO 25), reculer l'entrée **M1B** (GPIO 26).
  Seul le fil GPIO 25 → M1A n'arrivait pas.
- **Méthode** : un sens qui marche et l'autre non = un seul des deux fils de commande en cause.
- **Pour vérifier** : commande `x` (100 %) puis `g`, tension entre la broche M1A du MDD3A et GND :
  environ 3,3 V. Le bouton de test M1A du MDD3A permet de tester le driver sans l'ESP32.

## Encodeurs

### Niveau des signaux : branchement direct
- Mesuré : encodeur alimenté en 5 V, les voies A et B alternent entre 0 et 3,3 V.
  **Pas de pont diviseur, pas de pull-up** (`ENCODER_USE_INTERNAL_PULLUP = false`).
- Les encodeurs s'alimentent en **5 V**, jamais en 3,3 V.

### `PANNE : roue bloquee ou encodeur inverse` au démarrage d'un essai
- **Ce que veut dire ce message** : une roue reçoit beaucoup de PWM, mais son compteur ne suit pas
  (il ne bouge pas, ou il part dans l'autre sens). Le programme coupe tout au bout de 0,6 s.
  C'est une sécurité : sans elle, le régulateur pousserait jusqu'à 100 %.
- **Causes rencontrées** :
  - **0 tick** : le MDD3A n'était pas alimenté (voir plus haut) ;
  - **une roue part brusquement, l'autre ne bouge pas** : encodeurs **croisés**, l'encodeur de la roue
    droite était lu comme le gauche. Le régulateur gauche croyait sa roue trop rapide, le droit poussait
    à 100 %.
- **Solution** :
  - encodeurs croisés → échanger les fils : moteur gauche A/B sur **D27/D14**, moteur droit A/B
    sur **D13/D4** (gauche et droite vues depuis l'arrière du PAMI) ;
  - un compteur qui **descend** quand sa roue avance → `ENCODER_LEFT_INVERTED` ou
    `ENCODER_RIGHT_INVERTED` à `true` dans `config.h`. C'est fréquent : les moteurs sont montés
    en miroir.
- **Pour vérifier** : point 4 de la checklist, roue par roue, en notant quel compteur bouge et dans
  quel sens.

## ESP32

### La carte redémarre puis n'arrive plus à démarrer
- **Symptôme** : `rst:0x1 (POWERON_RESET)`, puis des lignes absurdes
  (`load:0xd0012000,len:12586028`, une longue liste `mmu set …`), puis `rst:0x8 (TG1WDT_SYS_RESET)`.
- **Cause** : l'ESP32 n'arrive plus à lire sa mémoire flash au démarrage. Cause la plus fréquente :
  **GPIO 12 à l'état haut au démarrage**, par exemple un fil d'encodeur planté sur D12 au lieu de D13
  ou D14. Le plantage dépend alors de la position de la roue, ce qui le rend aléatoire.
  Autre cause possible : une chute de tension au démarrage des moteurs.
- **Solution** : rien sur D12 (ni D0, D2, D15). Courant des moteurs hors breadboard et hors USB.
  Condensateur de 100 µF entre VB+ et VB− (bande blanche = patte −).

### Broches voisines faciles à confondre
- D25 est entre D33 et D26 ; D12 est entre D14 et D13. Toujours relire le nom imprimé sur la carte.
- Deux fils dans la même rangée de breadboard sont reliés entre eux.

## Ultrason HC-SR04 : non résolu

### Pas de mesure, le firmware de match passe en `FAULT`
- **Symptôme** : `US 0 mm`, puis `[SENS] PANNE ultrason : aucune mesure depuis 1 s.` et `FAULT`.
  C'est voulu : sans ultrason, pas d'évitement, donc pas de mouvement (règle d'or n° 1).
- **Mesures faites** :
  - ECHO ↔ GND : environ 1,05 V en moyenne, donc le module reçoit bien l'ordre de mesure (VCC, GND
    et TRIG corrects) ;
  - mais cette tension **ne change pas** quand la main approche : le module ne voit jamais d'écho ;
  - point milieu du pont : 0,23 V, environ 1/5 d'ECHO au lieu des 2/3 attendus (valeurs des résistances
    vérifiées et bonnes).
- **Piste à vérifier en premier** : ces mesures ont été faites **avant** de relier toutes les masses.
  Une masse non commune peut expliquer ces deux anomalies. Refaire les mesures avec la masse commune.
- **Ensuite** : VCC du module ≥ 4,8 V mesuré sur ses pattes ; sinon un autre module.

### Mesurer une impulsion au multimètre
- Le multimètre en continu affiche une **moyenne**. Sur ECHO, la moyenne doit **baisser** quand un
  obstacle approche (l'écho revient plus vite). Au point milieu du pont, on doit lire les 2/3 d'ECHO.

## Méthode
- **Le matériel avant le code.** Aujourd'hui, tous les problèmes venaient du câblage ou de
  l'alimentation, aucun du code. Ordre de recherche (skill `banc-de-test`) :
  1. alimentation, mesurée sur le composant lui-même ;
  2. masse commune ;
  3. câblage conforme à `docs/materiel.md` ;
  4. niveaux logiques (3,3 V ou 5 V) ;
  5. composant seul, avec un banc de test ;
  6. seulement ensuite, le code.
- **Une seule hypothèse à la fois**, et une mesure pour la confirmer.
- **Pour faire rouler sans ultrason** : le banc `test_odometrie`
  (`pio run -e test_odometrie -t upload -t monitor`), jamais le firmware de match sans évitement.
  Après le banc, remettre le firmware : `pio run -t upload`.
- **Le BAU n'est pas encore câblé.** Sur cales, garder la main sur le bouton de sortie de l'alim.
  **Câbler le BAU avant tout essai au sol** (en série sur le fil du +, contact NC).
  Un BAU enfoncé reste verrouillé : il faut le tourner ou le tirer pour le relâcher.

## État à la fin de la séance du 13/10/2026
- **Fonctionne** : téléversement (avec BOOT), moniteur, encodeurs sans faux ticks (masse commune),
  driver alimenté, les deux roues dans les deux sens avec `test_moteurs`.
- **En cours** : encodeurs croisés à décroiser, puis sens de chaque compteur à vérifier roue par roue.
- **Pas encore fait** : réglage de l'odométrie (ligne droite, rotation, carré), câblage du BAU.
- **Non résolu** : l'ultrason ne mesure rien (refaire les mesures avec la masse commune).

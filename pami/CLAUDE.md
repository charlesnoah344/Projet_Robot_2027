# Firmware du PAMI

ESP32, C++ avec le framework Arduino, projet PlatformIO. Brochage du 25/11/2026 (D018, `docs/materiel.md`).
Objectif actuel : atteindre **la douve de son côté** par l'odométrie (D017). Le PAMI GAUCHE va à la
douve de gauche, le PAMI DROITE à celle de droite. C'est évalué d'abord au Mechatronics Lab.
Positions des douves : `docs/douves-coordonnees.md`. Plan d'origine et ordre des étapes :
`docs/plan-logiciel-pami.md` (écrit pour le défenseur, en partie dépassé par D012 à D014).
Pour écrire ou modifier du code ici, suis la skill `firmware-pami`.

> ## ⚠ Profil labo D007 / D013 — À REMPLACER AVANT TOUT MATCH EUROBOT
> Le minuteur suit la grille du labo, pas Eurobot :
> - départ : commande `go g` (PAMI GAUCHE) ou `go d` (PAMI DROITE) tapée dans le moniteur (Wi-Fi ou
>   USB, D013, D020), jamais automatique à l'allumage. Elle donne aussi le côté ;
> - immobile jusqu'à 5 s après le départ, arrêt total à 14,5 s.
> Toutes les autres règles d'or s'appliquent : évitement toujours actif, commandes moteurs via `safety`.
>
> Avant tout match Eurobot ou homologation :
> - départ par un cordon (≥ 500 mm) ou par le robot principal : le départ par Wi-Fi est **interdit**
>   (règlement général F.6). Le PCB n'a pas de broche prévue pour un cordon ;
> - `MATCH_MOVE_ALLOWED_MS = 85300` et `MATCH_STOP_MS = 99500` ;
> - `WIFI_ON_DURING_MATCH = false` : radio coupée pendant le match (règlement général F.6, D010) ;
> - test au chronomètre ;
> - D007 et D013 marquées « remplacées ».

## Ce que le PAMI doit respecter
Résumé de : règlement général F.7 et règles 2027 E.4. Détails dans `docs/`.
- Dimensions :
  - au départ : hauteur ≤ 150 mm, posé uniquement sur la table ;
  - plus grand qu'un cube de 100 mm ;
  - périmètre ≤ 600 mm au départ, ≤ 700 mm une fois déployé ;
  - altitude ≤ 350 mm pendant le match ;
  - masse ≤ 1,5 kg ;
  - une zone libre de 30 × 30 mm pour l'autocollant.
- Arrêt d'urgence et évitement obligatoires, comme pour le robot principal.
- Démarrage par un cordon de 50 cm, ou par le robot principal pendant le match.
- Reste dans l'écurie jusqu'à 85 s. S'il sort avant, il est retiré de la table.
- Actions 2027, comptées seulement entre 85 s et la fin du match :
  - atteindre une douve du château adverse ;
  - attaquer un PAMI adverse avec un actionneur de couleur distincte, sans le dégrader ;
  - lancer un boulet (1 boulet maximum par PAMI).

## Architecture en couches
```
src/
  main.cpp          setup() + loop() : appelle chaque couche, sans jamais bloquer
  config.h          broches (recopiées de docs/materiel.md), seuils, vitesses, durées, douves
  match_timer.*     minuteur global : départ et côté (commande « go g » / « go d », D013, D020), mouvement de 5 s
                    à 14,5 s, arrêt total à 14,5 s (profil labo D007)
  motor_driver.*    bas niveau du driver MDD3A (PWM 20 kHz, frein). Seul safety l'appelle
  encoders.*        comptage des ticks des 2 encodeurs par le compteur matériel PCNT, avec filtre
                    anti-parasites (D022)
  sensors.*         2 ultrasons avant (gauche, droit), un TRIG chacun, mais ils émettent ensemble (D018).
                    Médiane des 3 dernières mesures, filtre de montée, détection de panne (D012, D015)
  motion.*          déplacements mesurés par les encodeurs (voir ci-dessous)
  safety.*          filtre de sécurité : dernière étape avant le driver moteurs. Obstacle si
                    l'ultrason gauche OU le droit voit quelque chose (D012)
  mission.*         liste d'étapes du trajet (points de passage) + miroir selon le côté.
                    Mission MOAT : point de virage puis douve de son côté (D017)
  side.*            côté du PAMI : bord de table à sa gauche ou à sa droite (D017), donné par la
                    commande de départ, puis fixe jusqu'à la fin (D020)
  arm.*             bras (servo SG90) : attaque à 120°, 3 s, retour à 0° (D021), détaché à la fin
  state_machine.*   machine à états (voir ci-dessous)
  console.*         objet Console : remplace Serial pour les messages et les touches (USB + Wi-Fi)
  wireless.*        Wi-Fi (D010) : point d'accès 192.168.4.1, moniteur telnet, téléversement sans
                    fil accepté seulement PAMI immobile. Ne commande rien, ne bloque jamais loop()
  wifi_secrets.h    nom et mot de passe du réseau, ignoré par git (modèle : wifi_secrets.example.h)
  logic/            logique pure, sans aucun accès au matériel : filtres, rampe, régulateur,
                    odométrie, décisions, transitions. Testable sur PC.
test/               tests automatiques de logic/, exécutés sur PC (pio test -e native)
test_bench/         programmes de test isolés par composant (voir la skill banc-de-test)
```

**Rôle des encodeurs dans `motion`** (D004)
- Asservissement de vitesse : un régulateur par roue compare la vitesse mesurée à la vitesse voulue
  et corrige la PWM. Les deux roues tournent ainsi à la même vitesse, même si les moteurs
  ne sont pas identiques.
- Ligne droite : l'écart entre les distances parcourues par la roue gauche et la roue droite indique
  que le PAMI tourne. On corrige les consignes des deux roues pour annuler cet écart.
- Distances mesurées : ticks × `LEFT_MM_PER_TICK` ou `RIGHT_MM_PER_TICK` : chaque roue a son
  diamètre effectif, sinon le PAMI dérive en ligne droite sans le voir (D023).
- Angles mesurés : (distance droite − distance gauche) / `WHEEL_BASE_MM`.
- Rampes d'accélération : pas de patinage, pas de pic de courant.
- Arrêt complet entre deux mouvements (D024) : cible atteinte, frein, puis `isDone()` seulement quand
  les deux roues sont arrêtées. Une rotation qui démarre pendant une glissade se bloque (T027).
- Détection de blocage : une roue commandée qui ne tourne pas signale une panne (FAULT).
- Gyroscope : optionnel, plus tard.

**Machine à états** (détails dans `docs/plan-logiciel-pami.md`, § 3)
- `WAIT_START → WAIT_DELAY (5 s) → MISSION ⇄ AVOID_STOP / DETOUR → DEFEND`,
  plus `END` (14,5 s) et `FAULT` (panne).
- `DETOUR` contourne toujours par le centre de la table (D016, `logic/detour.h`) : quart de tour, pas de
  côté, retour parallèle à la route, regard, puis dépassement de l'obstacle avant de viser de nouveau
  le point. Par la droite pour un PAMI GAUCHE, par la gauche pour un PAMI DROITE (D014, D017).
- `DEFEND` : arrivé dans la douve, immobile. Un adversaire à moins de 60 mm : le bras attaque
  (120°, 3 s, retour, `logic/arm_attack.h`, D021).
- Pas de marche arrière : les capteurs ne regardent que vers l'avant, et `safety` refuse le recul.

Chemin des commandes moteur : `state_machine → motion → safety → motor_driver`.
Aucun code du firmware ne pilote le driver sans passer par `safety`.
Seuls les programmes de `test_bench/`, qui ne servent jamais en match, appellent `motor_driver` directement.
Le minuteur est vérifié à chaque tour de `loop()` et a priorité sur tous les états.

**Environnements PlatformIO**
- `esp32dev` : le firmware, environnement par défaut de `pio run` et `pio run -t upload`.
- `test_*` : un banc de test par composant.
  - Compiler : `pio run -e test_xxx`.
  - Téléverser : `pio run -e test_xxx -t upload`.
  - Après un banc de test, **toujours re-téléverser le firmware** (`pio run -t upload`).
  - `test_parcours` : aller à un point tapé au moniteur (`p 600 -200`), 10 s d'attente, évitement et détour
    vers le centre comme en match, mais avec **un seul ultrason** (choisi dans `config.h`) et le côté choisi par `c g` / `c d`
    (`TEST_BENCH_PARCOURS_START_ON_LEFT`). Commande `d` : le trajet de la douve du match (`MOAT_STEPS`, D017).
    Banc seulement : le firmware de match exige les deux ultrasons.
  - `test_bras` : le servo d'attaque seul (repos, attaque 120° / 3 s / retour, angle au choix, mode
    défense avec les ultrasons), roues freinées (D021).
- `*_wifi` (D010) : les mêmes programmes, téléversés et lus sans câble, PC connecté au réseau du PAMI.
  `pio run -e esp32dev_wifi -t upload`, `pio device monitor -e esp32dev_wifi`.
  Tout nouveau programme pour l'ESP32 doit inclure `${wireless.src}` et appeler `wireless::begin()`
  et `wireless::update()` : sans ça, une fois téléversé, il couperait la liaison sans fil.
- Les messages s'écrivent avec `Console` (pas `Serial`), qui les envoie sur l'USB et sur le Wi-Fi.
- `native` : tests de `logic/` sur PC (nécessite g++, par exemple MinGW-w64 sous Windows).
- `logic_esp32` : les mêmes tests, exécutés sur l'ESP32 branché en USB (`pio test -e logic_esp32`).
  Utile quand g++ n'est pas installé.

## État d'avancement
Coche une étape uniquement après un test réussi sur le vrai PAMI, consigné dans `docs/journal-tests.md`.
Le code des étapes A à H est écrit avant les tests (choix de l'équipe) : « écrit » ne veut pas dire
« validé ». Ce qu'il faut tester à chaque étape : `docs/plan-logiciel-pami.md`, § 9.
- [x] 1. Projet PlatformIO + squelette des couches : compilation OK, minuteur et bouton BOOT (T001)
- [ ] 2. Moteurs + driver : sens de rotation OK (T002), reste la zone morte, l'arrêt net et le BAU
- [ ] Brochage du 25/11/2026 (D018) : sens des moteurs (`test_moteurs`) et des encodeurs (`test_odometrie`)
      revérifiés ; ultrasons gauche / droit pas inversés ; ils ne s'entendent pas (`test_ultrason`, protocole 6)
- [ ] A. Départ par la commande `go g` / `go d` (D013, D020) + minuteur labo 5 s / 14,5 s
- [ ] B. Encodeurs Hall : comptage, ticks par tour de roue, mm par tick
- [ ] C. 2 ultrasons + filtre de sécurité : arrêt devant un obstacle vu par G seul, puis par D seul,
      distance de freinage
- [ ] D. `motion` : ligne droite régulée, rotation mesurée
- [ ] E. Odométrie + mission « carré »
- [ ] F. Évitement : arrêt, détour vers le centre de la table, contournement
- [ ] G. Côté gauche / droite par la commande de départ (D020) + miroir (D014)
- [ ] H. Mission MOAT : douve de son côté atteinte (D017), GAUCHE et DROITE, bras qui touche l'adversaire.
      D'abord au banc `test_parcours` (commande `d`). Vitesse relevée à 350 mm/s (D019) : remesurer
      l'anticipation (`test_pid`), l'étalonnage (`test_trajectoire`) et la distance d'arrêt
- [ ] Arrêt d'urgence câblé et testé
- [ ] **Profil Eurobot : départ par cordon ou par le robot principal + attente de 85,3 s
      (remplace D007 et D013) — obligatoire avant tout match Eurobot**
- [ ] (Optionnel) Gyroscope : lecture du cap, dérive mesurée

## Valeurs calibrées
Les valeurs mesurées pendant les tests sont reportées dans `config.h`.
Leur origine est tracée dans `docs/journal-tests.md`.

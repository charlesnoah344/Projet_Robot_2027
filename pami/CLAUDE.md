# Firmware du PAMI

ESP32, C++ avec le framework Arduino, projet PlatformIO.
Objectif actuel : le PAMI **défenseur** évalué au Mechatronics Lab. Plan complet, schéma de câblage
et ordre des étapes : `docs/plan-logiciel-pami.md`.
Pour écrire ou modifier du code ici, suis la skill `firmware-pami`.

> ## ⚠ Profil labo D007 — À REMPLACER AVANT TOUT MATCH EUROBOT
> Le minuteur suit la grille du labo, pas Eurobot :
> - départ : bouton-poussoir externe sur GPIO 16, jamais automatique à l'allumage ;
> - immobile jusqu'à 5 s après le départ, arrêt total à 14,5 s.
> Toutes les autres règles d'or s'appliquent : évitement toujours actif, commandes moteurs via `safety`.
>
> Avant tout match Eurobot ou homologation :
> - tirette (cordon ≥ 500 mm) sur GPIO 16, à la place du bouton ;
> - `MATCH_MOVE_ALLOWED_MS = 85300` et `MATCH_STOP_MS = 99500` ;
> - test au chronomètre ;
> - D007 marquée « remplacée ».

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
  config.h          broches (recopiées de docs/materiel.md), seuils, vitesses, durées
  match_timer.*     minuteur global : départ (bouton GPIO 16, puis tirette), mouvement de 5 s
                    à 14,5 s, arrêt total à 14,5 s (profil labo D007)
  motor_driver.*    bas niveau du driver MDD3A (PWM 20 kHz, frein). Seul safety l'appelle
  encoders.*        comptage des ticks des 2 encodeurs (interruptions), conversion en mm
  sensors.*         ultrason + 2 IR, filtrage (médiane des 3 dernières mesures), détection de panne
  motion.*          déplacements mesurés par les encodeurs (voir ci-dessous)
  safety.*          filtre de sécurité : dernière étape avant le driver moteurs
  mission.*         liste d'étapes du trajet (points de passage) + miroir selon la couleur
  color.*           sélecteur de couleur, lu avant le départ puis verrouillé
  arm.*             bras du défenseur (servo SG90), détaché à la fin
  state_machine.*   machine à états (voir ci-dessous)
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
- Distances mesurées : ticks × `MM_PER_TICK`.
- Angles mesurés : (distance droite − distance gauche) / `WHEEL_BASE_MM`.
- Rampes d'accélération : pas de patinage, pas de pic de courant.
- Détection de blocage : une roue commandée qui ne tourne pas signale une panne (FAULT).
- Gyroscope : optionnel, plus tard.

**Machine à états** (détails dans `docs/plan-logiciel-pami.md`, § 3)
- `WAIT_START → WAIT_DELAY (5 s) → MISSION ⇄ AVOID_STOP / DETOUR → DEFEND`,
  plus `END` (14,5 s) et `FAULT` (panne).
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
- `native` : tests de `logic/` sur PC (nécessite g++, par exemple MinGW-w64 sous Windows).
- `logic_esp32` : les mêmes tests, exécutés sur l'ESP32 branché en USB (`pio test -e logic_esp32`).
  Utile quand g++ n'est pas installé.

## État d'avancement
Coche une étape uniquement après un test réussi sur le vrai PAMI, consigné dans `docs/journal-tests.md`.
Le code des étapes A à H est écrit avant les tests (choix de l'équipe) : « écrit » ne veut pas dire
« validé ». Ce qu'il faut tester à chaque étape : `docs/plan-logiciel-pami.md`, § 9.
- [x] 1. Projet PlatformIO + squelette des couches : compilation OK, minuteur et bouton BOOT (T001)
- [ ] 2. Moteurs + driver : sens de rotation OK (T002), reste la zone morte, l'arrêt net et le BAU
- [ ] A. Bouton de départ GPIO 16 + minuteur labo 5 s / 14,5 s
- [ ] B. Encodeurs Hall : comptage, ticks par tour de roue, mm par tick
- [ ] C. Ultrason + filtre de sécurité : arrêt devant un obstacle, distance de freinage
- [ ] D. `motion` : ligne droite régulée, rotation mesurée
- [ ] E. Odométrie + mission « carré »
- [ ] F. Capteurs IR + évitement : arrêt, détour, contournement
- [ ] G. Sélecteur de couleur + miroir
- [ ] H. Mission défenseur : zone atteinte, bras qui touche l'adversaire
- [ ] Arrêt d'urgence câblé et testé
- [ ] **Profil Eurobot : tirette + attente de 85,3 s (remplace D007) — obligatoire avant tout match Eurobot**
- [ ] (Optionnel) Gyroscope : lecture du cap, dérive mesurée

## Valeurs calibrées
Les valeurs mesurées pendant les tests sont reportées dans `config.h`.
Leur origine est tracée dans `docs/journal-tests.md`.

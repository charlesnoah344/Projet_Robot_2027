# Firmware du PAMI

ESP32, C++ avec le framework Arduino, projet PlatformIO.
Objectif actuel : un PAMI qui se déplace précisément et évite bien les obstacles.
Pour écrire ou modifier du code ici, suis la skill `firmware-pami`.

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

## Architecture en couches (cible)
```
src/
  main.cpp          setup() + loop() : appelle chaque couche, sans jamais bloquer
  config.h          broches (recopiées de docs/materiel.md), seuils, vitesses, durées
  match_timer.*     minuteur global : tirette, autorisation à 85 s, arrêt à 99,5 s
  sensors.*         ultrason + 2 IR, filtrage (médiane des 3 dernières mesures)
  motion.*          moteurs, rampes d'accélération, correction de cap (gyroscope)
  safety.*          filtre de sécurité : dernière étape avant le driver moteurs
  mission.*         liste d'étapes du trajet + miroir selon la couleur
  state_machine.*   INIT → WAIT_START → WAIT_85S → MISSION ⇄ AVOIDANCE → END
test/               tests automatiques de la logique, exécutés sur PC (pio test -e native)
test_bench/         programmes de test isolés par composant (voir la skill banc-de-test)
```

Chemin des commandes moteur : `mission → motion → safety → driver`.
Aucun code ne pilote le driver sans passer par `safety`.
Le minuteur est vérifié à chaque tour de `loop()` et a priorité sur tous les états.

## État d'avancement
Coche une étape uniquement après un test réussi sur le vrai PAMI, consigné dans `docs/journal-tests.md`.
- [ ] Projet PlatformIO créé, compilation OK
- [ ] Moteurs + driver : sens de rotation, vitesse minimale, arrêt
- [ ] Gyroscope : lecture du cap, dérive mesurée
- [ ] Aller droit sur 1 m et tourner de 90° avec précision
- [ ] Ultrason + IR : mesures fiables, filtrage
- [ ] Filtre de sécurité : arrêt devant un obstacle, reprise quand la voie est libre
- [ ] Arrêt d'urgence câblé et testé
- [ ] Tirette + minuteur (85 s / 99,5 s) + sélecteur de couleur
- [ ] Trajet réel sur une reproduction de la table, dans les deux couleurs

## Valeurs calibrées
Les valeurs mesurées pendant les tests sont reportées dans `config.h`.
Leur origine est tracée dans `docs/journal-tests.md`.

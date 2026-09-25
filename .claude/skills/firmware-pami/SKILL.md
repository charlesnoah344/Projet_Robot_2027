---
name: firmware-pami
description: Méthode et conventions pour écrire, modifier, relire ou déboguer le code C++ (framework Arduino, PlatformIO) de l'ESP32 du PAMI — moteurs, driver, capteurs ultrason et infrarouge, gyroscope, évitement d'obstacles, filtre de sécurité, minuteur de match, tirette, couleur, machine à états, trajets de mission. Utilise-la pour toute tâche de programmation dans le dossier pami/, même petite (ajouter un capteur, régler une vitesse, corriger un bug, créer config.h ou platformio.ini).
---

# Écrire le firmware du PAMI

Avant de coder, lis `pami/CLAUDE.md` (architecture, état d'avancement) et `pami/src/config.h` s'il existe.
Les broches viennent de `docs/materiel.md`. N'invente jamais une broche.

## Démarche
1. **Situer la demande** : quelle couche est concernée (sensors, motion, safety, mission, match_timer, state_machine) ?
2. **Plan court** : quels fichiers changent, et pourquoi.
   Si plus d'un fichier est touché, présente le plan et attends l'accord de l'équipe.
3. **Petits pas** : un changement testable à la fois.
4. **Compiler** avec `cd pami && pio run`. Ne rends jamais du code qui ne compile pas.
   Si PlatformIO n'est pas disponible, dis-le clairement.
5. **Expliquer** :
   - ce qui a changé, et pourquoi ce choix ;
   - comment le tester sur le vrai PAMI ;
   - ce qu'on doit observer si ça marche.
6. **Tenir à jour** :
   - `pami/CLAUDE.md` (avancement), seulement après un test réel réussi ;
   - `docs/decisions.md` si le choix est structurant.

## Règles de code

### Ne jamais bloquer la boucle
Pendant le match, `loop()` doit tourner vite (idéalement plusieurs centaines de fois par seconde).
Sinon l'évitement réagit en retard.
- Pas de `delay()` en dehors de `setup()`.
- Chaque tâche périodique utilise ce modèle :
  ```cpp
  const uint32_t now = millis();
  if (now - lastSensorReadMs >= SENSOR_PERIOD_MS) {  // soustraction : résiste au débordement de millis()
    lastSensorReadMs = now;
    sensors.update();
  }
  ```
  N'écris jamais `if (millis() > start + duree)` : cette forme casse au débordement de `millis()`.
- `pulseIn()` bloque la boucle.
  - Préfère une mesure par interruption sur la broche ECHO.
  - À défaut, mets un timeout court. Environ 12 000 µs, ce qui correspond à une portée d'environ 2 m.
- Ultrason : laisse au moins 60 ms entre deux mesures, pour éviter l'écho de la mesure précédente.
- Logs série : courts, limités en fréquence, et désactivables par un flag `DEBUG`.
  Un port série saturé ralentit la boucle.

### La sécurité passe avant tout
- Chemin unique des commandes : `mission → motion → safety → driver`.
  Seul `safety` appelle le driver moteurs.
- Au démarrage, la première chose que fait `setup()` : mettre les broches moteurs
  en sortie à l'état bas, pour que les moteurs soient arrêtés.
- Principe de sécurité par défaut : si un capteur est jugé en panne, on considère qu'il y a un obstacle.
  - « Rien dans la portée » (timeout sans écho) est normal.
  - « Capteur en panne » ne l'est pas : ECHO bloqué à l'état haut avant le déclenchement,
    ou aucune mesure valide pendant plus d'une seconde.
- **Aucun flag, `#ifdef`, mode ou paramètre ne doit permettre de désactiver l'évitement.**
  C'est une disqualification.
  - Pour tester les moteurs sans évitement, utilise un programme séparé dans `test_bench/`
    (skill `banc-de-test`), jamais le firmware de match.
  - Près de la cible (douve adverse, bordures), on réduit la vitesse et le seuil d'arrêt.
    On ne coupe jamais la détection.
- Le seuil d'arrêt dépend de la distance de freinage mesurée à la vitesse utilisée.
  Il n'est pas choisi « au feeling ».

### Minuteur de match
- La référence de temps est l'instant où la tirette est retirée.
  Le système n'est armé que si la tirette était en place au démarrage.
  Si elle est absente à l'allumage, le PAMI reste en INIT et le signale (LED, log).
- Constantes : `MATCH_MOVE_ALLOWED_MS = 85300` et `MATCH_STOP_MS = 99500`.
- Le minuteur est vérifié à chaque tour de `loop()`, avant la machine à états.
  À `MATCH_STOP_MS`, tout s'arrête, quel que soit l'état (y compris AVOIDANCE).
- À l'arrêt final :
  - moteurs à zéro ;
  - servomoteur détaché (plus de PWM) ;
  - plus aucun actionneur actif.
  Les afficheurs et LED peuvent rester allumés.
- Si le PAMI est démarré par le robot principal (ESP-NOW par exemple), la règle des 85 s
  s'applique quand même. Le PAMI doit la garantir lui-même, sans faire confiance à l'émetteur.

### Couleur et trajets
- Le sélecteur de couleur est lu avant le départ, puis verrouillé au retrait de la tirette.
- Les trajets sont écrits une seule fois, pour le bleu.
  Le jaune est obtenu par miroir : on inverse le signe des angles.
  Pour des coordonnées absolues, on inverse l'axe de symétrie de la table ;
  vérifie cet axe sur le plan en annexe du PDF.
- Une mission est une liste d'étapes (`DRIVE mm`, `TURN deg`, `WAIT ms`…).
  Ce n'est pas du code en dur éparpillé.

### Lisibilité
- Une couche par fichier `.h` / `.cpp`, une responsabilité par module, des fonctions courtes.
- Toutes les constantes dans `config.h`, avec l'unité dans le nom :
  `OBSTACLE_STOP_DISTANCE_MM`, `DRIVE_SPEED_PCT`, `SENSOR_PERIOD_MS`.
  Commente l'origine des valeurs calibrées (ex. `// T007`).
- Commentaires en français, qui expliquent le **pourquoi** plutôt que le quoi.
- Logs préfixés par module : `[SENS]`, `[MOT]`, `[SAFE]`, `[MISS]`, `[TIME]`, `[FSM]`.

### Logique testable sur PC
Garde la logique pure (filtre médian, miroir de couleur, transitions de la machine à états,
calculs du minuteur) sans appel direct au matériel. On peut alors la tester sur PC
avec `pio test -e native` (framework Unity, dossier `test/`).
Propose ces tests quand tu écris ou modifies cette logique.

## Pièges connus de l'ESP32
- **Broches** :
  - GPIO 6 à 11 sont interdites (flash) ;
  - GPIO 34 à 39 sont en entrée seule, sans pull-up interne ;
  - GPIO 0, 2, 12, 15 influencent le démarrage : évite-les pour les boutons et la tirette ;
  - les broches ADC2 ne font plus de mesure analogique quand le Wi-Fi ou ESP-NOW est actif.
- **Niveaux logiques** : l'ESP32 fonctionne en 3,3 V et n'accepte pas le 5 V.
  L'ECHO d'un HC-SR04 en 5 V passe par un pont diviseur.
- **PWM** : l'API LEDC a changé entre les versions 2.x et 3.x du core Arduino-ESP32.
  - 2.x : `ledcSetup` et `ledcAttachPin`.
  - 3.x : `ledcAttach` et `ledcWrite` par broche.
  Vérifie la version utilisée (platformio.ini, `pio pkg list`) avant d'écrire du code PWM.
- **Servomoteur** : utilise la bibliothèque `ESP32Servo`. La bibliothèque `Servo` standard ne marche pas sur ESP32.
- **Redémarrages quand les moteurs démarrent** : c'est une chute de tension (brown-out).
  Les pistes :
  - rampes d'accélération ;
  - condensateurs près du driver ;
  - alimentation logique séparée ou mieux régulée.
  Ce n'est pas un bug logiciel.
- **Gyroscope** : calibre le biais au repos au démarrage, PAMI immobile.
  Mesure la dérive sur 100 s pour savoir si elle est acceptable.

## Avant de rendre du code
- [ ] Compile sans erreur ni avertissement nouveau.
- [ ] Aucun `delay()` ni attente bloquante dans le chemin du match.
- [ ] Toutes les commandes moteurs passent par `safety`.
- [ ] Aucune façon de désactiver l'évitement.
- [ ] Minuteur prioritaire sur tout le reste.
- [ ] Constantes dans `config.h`, unités dans les noms.
- [ ] Procédure de test sur le vrai PAMI expliquée.

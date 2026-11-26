# Journal des décisions techniques

> Une entrée par décision importante. On ne supprime pas une décision dépassée :
> on la marque « remplacée par Dxxx ». Ce journal sert aussi à préparer le poster
> et les explications aux arbitres.

## Modèle
### Dxxx — Titre court
- **Date** :
- **Statut** : proposée / adoptée / remplacée par Dxxx
- **Contexte** : quel problème fallait-il résoudre ?
- **Options envisagées** : A, B, C, avec leurs avantages et inconvénients
- **Décision** : ce qu'on a choisi
- **Pourquoi** : les raisons principales
- **Conséquences** : ce que ça implique (matériel à acheter, code à écrire, risques)

---

### D001 — Langage et outils du firmware
- **Date** : 25/09/2026
- **Statut** : adoptée
- **Contexte** : il faut choisir un langage pour programmer l'ESP32 du PAMI.
- **Options envisagées** :
  - C++ avec Arduino : rapide, précis dans le temps, beaucoup de bibliothèques.
  - MicroPython : plus simple, mais plus lent, avec des pauses imprévisibles.
  - ESP-IDF : très puissant, mais trop complexe pour nous.
- **Décision** : C++ avec le framework Arduino, dans PlatformIO (VS Code).
- **Pourquoi** :
  - fiabilité temporelle (minuteur, évitement, correction de cap) ;
  - bibliothèques disponibles pour nos composants ;
  - PlatformIO gère un projet en plusieurs fichiers et s'utilise bien avec Git.
- **Conséquences** : installer VS Code et PlatformIO. Apprendre les bases du C++ côté équipe.

### D002 — Architecture logicielle en couches
- **Date** : 25/09/2026
- **Statut** : adoptée
- **Contexte** : le code doit rester fiable et compréhensible par toute l'équipe.
- **Décision** : couches séparées (capteurs, mouvement, sécurité, mission, minuteur),
  orchestrées par une machine à états et une boucle non bloquante.
- **Pourquoi** :
  - chaque couche se teste seule ;
  - la sécurité est une étape obligatoire avant les moteurs ;
  - le minuteur est indépendant de la stratégie.
- **Conséquences** : détail dans `pami/CLAUDE.md`.

### D003 — Arrêt d'urgence câblé sur la puissance moteurs
- **Date** : 25/09/2026
- **Statut** : adoptée
- **Contexte** : le règlement exige un arrêt immédiat. Un bug logiciel ne doit pas pouvoir l'empêcher.
- **Décision** : BAU en série sur l'alimentation du driver moteurs. L'ESP32 reste alimenté.
- **Conséquences** : lecture optionnelle de l'état du BAU sur une entrée, via un pont diviseur.

### D004 — Encodeurs pour les distances et la ligne droite, gyroscope optionnel plus tard
- **Date** : 26/09/2026 (proposée le 25/09/2026)
- **Statut** : adoptée
- **Contexte** :
  - le PAMI doit aller droit et mesurer ses déplacements ;
  - la première version de cette entrée supposait des moteurs sans encodeurs. En fait, les moteurs
    FIT0450 ont chacun un encodeur à effet Hall, à 2 voies.
- **Options envisagées** :
  - gyroscope seul : mesure bien le cap, mais pas la distance. Achat nécessaire ;
  - encodeurs seuls : déjà sur les moteurs. Mesurent la distance, la vitesse de chaque roue et
    l'angle de virage. Faussés si une roue glisse ;
  - encodeurs + gyroscope : le plus précis, mais plus de code et un achat.
- **Décision** :
  - encodeurs pour les distances, la ligne droite et les angles de virage ;
  - gyroscope optionnel, plus tard, si les virages dérivent trop.
- **Pourquoi** :
  - les encodeurs sont déjà là ;
  - ils mesurent la distance, ce qu'un gyroscope ne sait pas faire ;
  - ils permettent de réguler la vitesse de chaque roue ;
  - la correction de cap se fait par la différence des distances parcourues par les deux roues.
- **Conséquences** :
  - 4 broches A/B, et des ponts diviseurs si les signaux sont en 5 V (voir `docs/materiel.md`) ;
  - lecture par interruption sur les deux fronts de la voie A. À chaque front, on lit B pour
    connaître le sens de rotation ;
  - si des ticks se perdent, plan B : le compteur matériel PCNT de l'ESP32 ;
  - limite : les roues peuvent glisser, surtout en virage. Le gyroscope pourra compléter plus tard.
    Les broches I2C (GPIO 21/22) restent réservées.

### D005 — Exception temporaire au minuteur pendant le développement
- **Date** : 26/09/2026
- **Statut** : remplacée par D007
- **Contexte** : pendant le développement, attendre 85 s avant chaque essai ralentit beaucoup les tests.
  La tirette n'existe pas encore.
- **Décision**, pour le firmware de développement uniquement :
  - pas d'attente de 85 s : c'est une exception volontaire de l'équipe à la règle d'or n° 3 ;
  - départ par un appui sur le bouton BOOT de la carte (GPIO 0), jamais automatiquement à l'allumage ;
  - l'arrêt total à 99,5 s après le départ est conservé ;
  - toutes les autres règles d'or s'appliquent. En particulier, l'évitement reste toujours actif
    et toutes les commandes moteurs passent par `safety`.
- **Règles en jeu** :
  - règles 2027, E.4 : un PAMI qui sort de l'écurie avant la phase d'attaque (85 s) est retiré de la table ;
  - règlement général, F.6 « Démarrage » : seul le cordon d'au moins 500 mm est homologué.
    Un bouton ne l'est pas ;
  - règlement général, H.4 : faux départ −50.
- **Conséquences** :
  - le firmware affiche un avertissement à chaque démarrage ;
  - la constante `MATCH_MOVE_ALLOWED_MS` existe dans `config.h`, mais n'est pas encore utilisée ;
  - **avant tout match ou homologation**, il faudra :
    - la tirette ;
    - l'attente de 85,3 s ;
    - le test au chronomètre ;
    - puis passer cette décision à « remplacée ».
    La case correspondante est dans `pami/CLAUDE.md`.

### D006 — Brochage des moteurs adapté au driver MDD3A
- **Date** : 26/09/2026
- **Statut** : adoptée
- **Contexte** : le premier brochage prévoyait 3 broches par moteur (PWM + 2 sens), comme un L298N.
  Le MDD3A n'a que 2 entrées par moteur.
- **Décision** : 2 broches par moteur, gauche GPIO 25/26, droit GPIO 33/32. Commande :
  - A = PWM et B = 0 : avance ;
  - A = 0 et B = PWM : recule ;
  - A = B = 0 : frein.
- **Pourquoi** :
  - c'est le mode de commande du MDD3A (fiche Cytron) ;
  - le frein arrête les roues plus vite que la roue libre, ce qui raccourcit la distance d'arrêt
    devant un obstacle ;
  - PWM à 20 kHz, le maximum accepté par le driver, pour que les moteurs ne sifflent pas.
- **Conséquences** :
  - GPIO 27 et 13 sont libérées et servent aux encodeurs ;
  - un seul fichier (`motor_driver.cpp`) connaît cette logique.

### D007 — Profil labo du minuteur et bouton de départ externe
- **Date** : 09/10/2026
- **Statut** : adoptée — **profil labo, à remplacer par le profil Eurobot avant tout match Eurobot**.
  Le départ par bouton-poussoir (GPIO 16) est remplacé par la commande `go` (D013) ; les durées
  5 s / 14,5 s ne changent pas.
- **Contexte** :
  - le PAMI est d'abord évalué au Mechatronics Lab. La grille demande « démarrage après 5 s et arrêt
    après 15 s », et non 85 s / 100 s comme Eurobot ;
  - le bouton BOOT de la carte (départ provisoire de D005) n'est pas accessible sur le PAMI monté ;
  - la tirette n'est pas encore achetée.
- **Options envisagées** :
  - deux profils (labo et Eurobot) choisis à la compilation : complet, mais plus de choses à expliquer
    pour un seul usage aujourd'hui ;
  - profil labo seulement, Eurobot plus tard : simple ;
  - garder Eurobot : on perd le point « temps de déplacement » de la grille.
- **Décision** :
  - profil labo seulement : immobile jusqu'à 5 s après le départ (`MATCH_MOVE_ALLOWED_MS = 5000`),
    arrêt total à 14,5 s (`MATCH_STOP_MS = 14500`, 0,5 s de marge comme pour Eurobot) ;
  - les deux durées comptent depuis l'appui sur le bouton de départ ;
  - départ par un bouton-poussoir externe sur GPIO 16, avec la logique déjà testée en T001
    (armé seulement après avoir vu le bouton relâché, appui d'au moins 50 ms, un seul départ) ;
  - `safety` refuse toute commande moteur en dehors de la période 5 s – 14,5 s.
- **Pourquoi** :
  - c'est ce que la grille note, et c'est le moins de code ;
  - l'état `WAIT_START` sépare l'allumage du départ : un redémarrage de l'ESP32 ne relance jamais
    le PAMI ;
  - la tirette pourra prendre la même broche avec la même logique (« en place » = relâché,
    « retirée » = appuyé). Il n'y aura qu'une ligne à changer.
- **Règles en jeu** : règle d'or n° 3 (Eurobot : 85,3 s et 99,5 s), règlement général F.6
  (seul le cordon d'au moins 500 mm est homologué à Eurobot), règles 2027 E.4.
- **Conséquences** :
  - le firmware rappelle le profil labo à chaque démarrage ;
  - **avant tout match Eurobot** : tirette sur GPIO 16, `MATCH_MOVE_ALLOWED_MS = 85300`,
    `MATCH_STOP_MS = 99500`, test au chronomètre, puis cette décision passe à « remplacée » ;
  - à confirmer avec l'encadrant : les 15 s comptent-elles depuis le départ, ou depuis la fin des 5 s ?

### D008 — Servo du bras piloté directement par LEDC, sans ESP32Servo
- **Date** : 09/10/2026
- **Statut** : adoptée
- **Contexte** : le bras du défenseur est un servo SG90 sur GPIO 23. Le plan prévoyait la bibliothèque
  ESP32Servo. Or nos moteurs utilisent déjà les canaux PWM 0 à 3 de l'ESP32 (LEDC), configurés
  directement par `motor_driver`.
- **Options envisagées** :
  - ESP32Servo : pratique, mais elle choisit elle-même un canal PWM et ne sait pas que les canaux 0 à 3
    sont pris. Si elle prenait le canal d'un moteur, elle changerait sa fréquence et enverrait les
    impulsions du servo au moteur : le moteur pourrait tourner sans passer par `safety` ;
  - ESP32Servo en lui réservant un minuteur (`ESP32PWM::allocateTimer`) : possible, mais c'est un
    réglage caché, facile à oublier ou à casser ;
  - piloter le servo nous-mêmes avec LEDC, sur un canal choisi : une dizaine de lignes.
- **Décision** : `arm.cpp` pilote le servo avec LEDC, canal 4 (minuteur 2, distinct des minuteurs 0 et 1
  des moteurs), 50 Hz, 16 bits. Angle → largeur d'impulsion entre `SERVO_MIN_PULSE_US` et
  `SERVO_MAX_PULSE_US`.
- **Pourquoi** :
  - aucun risque qu'une bibliothèque touche aux canaux des moteurs ;
  - même API que `motor_driver` (core 2.x : `ledcSetup`, `ledcAttachPin`, `ledcWrite`) ;
  - le code est court et s'explique facilement.
- **Conséquences** :
  - pas de bibliothèque à ajouter dans `platformio.ini` ;
  - la règle de la skill `firmware-pami` (« utilise ESP32Servo ») ne s'applique pas à ce projet tant que
    les moteurs utilisent LEDC directement ;
  - le servo n'a aucun PWM avant la défense, ne bouge qu'entre 5 s et 14,5 s, et son PWM est coupé
    à la fin (règle d'or n° 3).

### D009 — Les IR regardent le sol pour trouver la zone ; l'évitement repose sur l'ultrason
- **Date** : 13/10/2026
- **Statut** : remplacée par D012 (plus d'IR, 2 ultrasons) et D014 (la douve est trouvée par l'odométrie)
- **Contexte** :
  - les deux IR surveillaient les angles avant (évitement) ;
  - la grille du labo note « atteint une zone précisément » ; l'odométrie seule accumule des erreurs ;
  - l'équipe veut aussi pouvoir faire rouler le PAMI avec l'ultrason seul, sans les IR.
- **Options envisagées** :
  - garder les IR pour l'évitement et trouver la zone par l'odométrie seule : moins précis ;
  - IR tournés vers le sol pour voir le bord de la zone, évitement par l'ultrason seul : précis, mais
    les angles avant ne sont plus surveillés ;
  - acheter d'autres capteurs : plus tard si besoin.
- **Décision** :
  - les deux IR sont montés à l'avant, tournés vers le sol (GPIO 34 et 35, câblage inchangé) ;
  - nouvelle étape de mission `REACH_ZONE` : au début de l'approche, on note ce que voient les capteurs ;
    dès que l'un des deux voit autre chose, le PAMI est au bord. Il freine, entre de `ZONE_ENTRY_MM`
    (50 mm), et s'arrête. Recherche limitée à `ZONE_SEARCH_MAX_MM` ;
  - l'évitement ne dépend plus que de l'ultrason. Il reste toujours actif (règle d'or n° 1) ;
  - le côté du détour devient fixe : gauche pour le bleu, droite pour le jaune.
- **Pourquoi** :
  - le repère est le vrai bord de la zone : à 80 mm/s, l'arrêt se fait en moins d'un centimètre ;
  - comparer au sol de départ marche que la zone soit claire ou foncée ;
  - les capteurs de sol ne sont lus que pendant l'approche finale : les autres lignes croisées en chemin
    ne déclenchent rien.
- **Règles en jeu** :
  - grille du labo et règlement général : « couverture suffisante autour du robot pour assurer la
    détection dans tous les sens de déplacement possibles ». Avec un seul HC-SR04 (cône d'environ 30°),
    les angles avant ne sont pas couverts. **Risque à l'homologation** ;
  - règles 2027, E.4 : un PAMI compte s'il a une partie de lui dans une douve adverse. S'arrêter sur le
    bord suffirait (`ZONE_ENTRY_MM = 0`).
- **Conséquences** :
  - plus besoin des IR pour rouler : moteurs, driver, encodeurs et ultrason suffisent ;
  - le potentiomètre des IR se règle pour que la LED change d'état au passage sol → zone ;
  - si l'encadrant juge la couverture insuffisante : ajouter un 2ᵉ ultrason, ou deux ultrasons orientés
    un peu vers l'extérieur.

### D010 — Wi-Fi en point d'accès pour téléverser et lire les messages sans câble
- **Date** : 12/11/2026
- **Statut** : adoptée
- **Contexte** : le PAMI roule sur batterie. Brancher l'USB pour chaque téléversement ou pour lire
  le moniteur devient pénible, surtout pendant les essais au sol.
- **Options envisagées** :
  - Bluetooth classique : le PAMI apparaît comme un port COM, mais PlatformIO ne sait pas téléverser
    par Bluetooth. L'USB resterait nécessaire pour chaque téléversement ;
  - Wi-Fi, PAMI et PC sur le partage de connexion d'un téléphone : le PC garde internet, mais il faut
    ce téléphone à chaque séance, et l'adresse du PAMI peut changer ;
  - Wi-Fi, le PAMI crée son propre réseau (point d'accès) : marche partout, adresse fixe 192.168.4.1.
- **Décision** :
  - point d'accès « PAMI-1 », WPA2 ; nom et mot de passe dans `wifi_secrets.h`, ignoré par git ;
  - téléversement sans fil par ArduinoOTA (`upload_protocol = espota`, environnements `*_wifi`) ;
  - moniteur par telnet, port 23 (`pio device monitor -e esp32dev_wifi`) ;
  - `Console` remplace `Serial` partout : chaque message part sur l'USB et sur le Wi-Fi ;
  - tous les programmes (firmware et bancs) gardent le Wi-Fi, sinon téléverser un banc sans Wi-Fi
    couperait la liaison.
- **Pourquoi** :
  - seul le Wi-Fi permet de téléverser sans câble avec PlatformIO ;
  - point d'accès : aucune box ni téléphone nécessaire, au labo comme à la coupe.
- **Sécurité et règles d'or** :
  - envoi des messages en `MSG_DONTWAIT` : `WiFiClient::write()` peut bloquer jusqu'à 10 s quand le
    réseau est lent (règle d'or n° 4). Si le PC ne suit pas, les messages attendent dans un tampon de
    32 Ko, puis sont perdus (et comptés), mais `loop()` n'attend jamais ;
  - téléversement accepté seulement PAMI immobile : avant le départ ou après la fin (firmware), hors
    essai (bancs). Au début d'un téléversement, `loop()` est figée : les freins sont serrés par
    `motorDriver::brake()`. C'est la seule exception au chemin « safety → driver » ;
  - le firmware ne lit aucune touche : on ne peut pas piloter le PAMI à distance.
    **Modifié par D013** : il lit une seule commande, le départ (`go`), et rien d'autre.
- **Règles en jeu** :
  - règlement général F.6 : « aucune communication avec l'extérieur de la table pendant le match » ;
  - règlement général F.5 : Wi-Fi 5 GHz recommandé plutôt que 2,4 GHz. L'ESP32 ne fait que du
    2,4 GHz ; c'est une recommandation, et notre radio sera coupée pendant les matchs Eurobot.
- **Profil labo** : `WIFI_ON_DURING_MATCH = true`, pour lire les messages en direct pendant les essais.
  **Eurobot : `false`** (dans la liste « À REMPLACER AVANT TOUT MATCH EUROBOT » de `pami/CLAUDE.md`).
  La radio s'éteint alors au départ et se rallume à la fin ; les messages du match attendent dans le
  tampon et s'affichent à la reconnexion.
- **Question pour la FAQ** (www.eurobot.org/faq) : « Le point d'accès Wi-Fi d'un PAMI peut-il rester
  actif entre sa mise en place et le retrait du cordon, s'il est coupé pendant tout le match ? »
- **Conséquences** :
  - le tout premier téléversement d'un programme avec Wi-Fi se fait par USB ;
  - pare-feu Windows : autoriser le port 55910 (une fois) ;
  - la radio consomme environ 120 mA de plus sur la batterie ;
  - si le Wi-Fi ne répond plus, l'USB et les environnements habituels marchent toujours.

### D011 — Régulateur de vitesse : anticipation par roue + PI, sans terme D
- **Date** : 12/11/2026
- **Statut** : adoptée
- **Contexte** : la branche `PID` apporte un nouveau régulateur. L'équipe se demandait s'il fallait
  le garder, ou l'ancien « PI » de `pami-firmware-v1`, et comment régler les gains.
- **Constat** : les deux sont des PI avec anticipation ; aucun n'a de terme D. Le nouveau ajoute une
  anticipation propre à chaque roue et un anti-emballement de l'intégrale. Avec les réglages actuels,
  les deux se comportent exactement pareil : l'anticipation par roue vaut encore la valeur commune, et
  l'anti-emballement ne s'active qu'au-dessus de 240 mm/s.
- **Décision** :
  - garder le régulateur de la branche `PID` (fusionné dans `main` le 12/11/2026) ;
  - pas de terme D ;
  - régler dans l'ordre : anticipation de chaque roue (escalier de `test_pid`), puis KP / KI seulement si
    l'échelon échoue, puis l'étalonnage avec `test_trajectoire`.
- **Pourquoi** :
  - l'anticipation fait l'essentiel du travail ; mesurée par roue, elle rend les deux roues identiques ;
  - le terme D amplifierait le bruit de la mesure de vitesse (1 tick en 20 ms = 5 mm/s) ;
  - la plupart des erreurs de trajectoire viennent de l'étalonnage, pas des gains.
- **Conséquences** : méthode complète dans `docs/reglage-regulation.md` ; nouveau banc
  `test_trajectoire` ; correction du diamètre gauche/droite seulement si les mesures la demandent.

### D012 — PCB, deux ultrasons combinés par un OU, capteurs IR supprimés
- **Date** : 18/11/2026
- **Statut** : adoptée. Brochage et émission « chacun son tour » remplacés par D015 (nouveau PCB,
  TRIG commun) ; le OU entre les deux ultrasons et la suppression des IR restent.
- **Contexte** :
  - l'équipe a remplacé le câblage sur plaque d'essai par un PCB. Ses broches ont été testées avec le
    programme `Controller/` ;
  - le PCB a 3 connecteurs d'ultrason (gauche, droit, « du bas ») ;
  - les capteurs IR de sol (D009) devaient trouver le bord de la zone, mais le sol a beaucoup de lignes :
    le PAMI risquait de s'arrêter sur la mauvaise.
- **Options envisagées** :
  - 3 ultrasons : le troisième n'a pas d'usage défini pour l'instant ;
  - 2 ultrasons, avant gauche et avant droit, combinés par un OU : couvre les angles avant (le risque
    noté dans D009) ;
  - 2 ultrasons, mais une moyenne des deux : un capteur qui rate l'obstacle ferait croire qu'il est
    plus loin. Refusé.
- **Décision** :
  - brochage du PCB recopié dans `docs/materiel.md` et `config.h` ;
  - 2 ultrasons, gauche et droit. Le connecteur du bas n'est pas utilisé ;
  - **OU** : le PAMI freine si l'ultrason gauche OU l'ultrason droit voit un obstacle plus proche
    que `OBSTACLE_STOP_DISTANCE_MM` ;
  - les deux capteurs émettent chacun leur tour, 60 ms entre deux émissions (`US_PERIOD_MS`) ;
  - un capteur sans aucune mesure pendant 1 s = panne → FAULT, comme avant (sécurité par défaut) ;
  - capteurs IR et étape `REACH_ZONE` supprimés.
- **Pourquoi** :
  - avec un OU, si un capteur rate l'obstacle (écho perdu, obstacle hors de son cône), l'autre le voit
    encore. Un seul capteur fiable suffit pour freiner ;
  - émettre chacun son tour évite qu'un capteur entende le son de l'autre et voie un faux obstacle.
    Le Controller fait déjà comme ça, et ses essais ont réussi ;
  - un capteur « muet » (il ne répond plus du tout) n'est pas la même chose qu'un capteur qui rate un
    écho : le premier est détecté comme panne, le second est couvert par le OU.
- **Conséquences** :
  - chaque capteur mesure environ 8 fois par seconde au lieu de 16 : le PAMI réagit un peu plus tard
    (environ 0,12 s de plus, soit 2 cm à 150 mm/s). Le seuil de 150 mm reste volontairement large ;
  - les sens des moteurs et des encodeurs viennent du Controller, qui décode autrement : à revérifier
    avec les bancs `test_moteurs` et `test_odometrie` ;
  - `docs/pdf/cablage complet pami.pdf` et `docs/cablage-pami.html` sont obsolètes ;
  - le banc `test_ultrason` affiche les deux capteurs et la décision de `safety`.

### D013 — Départ par une commande tapée dans le moniteur (profil labo)
- **Date** : 18/11/2026
- **Statut** : adoptée — **profil labo seulement, interdit à Eurobot**
- **Contexte** :
  - le PCB n'a plus de broche pour le bouton de départ (GPIO 16 est passé au moteur droit) ;
  - l'équipe ne veut pas de tirette : le PC est déjà connecté au Wi-Fi du PAMI (D010) pour lire les
    messages.
- **Décision** :
  - le départ se donne en tapant `go` puis Entrée dans le moniteur, Wi-Fi ou USB
    (`START_COMMAND`, `config.h`) ;
  - majuscules et espaces ne comptent pas ; un seul départ par allumage ; tout autre mot est refusé ;
  - le minuteur (5 s / 14,5 s, D007) compte depuis cette commande ;
  - `logic/start_button.*` est supprimé (il reste dans l'historique git, testé en T001).
- **Pourquoi** :
  - aucun matériel à ajouter ;
  - un mot suivi d'Entrée, plutôt qu'une touche : une touche frappée par erreur ne lance pas le PAMI ;
  - le PAMI ne part toujours jamais tout seul à l'allumage.
- **Règles en jeu** : règlement général F.6.
  - « Aucun autre démarrage manuel n'est homologué : pas de télécommande, pas d'interrupteur » ;
  - « aucune communication avec l'extérieur de la table pendant le match » ;
  - pour un PAMI, seuls un cordon de 50 cm ou un départ donné par le robot principal sont prévus.
  Ce départ est donc **interdit à Eurobot**. À confirmer avec l'encadrant pour l'évaluation du labo.
- **Conséquences** :
  - D010 change sur un point : le firmware lit désormais UNE commande, le départ. Il ne lit rien d'autre :
    on ne peut toujours pas piloter le PAMI à distance, ni toucher à l'évitement ;
  - avant tout match Eurobot : départ par cordon (une broche libre du PCB à câbler) ou par le robot
    principal (ESP-NOW par exemple). Ajouté à la liste « À REMPLACER » de `pami/CLAUDE.md`.

### D014 — Côté de départ gauche/droite, douve la plus proche par l'odométrie, détour vers le centre
- **Date** : 18/11/2026
- **Statut** : adoptée, en partie remplacée par D017 (douve assignée au lieu de la plus proche, vraies
  positions des douves, nouveau sens de l'interrupteur) et par D020 (plus d'interrupteur : le côté est
  choisi avec la commande de départ). Le miroir et le détour vers le centre restent valables.
- **Contexte** :
  - le PAMI peut partir du côté gauche ou du côté droit de la table (vu du public, règles 2027 D.2) ;
  - son but est d'atteindre une douve du château adverse (règles 2027, E.4) ;
  - sans capteurs de sol (D012), il doit trouver la douve par l'odométrie seule.
- **Décision** :
  - un interrupteur 2 positions sur GPIO 22 (GPIO 27 depuis D015) donne le côté : ouvert = GAUCHE, fermé = DROITE. Lu avant le
    départ, verrouillé au départ. Le module `side` remplace le module `color` ;
  - les trajets sont écrits une fois, pour un départ à gauche, et mis en miroir pour la droite
    (y → −y, caps changés de signe) par `forSide()` (règle d'or n° 5) ;
  - départ en (0, 0), cap 0 ; les 3 douves sont écrites en dur dans `config.h` (`MOATS_MM`), dans le
    repère d'un départ à gauche. **Provisoires** tant que le plan de la table n'est pas relevé ;
  - mission `MOAT` : au départ, le PAMI choisit la douve la plus proche (`logic/nearest_moat.h`), y va
    (étape GOTO, arrivée à moins de 30 mm), puis y reste, bras prêt (étape et état DEFEND) ;
  - le détour se fait toujours vers le centre de la table : par la droite pour un départ à gauche, par
    la gauche pour un départ à droite.
- **Pourquoi** :
  - un interrupteur se voit d'un coup d'œil, sans console ;
  - le miroir évite d'écrire et de tester deux fois chaque trajet ;
  - le miroir ne change aucune distance depuis (0, 0) : la douve choisie est la même des deux côtés ;
  - contourner par le centre évite de se coincer contre une bordure.
- **Limites** :
  - le côté du détour est fixé dans le repère du PAMI : c'est juste tant que le PAMI ne s'est pas
    retourné de plus de 90° par rapport à son cap de départ. À vérifier avec les vraies douves ;
  - la précision à l'arrivée dépend entièrement de l'étalonnage (diamètre des roues, entraxe) et du
    glissement : bancs `test_trajectoire` et `test_odometrie`. Viser un point bien à l'intérieur de la douve.
- **Conséquences** :
  - `ACTIVE_MISSION` reste sur `STRAIGHT_LINE` tant que les positions des douves sont fausses ;
  - remplace D009 pour la façon de trouver la zone.

### D015 — PCB « Test PCB 2026 5EO » : nouveau brochage, TRIG commun aux deux ultrasons
- **Date** : 21/11/2026
- **Statut** : remplacée par D018 (brochage du 25/11/2026, un TRIG par ultrason)
- **Contexte** :
  - l'équipe a un nouveau PCB, « Test PCB 2026 5EO ». Son pinout et le programme de test de l'équipe
    donnent de nouvelles broches pour les moteurs, les encodeurs et les ultrasons ;
  - sur ce PCB, les deux ultrasons partagent une seule broche TRIG (GPIO 21) ;
  - GPIO 22 et 23 servent à un accéléromètre I2C, et le pinout ne prévoit pas d'interrupteur de côté.
- **Décision** :
  - brochage recopié dans `docs/materiel.md` puis `config.h` : moteurs 32/33 (gauche) et 19/18
    (droit), encodeurs 25/26 (gauche) et 5/17 (droit), TRIG commun 21, ECHO 2 (gauche) et 15 (droit),
    servo 13, accéléromètre SDA 22 / SCL 23 (noté, pas encore utilisé) ;
  - interrupteur de côté sur **GPIO 27**, à câbler entre GPIO 27 et 3,3 V (pull-down interne) ;
  - les deux ultrasons émettent ensemble : une impulsion sur GPIO 21 toutes les 60 ms au moins, quand
    les deux ECHO sont au repos. Si un ECHO reste haut plus de 120 ms (`US_ECHO_STUCK_MS`), on
    déclenche quand même ;
  - le OU entre les deux capteurs (D012) ne change pas.
- **Pourquoi** :
  - avec un TRIG commun, émettre « chacun son tour » (D012) n'est plus possible ;
  - attendre que les deux ECHO soient au repos évite de couper une mesure en cours ;
  - déclencher quand même si un ECHO est bloqué : l'autre capteur continue de mesurer, et seul le
    capteur bloqué est déclaré en panne. Le diagnostic reste clair (le PAMI s'arrête de toute façon :
    panne = FAULT) ;
  - GPIO 27 : libre sur le pinout, pas une broche de démarrage, avec un pull-down interne.
- **Conséquences** :
  - chaque capteur mesure de nouveau environ 16 fois par seconde : le PAMI réagit plus vite qu'avec D012 ;
  - risque : un capteur peut entendre le son de l'autre. Renvoyé par un vrai obstacle, c'est sans
    conséquence grâce au OU ; passé directement d'un capteur à l'autre, ce serait un faux obstacle
    tout proche, et le PAMI s'arrêterait sans raison (côté sûr). **À vérifier** avec le banc
    `test_ultrason` (protocole 6 : rien devant pendant 10 s, aucune mesure sous 300 mm). Si ça arrive :
    écarter les capteurs, les tourner un peu vers l'extérieur, ou mettre une cloison entre eux ;
  - GPIO 2, 5 et 15 sont des broches de démarrage. Au repos, un ECHO est bas et l'encodeur a un
    pull-up : ça doit passer, mais si la carte refuse de démarrer, débrancher ultrasons et encodeur droit ;
  - le sens des moteurs et des encodeurs n'est pas connu pour ce PCB : à revérifier avec les bancs
    `test_moteurs` et `test_odometrie` (`MOTOR_*_INVERTED`, `ENCODER_*_INVERTED`) ;
  - le programme de test `Controller/` n'est plus dans le dépôt : seuls le pinout et cette entrée font foi.

### D016 — Évitement prudent : attendre, contourner par le centre, dépasser, puis reprendre la mission
- **Date** : 24/11/2026
- **Statut** : adoptée
- **Contexte** :
  - avec le banc `test_parcours`, face à une boîte, le PAMI faisait son détour (quart de tour vers le
    centre, 200 mm de côté) **puis s'arrêtait** au lieu de reprendre sa mission ;
  - en cause : juste après le pas de côté, il visait aussitôt son point en diagonale, vers le coin de
    l'obstacle qu'il n'avait pas dépassé. Il y avait donc un nouvel arrêt, de nouveaux détours tournant
    tous dans le même sens, puis une attente sur place (3 détours au plus) ;
  - le match utilisait exactement la même stratégie ;
  - l'équipe veut une stratégie **prudente**, qui passe par le centre puis reprend la mission.
- **Options envisagées** :
  - attendre plus longtemps avant le détour : utile face à un obstacle qui bouge (PAMI adverse, robot
    qui passe), inutile face à un obstacle fixe (pierre de 320 × 110 mm, robot arrêté). Et chaque seconde
    coûte : 14,2 s de mouvement pour un PAMI à Eurobot, 9,5 s au labo ;
  - viser un point de passage en diagonale, à côté de l'obstacle : rapide, mais la diagonale frôle le
    coin de l'obstacle ;
  - contourner en mouvements séparés, en regardant avant de repartir : plus long, mais le PAMI ne
    repart vers son point qu'après avoir vérifié qu'il a dépassé l'obstacle.
- **Décision** :
  1. arrêt à 150 mm (inchangé) ;
  2. attente de **1,5 s** (au lieu de 1 s) : un obstacle qui s'en va ne déclenche pas de contournement ;
  3. contournement par le centre (`logic/detour.h`) : quart de tour vers le centre, 200 mm de côté,
     quart de tour retour (parallèle à la route), pause de 0,3 s, puis regard. Voie libre sur plus de
     300 mm : le PAMI avance de 300 mm pour dépasser l'obstacle, puis reprend sa mission (il vise de
     nouveau son point). Sinon : un pas de côté de plus, **2 au plus** (400 mm de décalage) ;
  4. obstacle pendant une ligne droite du contournement : arrêt, retour à l'étape 2 ;
  5. **2 contournements par match au plus** ; ensuite, le PAMI reste arrêté et repart dès que la voie se
     libère. Il ne force jamais le passage.
- **Pourquoi** :
  - prudence : le PAMI n'avance jamais sans avoir regardé dans la direction où il va. Chaque ligne
    droite reste surveillée par l'évitement (règle d'or n° 1) ; les rotations sur place restent permises ;
  - le retour parallèle et le regard corrigent la cause du défaut : on ne recoupe plus vers l'obstacle ;
  - la même classe `Detour` sert au match (`state_machine.cpp`) et au banc `test_parcours` : ce que l'on
    voit au banc est ce que fera le PAMI en match. Elle est en logique pure, testée sur PC
    (`test/test_detour`).
- **Conséquences** :
  - un contournement dure environ 7 s aux vitesses actuelles : un seul tient dans la phase PAMI ;
  - `AVOID_WAIT_MS = 1500`, `AVOID_MAX_DETOURS = 2`, nouveaux `DETOUR_LOOK_MS = 300`,
    `DETOUR_PASS_MM = 300`, `DETOUR_MAX_SIDESTEPS = 2` (provisoires, à régler d'après les essais) ;
  - limite connue : le côté du contournement est fixé dans le repère du PAMI (D014). Il reste juste tant
    que le PAMI ne s'est pas retourné de plus de 90° par rapport à son cap de départ.

### D017 — Chaque PAMI va à la douve de son côté, positions tirées du plan de la table
- **Date** : 24/11/2026
- **Statut** : adoptée. Le sens GAUCHE / DROITE reste valable ; le côté vient maintenant de la
  commande de départ (D020), et la vitesse a été relevée (D019)
- **Contexte** :
  - la mission `MOAT` choisissait au départ la douve la plus proche (D014), parmi 3 positions de test ;
  - avec plusieurs PAMI, cette règle peut envoyer deux PAMI vers la même douve. Ils se gêneraient
    (ultrasons, évitement), et une douve ne compte qu'une fois (5 points) ;
  - l'équipe a relevé les positions sur le plan coté G.1 du règlement 1.0 (`docs/douves-coordonnees.md`,
    ± 5 mm). Dans le repère du PAMI (x = avance depuis notre petit bord, y = distance au grand bord côté
    écurie), les valeurs sont **les mêmes pour les 4 écuries** ;
  - le même document montre que notre définition de l'interrupteur (« côté de la table vu du public »)
    était fausse : l'écurie jaune avant, par exemple, a le bord de la table à la droite du PAMI.
- **Décision** :
  - **douve assignée** : chaque PAMI va à la douve diagonale de son côté (`MOAT_NEAR_MM` = (2179, 429)).
    Le PAMI DROITE va à la douve de droite, le PAMI GAUCHE à celle de gauche. Plus de calcul de la plus
    proche : `logic/nearest_moat.h` est supprimé ;
  - trajet conseillé par le document : tout droit à y = 210 mm jusqu'au point de virage (2000, 210),
    virage d'environ 51° vers le centre, puis environ 283 mm jusqu'au centre de la douve. Mission
    `MOAT_STEPS` (`logic/missions.h`) : 2 étapes GOTO, puis DEFEND ;
  - les points sont écrits dans le repère de la table (`TablePoint`, `config.h`). `gotoTablePoint()` les
    convertit dans le repère de l'odométrie pour GAUCHE, puis `forSide()` fait le miroir pour DROITE ;
  - départ **provisoire** : milieu de l'essieu en (100, 210) dans l'écurie, cap vers l'adversaire ;
  - sens de l'interrupteur : GAUCHE = bord de la table à la gauche du PAMI, regard vers l'adversaire,
    donc le centre est à sa droite. DROITE = l'inverse. Les valeurs du code (`StartSide`,
    `chooseDetourSide`) ne changent pas, seuls les commentaires et les messages `[COTE]` changent ;
  - la douve centrale et la douve éloignée restent dans `config.h` (`MOAT_CENTER_MM`, `MOAT_FAR_MM`),
    non assignées, pour plus tard (robot principal) ;
  - le banc `test_parcours` reçoit la commande `d`, qui joue exactement `MOAT_STEPS`.
- **Pourquoi** :
  - avec deux écuries par équipe et des interrupteurs opposés, les deux PAMI visent toujours deux
    douves différentes : pas de gêne mutuelle, et deux douves comptées ;
  - le couloir à y = 210 passe entre les carrières de bord (y ≤ 110) et nos remparts (y ≥ 305), et
    évite la carrière centrale ;
  - le virage vers la douve et le détour d'évitement vont du même côté, vers le centre : le plus grand
    virage fait environ 51°, donc la limite de D014 (pas plus de 90° par rapport au cap de départ) est
    respectée.
- **Limites** :
  - valable avec **un PAMI par écurie**. Deux PAMI dans la même écurie avec le même réglage iraient à
    la même douve : il faudrait un réglage de plus (douve centrale ou éloignée) ;
  - les PAMI adverses peuvent emprunter le même couloir en sens inverse, au même moment ;
  - positions relevées sur le plan : à vérifier sur une vraie table (règlement 1.0, D.1). Notre résumé
    `docs/regles-jeu-2027.md` est encore en bêta 0.4 : il est à mettre à jour avec la version 1.0.
- **Conséquences** :
  - **vitesse** : le trajet fait environ 2,2 m. À 150 mm/s, il dure environ 16 s sans obstacle. Or le PAMI
    n'a que 14,2 s pour bouger à Eurobot (85,3 → 99,5 s) et 9,5 s au labo. À 300 mm/s, il faudrait
    environ 9,5 s. Relever la vitesse est l'étape suivante : `test_pid` à refaire, distance de freinage
    à mesurer (elle fixe le seuil d'arrêt de 150 mm), étalonnage à revérifier ;
  - `ACTIVE_MISSION` reste `STRAIGHT_LINE` : le firmware de match exige les 2 ultrasons (GPIO 2 ne
    répond pas, T012), et le trajet ne tient pas encore dans le temps. Le trajet s'essaie avec le banc
    `test_parcours`, commande `d` ;
  - `START_POINT_MM` est à corriger quand la place du PAMI dans l'écurie sera fixée.

### D018 — Brochage du 25/11/2026 : un TRIG par ultrason, presque celui du premier PCB
- **Date** : 25/11/2026
- **Statut** : adoptée
- **Contexte** :
  - l'équipe change de brochage. Un premier brochage proposé (programme « Controller ») mettait les
    TRIG des ultrasons sur GPIO 25 et 5, déjà utilisées par les voies A des encodeurs. Une broche ne
    peut pas être à la fois une entrée d'encodeur et une sortie TRIG : comptage faux, et la sortie de
    l'ESP32 forcerait contre celle de l'encodeur. L'équipe a donc donné un autre brochage ;
  - il n'y a que deux ultrasons, gauche et droit.
- **Décision** :
  - moteurs : gauche IN1 (A) 27 / IN2 (B) 26, droit IN1 (A) 16 / IN2 (B) 17 ;
  - encodeurs : gauche A 33 / B 32, droit A 19 / B 18 ;
  - ultrasons : gauche TRIG 25 / ECHO 35, droit TRIG 14 / ECHO 12, puis **TRIG 5 / ECHO 4** (correction du
    25/11 après T016 : rien ne revenait sur 14 / 12, qui est le connecteur du bas) (« Sensor 1 » et « Sensor 2 » du
    programme de l'équipe) ;
  - servo : 13 (inchangé). Plus d'interrupteur de côté (D020), plus d'I2C ;
  - les deux ultrasons ont chacun leur TRIG, mais le firmware les fait **toujours émettre ensemble**,
    comme avec le TRIG commun (D015).
- **Pourquoi** :
  - émettre ensemble garde le fonctionnement déjà écrit et réfléchi (D015) : une mesure toutes les
    60 ms pour chaque capteur. Les faire émettre chacun leur tour diviserait par deux le nombre de
    mesures de chacun (le choix du premier PCB, D012) ;
  - un capteur peut entendre l'autre : le protocole 6 de `test_ultrason` le vérifie. Si c'est le cas,
    on passera à l'émission chacun son tour ;
  - le mode de référence de `test_ultrason` déclenche maintenant un seul capteur à la fois : il permet
    de séparer un problème de capteur d'un problème d'écoute croisée.
- **Conséquences** :
  - sens des moteurs et des encodeurs à revérifier (`test_moteurs`, `test_odometrie`) : les réglages
    `*_INVERTED` sont gardés, mais rien ne garantit qu'ils sont encore justes ;
  - gauche / droite des ultrasons à vérifier : une main devant le capteur gauche seul doit faire réagir G ;
  - pont diviseur 1 kΩ / 2 kΩ obligatoire sur chaque ECHO (ECHO sort du 5 V) ;
  - le banc `test_parcours` choisit son ultrason dans `config.h` (`TEST_BENCH_PARCOURS_USE_LEFT_US`) ;
  - broches libres pour un cordon de départ (Eurobot) : de préférence 14, 21, 22 ou 23.

### D019 — Vitesse de croisière 350 mm/s pour atteindre la douve dans les 9,5 s du labo
- **Date** : 25/11/2026
- **Statut** : adoptée, PROVISOIRE (à valider avec `test_pid`, `test_trajectoire` et `test_parcours`)
- **Contexte** :
  - le trajet de la douve (D017) fait environ 2,2 m. À 150 mm/s, il dure environ 16 s, alors que le
    profil labo ne laisse que 9,5 s de mouvement (5 s → 14,5 s) ;
  - d'après `config.h`, les FIT0450 sous 6 V font environ 400 mm/s au sol, au maximum.
- **Options envisagées** (durées estimées du trajet sans obstacle) :

  | Vitesse | Accélération | Durée |
  |---|---|---|
  | 150 mm/s (avant) | 400 mm/s² | environ 16 s |
  | 300 mm/s | 400 mm/s² | environ 9,5 s : aucune marge |
  | **350 mm/s** | **500 mm/s²** | **environ 8,5 s** |
  | 400 mm/s | 500 mm/s² | environ 7,7 s, mais au maximum des moteurs : plus de marge pour la régulation |

- **Décision** :
  - `DRIVE_SPEED_MM_S` 150 → **350**, `MOTION_ACCEL_MM_S2` 400 → **500**, `TURN_SPEED_MM_S` inchangé (90) ;
  - `OBSTACLE_STOP_DISTANCE_MM` 150 → **200** : le temps que la médiane des mesures confirme
    l'obstacle (jusqu'à environ 120 ms), le PAMI parcourt environ 40 mm, puis il glisse en freinant ;
  - banc `test_pid` : jusqu'à 400 mm/s, escalier à 100, 200, 300 et 350 mm/s (avant : 50 à 200),
    1 s par palier, limite au sol 1000 mm (avant : 800).
- **Pourquoi** : 350 mm/s fait tenir le trajet dans le temps du labo avec environ 1 s de marge, sans
  aller jusqu'à la limite des moteurs.
- **Conséquences** :
  - l'anticipation n'a été mesurée que jusqu'à 200 mm/s (T004, T005) : refaire l'escalier de `test_pid` ;
  - le glissement à l'arrêt grandit avec la vitesse : refaire l'étalonnage avec `test_trajectoire` ;
  - mesurer la vraie distance d'arrêt devant un obstacle (`test_parcours`) et régler le seuil d'après elle ;
  - revers du seuil de 200 mm : un ultrason voit plus tôt ce qui est sur le côté. Les carrières le long
    du bord sont à environ 70 mm de l'axe du capteur sur le trajet de la douve : à surveiller au sol ;
  - au labo, un seul contournement (plusieurs secondes) suffit à rater la douve : c'est le temps
    disponible qui l'impose, pas le code ;
  - avec la batterie (plus de 6 V), les vitesses réelles changeront : à revérifier.

### D020 — Le côté GAUCHE / DROITE est choisi avec la commande de départ
- **Date** : 25/11/2026
- **Statut** : adoptée — **profil labo seulement** (comme D013)
- **Contexte** :
  - l'équipe n'a pas d'interrupteur de côté sur le PAMI ;
  - au labo, le départ se donne sans fil, par une commande (D013), ce que l'enseignant autorise.
- **Décision** :
  - `go g` : départ, PAMI GAUCHE (bord de table à sa gauche, centre à sa droite) ;
  - `go d` : départ, PAMI DROITE ;
  - `go` seul est **refusé**, avec un message qui rappelle les deux commandes ;
  - majuscules et espaces ne comptent pas (`GO G`, `gog`) ; un seul départ par allumage, le côté ne
    change plus ensuite ;
  - le module `side` ne lit plus de broche : il garde le côté donné par la commande. Le détecteur de
    commande (`logic/start_command.h`) reste en logique pure, testé sur PC (`test_match_timer`) ;
  - le banc `test_parcours` choisit son côté avec `c g` ou `c d`.
- **Pourquoi** :
  - choisir le côté au moment même du départ évite d'oublier de le régler ;
  - refuser `go` seul : un PAMI parti du mauvais côté tournerait vers le bord au lieu de sa douve, et
    contournerait les obstacles vers le bord. Le côté doit toujours être choisi exprès.
- **Conséquences** :
  - à Eurobot, le départ sans fil est interdit (règlement général F.6) : il faudra un cordon de départ
    ET une autre façon de donner le côté (interrupteur, cavalier, ou le robot principal) ;
  - sans commande, rien ne bouge : le PAMI attend toujours son départ.

### D021 — Attaque du bras : 120°, tenue 3 s, retour à 0°
- **Date** : 25/11/2026
- **Statut** : adoptée, à confirmer avec le règlement (voir Limites)
- **Contexte** :
  - l'équipe veut que le servo tourne de 120° pour attaquer le PAMI adverse, reste 3 s, puis revienne
    à sa position de départ ;
  - avant, le bras se déployait à 90° et restait déployé.
- **Décision** :
  - séquence en logique pure (`logic/arm_attack.h`, testée dans `test/test_arm_attack`) :
    prêt → attaque (`ARM_ATTACK_DEG` = 120°, `ARM_ATTACK_HOLD_MS` = 3000) → retour (`ARM_REST_DEG` = 0°,
    `ARM_RETURN_MS` = 500 laissés au servo) → prêt ;
  - déclenchement inchangé : dans la douve (état DEFEND), objet vu à moins de `ARM_TRIGGER_DISTANCE_MM`
    (60 mm). Une fois revenu, le bras attaque de nouveau si l'adversaire est toujours là ;
  - l'attaque commencée va au bout, même si l'adversaire s'éloigne ;
  - le calcul de l'impulsion du servo passe dans `logic/servo_pulse.h`, partagé par le match et le
    nouveau banc `test_bras` ;
  - inchangé : le bras ne bouge que pendant la période de mouvement, et le servo est coupé à la fin
    (il reste alors où il est, sans force).
- **Pourquoi** : la séquence demandée, écrite sans attente bloquante (règle d'or n° 4), et testable sur PC.
- **Limites** :
  - **règles 2027, E.4 (bêta 0.4)** : l'attaque ne compte que si l'actionneur **touche le PAMI adverse
    à la fin du match**. Un bras revenu à 0° à la fin ne rapporte rien. Avec cette séquence, l'attaque
    ne compte que si elle a lieu dans les 3 dernières secondes ;
  - question pour la FAQ (www.eurobot.org/faq) : « Pour les points d'attaque d'un PAMI adverse, l'actionneur
    doit-il être en contact avec le PAMI adverse au moment exact de la fin du match, ou suffit-il d'avoir
    touché le PAMI adverse pendant la phase d'attaque ? » ;
  - le règlement interdit d'endommager le PAMI adverse : à vérifier avec le vrai bras ;
  - périmètre bras à 120° : ≤ 700 mm (règlement général F.7), à mesurer avec `test_bras`.

### D022 — Encodeurs comptés par le compteur matériel de l'ESP32 (PCNT), avec filtre anti-parasites
- **Date** : 25/11/2026
- **Statut** : adoptée, **validée par T022** (les deux roues tournent pareil à 350 mm/s ; dérive au sol
  identique à 100 et 350 mm/s)
- **Contexte** :
  - le PAMI dérivait vers la droite : 53 mm sur 800 mm à 100 mm/s, 340 mm à 350 mm/s (T020), alors
    que l'odométrie croyait aller droit ;
  - roues en l'air, à vitesse annoncée égale, une roue demandait 14 à 24 points de PWM de plus que
    l'autre (T018, T021) ;
  - vidéo des roues sur cales à 350 mm/s (T021) : la roue droite tournait **32 % plus vite** que la
    gauche alors que les encodeurs les annonçaient égales. L'un comptait trop, l'autre pas assez ;
  - échanger les voies A et B ne faisait que déplacer le défaut d'une roue à l'autre (T021). Moteurs
    arrêtés, roues tournées à la main, le comptage était presque juste (T019) ;
  - l'ancien code comptait par interruption : à chaque front de A, il lisait B pour connaître le sens.
    Un parasite très court venu des moteurs, ou une interruption en retard (Wi-Fi), faisait compter un
    tick en trop ou à l'envers.
- **Options envisagées** :
  - choisir la « bonne » voie pour l'interruption : écarté, aucune ne compte juste (T021) ;
  - interruptions sur A et B (décodage x4 par logiciel) : toujours sensible au retard et aux parasites ;
  - **compteur matériel PCNT** : un circuit de l'ESP32 fait le décodage lui-même, sans retard, avec un
    filtre qui ignore les impulsions trop courtes.
- **Décision** :
  - `encoders.cpp` utilise les unités PCNT 0 (gauche) et 1 (droite), en décodage « x2 » identique à
    l'ancien (fronts de A, sens donné par B) : toujours 1920 ticks par tour, les étalonnages et
    `ENCODER_*_INVERTED` (T014) restent valables. Les fonctions `begin()` et `read()` ne changent pas ;
  - filtre anti-parasites `ENCODER_GLITCH_FILTER_NS = 5000` (5 µs). Un vrai front arrive au plus vite
    toutes les 278 µs à 350 mm/s ;
  - le PCNT compte sur 16 bits (`ENCODER_PCNT_LIMIT_TICKS = 30000`) : un minuteur de l'ESP32 ajoute
    toutes les 10 ms (`ENCODER_COLLECT_PERIOD_MS`) ce qu'il a compté à un total sur 32 bits, même si
    personne n'appelle `read()` ;
  - retour aux voies A / B du programme de l'équipe (l'échange de l'essai T021 est abandonné).
- **Pourquoi** : la régulation de vitesse, la ligne droite et l'odométrie reposent entièrement sur les
  ticks. Un comptage faux à grande vitesse rend tout le reste faux, et aucun réglage du régulateur ne
  peut le corriger.
- **Conséquences** :
  - côté matériel, réduire les parasites à la source (`docs/materiel.md`) : 4,7 kΩ entre chaque fil
    d'encodeur et le 3,3 V, 100 nF entre les bornes de chaque moteur, fils séparés et torsadés ;
  - à vérifier : sens à la main, vidéo sur cales à 350 mm/s (les deux roues doivent tourner pareil),
    puis dérive au sol à 100 et 350 mm/s ;
  - il restera l'écart de 1,2 % de ticks par tour entre les roues (T019), à corriger par roue ;
  - si le défaut persistait malgré le PCNT, la piste suivante serait la forme des signaux A / B
    (déphasage réel, à voir à l'oscilloscope).

### D023 — Un diamètre effectif par roue (rapport droite / gauche) pour rouler droit
- **Date** : 26/11/2026
- **Statut** : adoptée. Valeur **1,0000** mesurée sur la surface de compétition (T025), à la place de
  1,0079 (sol du labo, T022), et **confirmée par T026** (4 mm sur 1000 mm, sous l'incertitude de mesure).
  Leçon : la dérive dépend du sol, ce réglage se fait sur la surface du match.
- **Contexte** :
  - depuis le comptage par PCNT (D022), la dérive en ligne droite ne dépend plus de la vitesse : environ
    **25 mm vers la gauche sur 800 mm, à 100 comme à 350 mm/s** (T022), alors que l'odométrie croit
    aller droit ;
  - dix tours à la main : 1947,7 ticks par tour à gauche, 1938,5 à droite (T022). Les deux roues
    n'avancent donc pas exactement de la même distance par tick (comptage et pneus) ;
  - le code n'avait qu'un seul `MM_PER_TICK` pour les deux roues.
- **Décision** :
  - nouveau réglage `WHEEL_DIAMETER_RIGHT_TO_LEFT_RATIO` dans `config.h`, d'où sortent
    `LEFT_WHEEL_DIAMETER_MM`, `RIGHT_WHEEL_DIAMETER_MM`, `LEFT_MM_PER_TICK` et `RIGHT_MM_PER_TICK`.
    La moyenne des deux diamètres reste `WHEEL_DIAMETER_MM` : l'étalonnage des distances (T006) ne
    change pas. `MM_PER_TICK` est supprimé, pour que le compilateur trouve chaque usage ;
  - `motion` (vitesse de chaque roue, correction de cap) et l'odométrie utilisent le nombre de mm par
    tick de chaque roue. La correction de cap égalise alors les distances **réelles** des deux roues ;
  - valeur de départ **1,0079**, calculée avec T022 : sur une ligne de L mm, un écart e sur le côté
    vient d'un écart ε = 2 × entraxe × e / L² entre les roues, soit 2 × 101,6 × 25 / 800² = 0,79 % ;
  - `test_trajectoire` : nouvelle commande `e` (écart réel sur le côté après une ligne, avec signe) ;
    le banc calcule le rapport conseillé et ce que donnerait l'écart restant sur la ligne de la douve.
    Calcul en logique pure (`logic/wheel_ratio.h`), testé dans `test/test_wheel_ratio`.
- **Pourquoi** :
  - aucun réglage du régulateur ne peut corriger une erreur que les encodeurs ne voient pas : il faut
    dire au programme quelle distance chaque roue parcourt vraiment pour un tick ;
  - l'enjeu est la douve : 0,79 % d'écart entre les roues donnerait environ 140 mm de dérive sur les
    1900 mm de la ligne droite de la douve.
- **Conséquences** :
  - à vérifier : `l 950` à la vitesse du match, 3 à 5 fois, avec `e` ; objectif moins de 0,2 % (environ
    9 mm sur 950 mm, 36 mm sur la ligne de la douve) ;
  - le rapport dépend des pneus : à remesurer si on change de roues ou de pneus, ou s'ils s'usent ;
  - avec le nouveau comptage (D022), l'anticipation (`test_pid`, escalier au sol) et le diamètre moyen
    (`l 950` puis `m`) sont aussi à remesurer.

### D024 — Un mouvement n'est fini que quand le PAMI est vraiment arrêté
- **Date** : 26/11/2026
- **Statut** : adoptée, à valider sur le vrai PAMI (mission douve, `go d` puis `go g`)
- **Contexte** (T027) :
  - mission douve : la ligne droite de 1900 mm est réussie, mais le PAMI ne fait jamais la rotation
    d'environ 50° vers la douve. Il reste immobile jusqu'à la fin du temps, sans aucun message ;
  - à 350 mm/s, le PAMI glisse encore d'environ 10 mm après la fin annoncée d'une ligne droite, et la
    rotation démarrait aussitôt, pendant cette glissade ;
  - pendant une rotation, `motion` surveille que le PAMI reste sur place (somme des distances des
    roues). Il voyait les 10 mm de glissade et demandait aux deux roues de reculer d'environ 50 mm/s,
    plus que la vitesse de départ de la rotation (40 mm/s) ;
  - `safety` refuse une marche arrière qui n'est pas une rotation sur place (aucun capteur derrière) :
    il freinait, `motion` repartait de zéro et redemandait la même chose, sans fin ;
  - à 150 mm/s (carrés T009 à T011), la glissade était trop petite pour déclencher ce blocage.
- **Décision** :
  - nouvelle phase `SETTLE` dans `motion.cpp`. Quand une ligne droite ou une rotation atteint sa
    cible, la commande passe à {0, 0} (frein). `isDone()` ne devient vrai que lorsque les deux roues
    restent sous `MOTION_STOPPED_SPEED_MM_S` (10 mm/s) pendant `MOTION_SETTLE_TIME_MS` (100 ms) ;
  - aucun délai bloquant : la vitesse des roues est regardée à chaque cycle de 20 ms (règle d'or n° 4) ;
  - pas de durée maximale : si on pousse le PAMI, il attend. Le minuteur coupe tout à la fin du temps,
    quel que soit l'état (règle d'or n° 3) ;
  - `stop()` ne change pas : il arrête tout de suite (obstacle, fin du temps, panne).
- **Pourquoi** :
  - le mouvement suivant démarre toujours à l'arrêt : une rotation n'a plus de glissade à corriger ;
  - la mission, le contournement et les bancs attendent tous `isDone()` : ils sont corrigés en même temps ;
  - bonus : la mission calcule l'angle vers la douve depuis la position finale, après la glissade.
- **Conséquences** :
  - environ 0,2 s de plus par mouvement, soit environ 0,6 s sur le trajet de la douve (fin estimée vers
    13 s au labo, avant l'arrêt à 14,5 s) ;
  - `safety` et l'évitement ne changent pas : pendant `SETTLE`, la commande est « frein ».

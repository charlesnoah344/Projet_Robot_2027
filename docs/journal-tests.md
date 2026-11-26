# Journal des tests sur le matériel

> Un test compte seulement s'il a été fait sur le vrai matériel et consigné ici.
> Les valeurs mesurées sont reportées dans `pami/src/config.h`, avec le numéro du test en commentaire.

## Modèle
### Txxx — Composant ou fonction testée
- **Date** :
- **Testeurs** :
- **Programme utilisé** : `pami/test_bench/...` (environnement PlatformIO `test_...`)
- **Montage** : alimentation, câblage, conditions (PAMI sur cales, sur table, éclairage…)
- **Protocole** : étapes suivies
- **Critère de réussite** : ce qui devait être observé, avec un chiffre si possible
- **Résultat** : réussi / échoué / partiel
- **Mesures** : valeurs relevées
- **Conclusion et suite** : ce qu'on en retient, valeurs à reporter dans config.h, prochain test

---

### T001 — Squelette du firmware : tests de logique, départ par BOOT, arrêt à 99,5 s (étape 1)
- **Date** : 28/09/2026
- **Testeurs** : à compléter
- **Programme utilisé** :
  - tests de logique `test/test_match_timer` (environnement `logic_esp32`) ;
  - firmware (environnement `esp32dev`, commit `a0652c7`).
- **Montage** : ESP32 seul (puce ESP32-D0WD-V3, révision 3.1), branché en USB sur COM5. Rien d'autre de branché.
- **Protocole** : checklist de l'étape 1 (`docs/guide-pratique-pami.md`, § 5).
- **Critère de réussite** :
  - 7 tests de logique sur 7 ;
  - au moins 1000 tours de boucle par seconde ;
  - aucun départ sans appui pendant 30 s ;
  - 2ᵉ appui sur BOOT ignoré ;
  - fin entre 99,2 et 99,8 s au chronomètre, 3 essais sur 3.
- **Résultat** : réussi (validation de l'équipe).
- **Mesures** :
  - tests de logique : 7 sur 7 réussis, exécutés sur l'ESP32 ;
  - boucle : environ 402 000 tours/s en IDLE et FORWARD, 413 000 tours/s en END (critère : au moins 1000) ;
  - essai 1 : départ environ 2 s après le démarrage ; `FIN DU MATCH (99,5 s)` entre les lignes
    t = 99,1 s et t = 100,1 s ; état END maintenu jusqu'à t = 134 s ;
  - essai 2 : après un redémarrage par EN, état IDLE pendant environ 45 s sans aucun départ ;
    `DEPART !` puis `IDLE -> FORWARD` à l'appui sur BOOT ; `FIN DU MATCH` entre les lignes
    t = 98,9 s et t = 99,9 s ; état END maintenu jusqu'à t = 180 s ;
  - 2ᵉ appui sur BOOT pendant le match : ignoré (aucune nouvelle ligne `DEPART`), selon l'équipe ;
  - temps au chronomètre : conformes selon l'équipe, mais les valeurs n'ont pas été notées ;
  - lignes du moniteur corrompues (64 caractères illisibles ou nuls à la place du « [ ») :
    2 dans l'essai 1, 6 dans l'essai 2.
- **Incident de téléversement** : `Wrong boot mode detected (0x13)`. La carte ne passe pas toute seule
  en mode téléversement. Il a réussi en maintenant BOOT pendant `Connecting`, ce que l'équipe a
  confirmé. C'est noté dans `docs/materiel.md` et dans le guide pratique.
- **Conclusion et suite** :
  - étape 1 validée : départ uniquement par BOOT, un seul départ, arrêt à 99,5 s, boucle très rapide ;
  - broche GPIO 0 (départ provisoire) validée dans `docs/materiel.md` ;
  - le chronométrage précis, avec valeurs notées, sera refait au test de la tirette et de l'attente
    de 85,3 s (levée de D005), obligatoire avant tout match ;
  - lignes corrompues : hypothèse, un problème de la liaison USB avec le PC (64 octets, c'est la
    taille d'un paquet USB). Le programme n'est pas touché : le texte reprend normalement juste après.
    Sans effet sur le PAMI, qui n'a pas d'USB en match. À surveiller quand les moteurs tourneront :
    si ça devient plus fréquent, ce sera un signe de parasites électriques ;
  - suite : étape 2, moteurs et driver (banc `test_moteurs`).

### T002 — Moteurs et driver MDD3A (étape 2)
- **Date** : 28/09/2026
- **Testeurs** : à compléter
- **Programme utilisé** : `pami/test_bench/moteurs/main.cpp` (environnement `test_moteurs`, commit `f8477e6`)
- **Montage** :
  - ESP32 en USB sur COM5 ;
  - MDD3A câblé selon `docs/guide-pratique-pami.md` (§ 3) : entrées M1A/M1B sur GPIO 25/26,
    M2A/M2B sur GPIO 33/32, GND commun ;
  - 2 moteurs FIT0450, encodeurs non branchés ;
  - alimentation de labo. À confirmer : réglages (6 V, 2 A prévus), BAU câblé, PAMI sur cales.
- **Protocole** : en-tête du banc de test (points 1 à 8).
- **Critère de réussite** :
  - `g`, `d`, `a`, `r` : la bonne roue, dans le bon sens ;
  - aucun mouvement pendant 3 redémarrages ;
  - zone morte mesurée (sur cales et au sol) ;
  - arrêt net ;
  - 0 redémarrage sur 10 départs à 100 % ;
  - BAU : arrêt immédiat, l'ESP32 ne redémarre pas.
- **Résultat** : partiel. Moteurs fonctionnels selon l'équipe ; plusieurs points du protocole ne sont
  ni visibles dans le log, ni rapportés.
- **Mesures** (log du moniteur et retour de l'équipe) :
  - démarrage du banc correct, cause du redémarrage : « mise sous tension ou bouton EN »
    (pas de chute de tension) ;
  - `a` : les deux roues commandées en avant (+20 %) ;
  - `g` : roue gauche seule (+20 %) ; `d` : roue droite seule (+20 %).
    Bonne roue et bon sens selon l'équipe (« tout fonctionne bien niveau moteur ») ;
  - `+` / `-` : vitesse changée par pas de 5 % entre 15 et 35 %, appliquée tout de suite ;
  - arrêt automatique après 10 s sans commande : observé 3 fois ;
  - aucun redémarrage de l'ESP32 pendant la séance (vitesse maximale utilisée : 35 %) ;
  - aucune ligne corrompue dans le moniteur pendant que les moteurs tournaient.
- **Pas encore vérifié** :
  - `r` (marche arrière) : aucune commande négative dans le log ;
  - broches flottantes : absence de mouvement pendant 3 redémarrages ;
  - zone morte, gauche et droite sur cales et au sol : aucune valeur relevée ;
  - arrêt net depuis 100 % ;
  - 10 départs à 100 % sans redémarrage de l'ESP32 ;
  - BAU.
- **Conclusion et suite** :
  - le câblage du MDD3A, les broches GPIO 25, 26, 33, 32 et la commande des roues fonctionnent ;
  - sens des roues correct selon l'équipe : `MOTOR_LEFT_INVERTED` et `MOTOR_RIGHT_INVERTED`
    restent à `false` ;
  - à compléter avant l'étape 3 : les points ci-dessus. La zone morte servira au régulateur de
    vitesse (étape 4) ; le test à 100 % et le BAU concernent la sécurité.

### T003 — Étalonnage : lignes droites et rotations courtes (banc test_trajectoire, PCB 2026 5EO)
- **Date** : 21/11/2026 (mesures transmises ce jour)
- **Testeurs** : l'équipe
- **Programme utilisé** : `pami/test_bench/trajectoire` (environnement `test_trajectoire`), sans
  ultrason. Vitesse des lignes : celle par défaut du banc a priori (150 mm/s), à confirmer.
- **Montage** : PCB 2026 5EO (D015), PAMI au sol. Réglages : `WHEEL_DIAMETER_MM = 60`,
  `WHEEL_BASE_MM = 100`, 1920 ticks par tour.
- **Protocole** : lignes `l 150`, `l 200`, `l 250` (2 essais chacune) ; rotations `r 90`, `r -90`
  (1 essai), `r 360` (3 essais). Distance réelle à la règle, angle réel au rapporteur.
- **Critère de réussite** (`docs/reglage-regulation.md`, § 4) : distance à 1 % près, angle à 5° près
  par tour, dispersion des essais < 1 %.
- **Résultat** : partiel. Les rotations sont très régulières ; les lignes trop courtes et trop
  dispersées pour un étalonnage précis.
- **Mesures** (commandé → réel) :

  | Commande | Réel | Écart moyen | Rapport réel / commandé |
  |---|---|---|---|
  | `l 150` | 147, 146 mm | −3,5 mm | 0,977 (−2,3 %) |
  | `l 200` | 193, 193 mm | −7 mm | 0,965 (−3,5 %) |
  | `l 250` | 250, 245 mm | −2,5 mm | 0,990 (−1,0 %) |
  | Total des 6 lignes | 1174 mm pour 1200 | | **0,978 (−2,2 %)** |
  | `r 90` / `r -90` | 90° / −90° | 0° | précision du rapporteur (±1°) trop faible pour voir 1 % |
  | `r 360` | 355, 357, 356° | −4° | **0,989 (−1,1 %)**, dispersion ±1° |

- **Analyse** :
  - lignes : le PAMI parcourt environ 2,2 % de moins que demandé. Si toute l'erreur vient du diamètre,
    le diamètre « effectif » (pneu écrasé, plus l'imprécision des 1920 ticks) vaut
    60 × 0,978 = **58,7 mm** ;
  - mais la dispersion des lignes est d'environ ±1,3 % (critère : 1 %), et l'erreur ne grandit pas
    avec la distance (200 mm est le pire cas). Sur 150 à 250 mm, une erreur fixe de quelques mm
    (tolérance d'arrêt de 3 mm, glissement de fin, repère) pèse autant que l'erreur de diamètre
    cherchée ;
  - biais du banc : « ce que le PAMI croit » est relevé quand le mouvement est déclaré fini, avant
    l'arrêt complet. Le PAMI glisse ensuite de quelques mm (5 mm vus avec `test_odometrie` :
    498 → 503 mm), que la mesure à la règle inclut. Si ce glissement vaut environ 5 mm ici aussi,
    le diamètre effectif serait plutôt de **57,8 mm**. Fourchette actuelle : 57,8 à 58,7 mm ;
  - rotations : très répétables (±1°). Le PAMI tourne 1,1 % de moins que demandé. Une fois le
    diamètre corrigé de −2,2 %, il tournera environ 1,1 % de plus que demandé (vers 364° pour 360°).
    L'entraxe effectif attendu est donc d'environ 100 × 0,978 / 0,989 ≈ **98,9 mm**, à mesurer après
    le diamètre.
- **Conclusion et suite** :
  - aucune valeur reportée dans `config.h` pour l'instant ;
  - refaire le diamètre avec `l 1000` (5 essais, 1,2 m dégagés) et `l 300` (3 essais) pour séparer
    l'erreur proportionnelle (diamètre) de l'erreur fixe. Noter aussi l'écart latéral réel ;
  - corriger le banc pour comparer la mesure réelle à la distance des encodeurs PAMI arrêté ;
  - puis l'entraxe (`r 360` et `r -360`), puis les carrés, puis les vitesses.

### T004 — Régulateur de vitesse : échelon, vitesse constante et escalier (banc test_pid, au sol)
- **Date** : 21/11/2026
- **Testeurs** : l'équipe
- **Programme utilisé** : `pami/test_bench/pid` (environnement `test_pid_wifi`, moniteur Wi-Fi)
- **Montage** : PCB 2026 5EO (D015), alimentation de labo 6 V, PAMI au sol (800 mm max par essai),
  les deux roues. Réglages : anticipation commune 10 % + 0,2 %/(mm/s), KP 0,05, KI 0,3.
- **Protocole** : échelon à 150 mm/s (`e`, 4 fois), vitesse constante à 150 mm/s (`c`, 1 fois, au sol),
  escalier 50 / 100 / 150 / 200 mm/s (`s`, 3 fois ; le premier arrêté par une touche à 5,2 s, après
  le début du palier 200).
- **Critère de réussite** (échelon à 150 mm/s, au sol) : montée à 90 % en 300 ms au plus,
  dépassement de 15 % au plus, erreur en régime établi de 5 mm/s au plus. Escalier : valeurs stables
  d'un essai à l'autre.
- **Résultat** : échelon en échec (dépassement), cause identifiée et corrigée ; escalier réussi.
- **Mesures** :
  - échelon (4 essais, gauche et droite quasi identiques) : montée 140 à 160 ms ; dépassement
    **28 à 30 %** (pointe 191 à 195 mm/s) ; régime établi 155 mm/s (erreur +4,6 à +5,3 mm/s) ;
    PWM moyenne 31 % = anticipation 40 % + correction P+I **−9 %** ;
  - vitesse constante : la vitesse redescend lentement de 190 à 151 mm/s en 3,5 s environ, PWM 30 % ;
  - escalier (vitesses réellement mesurées sur chaque palier : 65, 106, 155, 204 mm/s ; le palier
    « 50 » n'est jamais atteint en 1,5 s) :

    | Roue | Zone morte (3 essais) | Pente %/(mm/s) (3 essais) | Retenu (moyenne) |
    |---|---|---|---|
    | Gauche | 1,2 / 1,6 / 1,6 % | 0,195 / 0,191 / 0,191 | 1,5 % et 0,192 |
    | Droite | 1,0 / 1,0 / 1,2 % | 0,197 / 0,197 / 0,195 | 1,1 % et 0,196 |

- **Analyse** :
  - l'anticipation provisoire (10 + 0,2 × v) donnait 40 % à 150 mm/s alors qu'il en faut 31 % :
    chaque départ envoyait trop de PWM. La roue montait à 190 mm/s, puis l'intégrale mettait plus
    de 2 s à corriger (correction de −9 %). Ce n'est pas un problème de gains KP / KI ;
  - l'« erreur » du régime établi (+5 mm/s) vient de cette convergence lente, pas d'un écart permanent ;
  - les pentes sont stables à ±1 %. Les zones mortes varient de ±0,3 % de PWM : négligeable ;
  - les deux moteurs sont presque identiques (pentes à 2 % près) : bonne nouvelle pour les lignes droites ;
  - la droite n'est mesurée qu'entre 65 et 205 mm/s. En dessous (fin de mouvement à 40 mm/s,
    `MOTION_MIN_SPEED_MM_S`), le frottement au démarrage peut demander plus que 1,5 %.
- **Conclusion et suite** :
  - reporté dans `config.h` : `LEFT_MOTOR_DEADBAND_PCT = 1.5`, `LEFT_MOTOR_PCT_PER_MM_S = 0.192`,
    `RIGHT_MOTOR_DEADBAND_PCT = 1.1`, `RIGHT_MOTOR_PCT_PER_MM_S = 0.196` (// T004) ;
  - KP et KI inchangés ;
  - à vérifier (re-téléverser `test_pid`) : échelon à 150 mm/s, 3 fois. Attendu : dépassement ≤ 15 %,
    correction P+I proche de 0 (moins de 5 %), erreur ≤ 5 mm/s. Puis un échelon à 50 mm/s (basse
    vitesse, hors de la plage mesurée) : la roue doit démarrer franchement ;
  - ensuite seulement : diamètre avec `test_trajectoire` (`l 1000`, 5 fois), voir T003.

### T005 — Régulateur de vitesse avec l'anticipation mesurée (banc test_pid, au sol)
- **Date** : 21/11/2026
- **Testeurs** : l'équipe
- **Programme utilisé** : `pami/test_bench/pid` (environnement `test_pid_wifi`)
- **Montage** : comme T004 (PCB 2026 5EO, alimentation de labo 6 V, au sol, les deux roues).
  Anticipation mesurée en T004 : gauche 1,5 % + 0,192 %/(mm/s), droite 1,1 % + 0,196 %/(mm/s).
  KP 0,05 et KI 0,3 inchangés.
- **Protocole** : échelon à 150 mm/s (`e`, 3 fois), puis échelon à 50 mm/s (`m` 4 fois, puis `e`, 1 fois).
- **Critère de réussite** : montée à 90 % en 300 ms au plus, dépassement de 15 % au plus, erreur en
  régime établi de 5 mm/s au plus, correction P+I de moins de 5 % de PWM.
- **Résultat** : **réussi**, pour les deux roues, aux deux vitesses.
- **Mesures** :

  | Essai | Montée 90 % | Dépassement | Erreur établie | Correction P+I |
  |---|---|---|---|---|
  | 150 mm/s (3 fois, G et D) | 220 ms | 8 à 9 % (pointe 162 à 164) | +1,7 à +2,0 mm/s | +0,2 à +0,5 % |
  | 50 mm/s (G / D) | 220 ms | 8 % / 5 % | 0,0 mm/s | +0,8 % / +0,9 % |
  | Rappel T004, anticipation provisoire | 140 à 160 ms | 28 à 30 % | +4,6 à +5,3 mm/s | −9 % |

- **Analyse** :
  - l'anticipation fait maintenant presque tout le travail : la correction P+I est quasi nulle ;
  - la montée est un peu plus lente qu'en T004 (220 ms au lieu de 160 ms) parce que la roue ne
    reçoit plus de PWM en trop. Elle reste sous les 300 ms, et le dépassement est divisé par 3 ;
  - à 50 mm/s, hors de la plage mesurée en T004, la roue démarre franchement et tient sa vitesse.
    La correction un peu plus forte (+0,9 %) montre une zone morte réelle à peine plus grande que
    prévu : négligeable ;
  - les deux roues se comportent de façon identique.
- **Conclusion et suite** :
  - régulateur de vitesse validé à 50 et 150 mm/s, au sol, sous 6 V. KP et KI restent inchangés ;
  - quand `WHEEL_DIAMETER_MM` changera (étalonnage, T003), les vitesses en mm/s changeront de quelques
    % : refaire un escalier (`s`, 1 fois) pour remettre la pente à jour ;
  - à refaire sur batterie (tension différente) ;
  - suite : diamètre avec `test_trajectoire` (`l 1000` 5 fois, `l 300` 3 fois), puis entraxe.

### T006 — Diamètre des roues : lignes de 950 et 300 mm (banc test_trajectoire)
- **Date** : 21/11/2026
- **Testeurs** : l'équipe
- **Programme utilisé** : `pami/test_bench/trajectoire` (environnement `test_trajectoire_wifi`)
- **Montage** : PCB 2026 5EO, alimentation de labo 6 V, au sol. Anticipation de T004 (régulateur
  validé en T005). Réglages : diamètre 60 mm, entraxe 100 mm, 150 mm/s.
- **Protocole** : `l 950` 5 fois, `l 300` 3 fois, distance réelle au mètre ruban.
  Les mesures ont été tapées « m <925> » : le banc les a refusées (« Nombre illisible »), à cause des
  chevrons que montrait son propre message. Le calcul ci-dessous est fait à la main, avec la distance
  « crue en fin de mouvement » affichée par le banc. Le glissement après l'arrêt n'a donc pas été relevé.
  Banc corrigé le même jour : chevrons ignorés, messages avec un vrai exemple (`m 947`).
- **Critère de réussite** : distance à 1 % près, dispersion des essais < 1 %.
- **Résultat** : partiel. Diamètre déterminé ; une perte fixe d'environ 8 mm par ligne reste à expliquer.
- **Mesures** :

  | Commande | Cru (fin de mouvement) | Réel | Rapport moyen |
  |---|---|---|---|
  | `l 950` ×5 | 947,3 / 948,5 / 948,0 / 947,0 / 947,6 | 925 / 935 / 935 / 923 / 937 | 0,982 (−1,8 %), écart-type 6,5 mm (0,7 %) |
  | `l 300` ×3 | 298,3 / 297,4 / 297,7 | 290 / 285 / 287 | 0,965 (−3,5 %), écart-type 2,5 mm |

  Le PAMI croyait aller droit dans tous les essais (y = 0 mm, cap −0,1° à 0,2°), PWM max 37 %.
  Écart latéral réel : pas relevé.
- **Analyse** :
  - l'erreur n'est pas la même en pourcentage à 950 et à 300 mm : elle n'est donc pas seulement due
    au diamètre. Droite ajustée sur les 8 essais : **réel = 0,9905 × cru − 7,6 mm**
    (k à ± 0,006, constante à ± 4 mm) ;
  - la partie proportionnelle (−0,95 %) est l'erreur de diamètre : **59,4 mm** (± 0,3 mm). Un
    glissement de fin à peu près constant ne change pas cette pente : il ne déplace que la constante ;
  - **vérification indépendante** : en T003, `r 360` donnait 356° (−1,1 %). Avec 59,4 mm, la même
    rotation donnerait 356 / 0,9905 ≈ 359,4°. Les rotations confirment le diamètre, et l'entraxe de
    100 mm semble juste (l'estimation de 98,9 mm faite en T003 venait de lignes trop courtes, faussées
    par la perte fixe) ;
  - la perte fixe d'environ 8 mm par ligne (peut-être plus, glissement de fin compris) pèse 2,6 % sur
    300 mm, 0,8 % sur 950 mm. Causes possibles : patinage au démarrage, roulette folle qui pivote au
    départ (le PAMI a été ramené en arrière entre deux essais), ou méthode de mesure (repère, zéro du mètre).
- **Conclusion et suite** :
  - reporté dans `config.h` : `WHEEL_DIAMETER_MM = 59.4` (// T006). Les pentes d'anticipation de T004
    passent à 0,194 (gauche) et 0,198 (droite), soit × 60 / 59,4, car elles sont exprimées en mm/s ;
  - à vérifier après téléversement : `l 950` ×3 (attendu : réel ≈ cru − 8 mm), et `r 360` / `r -360` ×3
    (attendu : 358 à 362°, l'entraxe ne change pas) ;
  - perte fixe : `l 100` ×3 (si l'écart reste d'environ 8 mm, il est bien fixe), en tapant bien `m 92`
    pour voir le glissement de fin ; essayer de ne pas faire pivoter la roulette avant le départ
    (pousser le PAMI vers l'avant sur 2 cm avant chaque essai) ; vérifier le repère de mesure.

### T007 — Entraxe : rotations d'un tour dans les deux sens (banc test_trajectoire)
- **Date** : 21/11/2026
- **Testeurs** : l'équipe
- **Programme utilisé** : `pami/test_bench/trajectoire` (environnement `test_trajectoire_wifi`), version
  qui relit l'angle cru PAMI arrêté et accepte les angles négatifs.
- **Montage** : PCB 2026 5EO, alimentation de labo 6 V, au sol. Diamètre 59,4 mm (T006), entraxe
  100 mm, rotation à 90 mm/s par roue, anticipation de T004/T006.
- **Protocole** : `r 360` et `r -360` en alternance, 3 fois chacun. Angle réel au rapporteur.
- **Critère de réussite** : angle à 5° près par tour, dispersion < 1 %.
- **Résultat** : partiel. Entraxe déterminé ; un écart de 4° entre les deux sens et un dépassement à
  l'arrêt restent à traiter.
- **Mesures** :

  | Sens | Cru en fin de mouvement | Glissement après l'arrêt | Cru PAMI arrêté | Réel | Entraxe effectif |
  |---|---|---|---|---|---|
  | Gauche (`r 360`) | 358,2 / 358,7 / 359,1° | 4,3 / 3,9 / 3,8° | 362,6 / 362,6 / 362,9° | 355 / 355 / 355° | 102,2 mm |
  | Droite (`r -360`) | 358,0 / 358,5 / 359,1° | 4,4 / 4,2 / 3,7° | 362,4 / 362,6 / 362,8° | 359 / 359 / 359° | 101,0 mm |

  Le PAMI reste sur place pendant les rotations (x et y calculés à 0 mm). PWM max 22 %.
- **Analyse** :
  - entraxe effectif moyen : **101,6 mm** (le banc conseille 101,6 mm). Il est plus grand que les 100 mm
    mesurés à la règle : c'est classique, les pneus frottent quand le PAMI tourne sur place ;
  - **écart entre les deux sens** : toujours 355° à gauche et 359° à droite, très reproductible. L'entraxe
    ne corrige que la moyenne. Une différence de diamètre entre les roues ne donnerait pas cet écart en
    rotation sur place. Pistes : la roulette folle (elle se retourne à chaque changement de sens), un
    centre de rotation décalé, ou la façon de mesurer. Les valeurs identiques à chaque essai font penser
    à un rapporteur lu au degré près : mesure au laser sur un mur conseillée (1° = 35 mm à 2 m) ;
  - **dépassement à l'arrêt** : le PAMI s'arrête environ 1,5° avant la cible (tolérance de 2 mm d'arc),
    puis glisse de 4° (environ 3,5 mm d'arc par roue) au freinage. Il finit donc **2,6° trop loin**, dans
    les deux sens. Ce n'est pas un défaut d'étalonnage mais de fin de mouvement : l'entraxe est calculé
    PAMI arrêté, glissement compris ;
  - correction de la conclusion de T006 : la rotation de T003 (356°) ne confirmait pas le diamètre comme
    annoncé. En T003, l'anticipation trop forte faisait tourner les roues trop vite et glisser davantage.
    Le diamètre de T006 reste fondé sur les lignes de 950 et 300 mm ;
  - piste générale : en ligne (T006) comme en rotation, le PAMI perd un peu de mouvement que les encodeurs
    ne voient pas. Le sol des essais compte : refaire l'étalonnage sur le vinyle de la table de jeu.
- **Conclusion et suite** :
  - reporté dans `config.h` : `WHEEL_BASE_MM = 101.6` (// T007) ;
  - attendu avec ce réglage et le même glissement : environ 360,6° à gauche, 364,7° à droite pour `r 360`.
    Sans le dépassement de 2,6°, ce serait 357,9° et 362,1° ;
  - à faire : tenir compte du glissement dans la décision d'arrêt (rotations et lignes), mesurer le
    glissement des lignes (`l 950`, le banc l'affiche), remesurer les rotations au laser, puis les carrés
    dans les deux sens (`c 300`, `c -300`).

### T008 — Glissement en fin de ligne droite (banc test_trajectoire, diamètre 59,4 mm)
- **Date** : 21/11/2026
- **Testeurs** : l'équipe
- **Programme utilisé** : `pami/test_bench/trajectoire` (environnement `test_trajectoire_wifi`)
- **Montage** : comme T006, avec `WHEEL_DIAMETER_MM = 59.4` et `WHEEL_BASE_MM = 101.6`.
- **Protocole** : `l 950` 3 fois, distance réelle au mètre ruban, saisie avec `m`.
- **Critère de réussite** : glissement mesuré et régulier ; résultat cohérent avec T006.
- **Résultat** : réussi.
- **Mesures** :

  | Essai | Cru en fin de mouvement | Glissement | Cru PAMI arrêté | Réel | Réel − cru arrêté |
  |---|---|---|---|---|---|
  | 1 | 947,4 mm | 5,0 mm | 952,4 mm | 935 mm | −17,4 mm |
  | 2 | 948,0 mm | 4,9 mm | 952,8 mm | 940 mm | −12,8 mm |
  | 3 | 947,5 mm | 5,3 mm | 952,8 mm | 941 mm | −11,8 mm |
  | Moyenne | 947,6 mm | **5,1 mm** | 952,7 mm | **938,7 mm** (écart-type 3,2 mm) | **−14,0 mm** |

  Cap cru entre −0,2° et +0,1° ; PWM max 36 %. Écart latéral réel : pas relevé.
- **Analyse** :
  - le glissement après la fin du mouvement est très régulier : **5 mm** à 150 mm/s. Le PAMI s'arrête
    environ 2,4 mm avant la cible (tolérance de 3 mm), puis glisse de 5 mm : les encodeurs le voient
    finir 2,7 mm au-delà des 950 mm demandés ;
  - le réel (938,7 mm) correspond à ce que prévoyait T006 avec 59,4 mm (environ 940 mm) : le diamètre
    est confirmé ;
  - il reste une perte d'environ **13 mm par ligne** que les encodeurs ne voient pas (−7,6 mm par rapport
    au cru de fin de mouvement en T006, auxquels s'ajoutent les 5 mm de glissement). Elle ne dépend pas
    de la longueur (T006). Pistes : patinage au démarrage, jeu des engrenages (l'encodeur est sur l'axe du
    moteur, avant le réducteur 120:1), roulette folle, méthode de mesure ;
  - le « diamètre conseillé » du banc (58,5 mm) met toute l'erreur sur le diamètre : **ne pas l'appliquer**.
- **Conclusion et suite** :
  - aucune valeur changée ; diamètre 59,4 mm confirmé ;
  - pour la perte fixe : `l 100` ×3 (si l'écart reste d'environ 13 mm, il est bien fixe), et un essai en
    avançant le PAMI de 2 cm avant de le poser au départ (roulette déjà orientée) ;
  - le glissement de 5 mm en ligne et de 4° en rotation (T007) servira à corriger la décision d'arrêt.

### T009 — Carrés de 300 mm dans les deux sens (banc test_trajectoire) — résultat partiel
- **Date** : 21/11/2026
- **Testeurs** : l'équipe
- **Programme utilisé** : `pami/test_bench/trajectoire` (environnement `test_trajectoire_wifi`)
- **Montage** : diamètre 59,4 mm (T006), entraxe 101,6 mm (T007), au sol, croix de scotch au départ.
- **Protocole** : `c 300` et `c -300`, plusieurs fois chacun.
- **Critère de réussite** : retour à moins de 30 mm du départ, dans les deux sens.
- **Résultat** : réussi pour le critère (10 à 15 mm). Mesures détaillées pas encore transmises : seul le
  résumé de l'équipe est connu.
- **Mesures** : dans les deux sens, le PAMI s'arrête toujours **10 à 15 mm avant la croix**, dans le sens
  d'où il arrive (dernier côté trop court). Il ne la dépasse jamais. Écart devant / derrière et cap final :
  pas relevés.
- **Analyse** :
  - un écart constant par ligne s'annule entre côtés opposés : il ne peut pas expliquer cet arrêt avant la
    croix. Ce sont les erreurs de cap des quarts de tour 1 et 2 : écart = 300 mm × (δ1 + δ2) ;
  - 10 à 15 mm avant la croix dans les deux sens : chaque quart de tour tourne **trop peu, d'environ 1,2°**
    (entre 0,95 et 1,4°). Les deux sens se comportent pareil : pas de signe d'une différence de diamètre
    entre les roues ;
  - c'est le contraire de la prévision tirée de T007 (quarts de tour trop longs de 2 à 3°, avec un modèle
    où l'erreur est proportionnelle à l'angle). Explication possible : comme pour les lignes (perte fixe
    d'environ 13 mm, T006 et T008), **chaque rotation perdrait un angle fixe**, d'environ 5°, que les
    encodeurs ne voient pas. Calé sur T007 et ces carrés, ce modèle donne un entraxe effectif d'environ
    100 mm (et non 101,6 mm) ;
  - les deux modèles se départagent facilement : 4 × `r 90` à la suite font environ **370°** au total avec
    le modèle proportionnel, environ **355°** avec le modèle « perte fixe par rotation ». Un seul `r 360`
    fait environ 363° dans les deux cas.
- **Conclusion et suite** :
  - aucune valeur changée pour l'instant ;
  - test à faire : 4 × `r 90` sans toucher au PAMI, puis angle total au rapporteur (ou au laser) ; idem
    4 × `r -90` ; et un `r 360` pour comparer ;
  - sur les carrés, relever aussi l'écart devant / derrière (attendu : 10 à 15 mm **derrière**, si l'erreur
    vient bien des rotations) et le cap final (attendu : environ 5° de moins qu'un tour complet).

### T010 — Quarts de tour enchaînés et tours complets (banc test_trajectoire)
- **Date** : 21/11/2026
- **Testeurs** : l'équipe
- **Programme utilisé** : `pami/test_bench/trajectoire` (environnement `test_trajectoire_wifi`)
- **Montage** : diamètre 59,4 mm (T006), entraxe 101,6 mm (T007), au sol.
- **Protocole** : 4 × `r 90` sans toucher au PAMI, angle total mesuré à la fin (2 fois) ; 4 × `r -90`
  (1 fois) ; `r 360` et `r -360` (1 fois chacun). Pour départager les deux explications de T009.
  Les « m 367 » tapés après le 4e quart de tour ont été comptés par le banc comme la mesure du
  **dernier** quart de tour seul : la série de rotations et les « entraxe conseillé » de cette séance
  sont faux (consigne mal donnée). Les calculs ci-dessous sont faits à la main.
- **Critère de réussite** : savoir si l'erreur de rotation est proportionnelle à l'angle, ou fixe à chaque rotation.
- **Résultat** : réussi (question tranchée), mais une contradiction avec les carrés apparaît.
- **Mesures** :

  | Essai | Cru en fin de mouvement | Glissement | Cru PAMI arrêté | Réel |
  |---|---|---|---|---|
  | 4 × `r 90` (2 fois) | 88,9° par quart en moyenne | 4,5 à 4,8° | 93,6° par quart (374° pour 4) | **367°** et **367°** |
  | 4 × `r -90` | 89,2° par quart | 4,6° | 93,6° par quart | **367°** |
  | `r 360` | 358,9° | 4,5° | 363,4° | **360°** |
  | `r -360` | 359,0° | 4,5° | 363,6° | **360°** |

- **Analyse** :
  - l'hypothèse « angle fixe perdu à chaque rotation » (T009) est **rejetée** : elle prévoyait environ 355°
    pour 4 quarts de tour, on mesure 367°. Le modèle proportionnel (T007) prévoyait environ 370° : il est
    proche ;
  - avec l'entraxe de 101,6 mm, un tour complet tombe **pile sur 360°**, dans les deux sens. L'écart de 4°
    entre les deux sens vu en T007 n'apparaît plus : il venait probablement de la mesure ;
  - un quart de tour isolé va **trop loin d'environ 1,75°** (91,75° réels). Ce dépassement vient de la
    fin du mouvement (le PAMI coupe environ 1° avant, puis glisse de 4,6°). Il pèse beaucoup plus sur
    90° que sur 360° ;
  - **contradiction avec les carrés (T009)** : avec +1,75° par quart de tour, un carré devrait finir
    environ 18 mm **après** la croix ; on l'observe 10 à 15 mm **avant** (−1,2° par quart de tour).
    Les quarts de tour d'un carré se comportent donc autrement que des quarts de tour enchaînés.
    Différence : dans le carré, chaque rotation suit une ligne droite et **démarre aussitôt la ligne
    finie, pendant que le PAMI glisse encore** (environ 5 mm). La roulette folle, alignée par la ligne,
    doit aussi pivoter au début de la rotation. La mission du match enchaîne ses mouvements de la même
    façon que le banc.
- **Conclusion et suite** :
  - entraxe 101,6 mm conservé ;
  - test à faire : un carré « à la main », commande par commande (`l 300`, `r 90`, `l 300`, `r 90`…), ce
    qui laisse plusieurs secondes d'arrêt entre deux mouvements. S'il finit après la croix (comme prévu
    avec +1,75°), c'est l'enchaînement immédiat qui fait perdre de l'angle ; s'il finit encore avant,
    c'est plutôt la roulette (passage de la ligne à la rotation) ;
  - taper `n` pour effacer la série de rotations faussée avant les prochaines mesures.

### T011 — Carré « à la main », commande par commande (banc test_trajectoire)
- **Date** : 21/11/2026
- **Testeurs** : l'équipe
- **Programme utilisé** : `pami/test_bench/trajectoire` (environnement `test_trajectoire_wifi`)
- **Montage** : comme T010 (diamètre 59,4 mm, entraxe 101,6 mm), croix de scotch au départ.
- **Protocole** : `l 300`, `r 90`, `l 300`, `r 90`, `l 300`, `r 90`, `l 300`, `r 90`, tapés un par un :
  le PAMI reste arrêté plusieurs secondes entre deux mouvements. Écart à la croix mesuré à la fin.
- **Critère de réussite** : départager les deux causes possibles de T010 (enchaînement immédiat ou roulette).
- **Résultat** : réussi (cause trouvée).
- **Mesures** : le PAMI finit **15 à 20 mm après la croix**, dans le sens d'où il arrive.
- **Analyse** :
  - c'est la prévision faite avec +1,75° par quart de tour (T010) : environ 18 mm après la croix ;
  - le carré enchaîné du banc (T009) finissait 10 à 15 mm avant : quand une rotation démarre aussitôt la
    ligne finie, pendant que le PAMI glisse encore, chaque quart de tour perd environ 3° (de +1,75° à −1,2°).
    La cause est **l'enchaînement immédiat**, pas la roulette ;
  - la mission du match enchaîne ses mouvements de la même façon : le problème la concerne directement ;
  - deux défauts de fin de mouvement restent donc : l'enchaînement pendant le glissement, et le
    dépassement dû au glissement (+1,75° par quart de tour, environ +3 mm par ligne).
- **Conclusion et suite** : corriger la fin de mouvement dans `motion` (attendre l'arrêt réel avant de
  déclarer le mouvement fini, et couper plus tôt de la longueur du glissement), puis refaire l'entraxe
  (4 × `r 90` et `r 360`) et les carrés.

### T012 — Premier ultrason sur le PCB 2026 5EO (banc test_ultrason)
- **Date** : 21/11/2026
- **Testeurs** : l'équipe
- **Programme utilisé** : `pami/test_bench/ultrason` (environnement `test_ultrason_wifi`), TRIG commun GPIO 21.
- **Montage** : un seul HC-SR04 (le deuxième n'est pas encore là). Vérifications faites par l'équipe avant
  de brancher (pont diviseur, 5 V, ordre des broches).
- **Protocole** : capteur sur le connecteur de GPIO 2 (« gauche »), puis sur celui de GPIO 15 (« droit ») ;
  boîte à 10, 15, 20 et 30 cm, puis boîte retirée.
- **Critère de réussite** : boîte vue à ± 10 mm de 10 à 100 cm, 0 « sans écho », environ 16 mesures/s.
- **Résultat** : partiel. Le capteur marche sur le connecteur de GPIO 15 ; le connecteur de GPIO 2 ne
  donne rien.
- **Mesures** :
  - connecteur GPIO 2 : 0 mesure pour 16 à 17 envois par seconde. ECHO reste à l'état bas en
    permanence (sinon le banc ralentirait à environ 8 envois par seconde) ;
  - connecteur GPIO 15 : 16 à 17 mesures par seconde, aucune perdue, 0 « sans écho » ;

    | Boîte annoncée | Distance lue (filtrée) | Écart |
    |---|---|---|
    | 10 cm | 114 à 115 mm | +15 mm |
    | 15 cm | 159 à 166 mm (≈ 162) | +12 mm |
    | 20 cm | 201 à 214 mm (≈ 208) | +8 mm |
    | 30 cm | 295 à 306 mm (≈ 300) | 0 |
    | boîte retirée | 504 à 521 mm (un objet vers 51 cm) | — |

  - quelques mesures isolées très longues (876 et 981 mm) pendant le retrait de la boîte : refusées par
    le filtre de montée, la distance filtrée est restée vers 506 à 577 mm ;
  - le bilan annonce bien « FREINERAIT (D) » sous 150 mm et plus au-dessus.
- **Analyse** :
  - le capteur, le TRIG commun (GPIO 21), l'alimentation des capteurs et la lecture sur GPIO 15
    fonctionnent ; la mesure est très stable (± 1 mm sur un objet fixe) ;
  - l'écart de +12 à +15 mm à 10 et 15 cm est à confirmer : il dépend d'où la distance de la boîte a été
    mesurée (face avant des cylindres du capteur, ou bord du PAMI ?). S'il est réel, le PAMI verrait les
    obstacles proches un peu plus loin qu'ils ne sont, et s'arrêterait vers 135 mm au lieu de 150 mm ;
  - le connecteur de GPIO 2 reste à diagnostiquer avant l'arrivée du deuxième capteur : liaison, pont
    diviseur, LED de la carte sur GPIO 2, ou broche abîmée.
- **Conclusion et suite** :
  - premier ultrason validé sur le connecteur de GPIO 15 ;
  - remesurer 10 et 15 cm depuis la face avant du capteur ;
  - diagnostiquer le connecteur de GPIO 2 (multimètre : tension sur GPIO 2 capteur branché vers le vide,
    mode de référence `r`) ;
  - le test « rien devant pendant 10 s » (échos croisés) se fera avec les deux capteurs.
- **Complément (même séance), distances remesurées depuis la face avant du capteur** (connecteur GPIO 15,
  bilans par seconde avec la touche `v`) :

  | Boîte | Distance lue (bilan) | Écart |
  |---|---|---|
  | 10 cm | 101 mm (100 à 102), puis 98 à 100 mm | ≈ 0 |
  | 15 cm | 149 à 152 mm | ≈ 0 |
  | 20 cm | 192 à 199 mm (≈ 195) | −5 mm |
  | 30 cm | 291 à 304 mm (≈ 294) | −6 mm |
  | vers 5 cm | 42 à 59 mm | mesure encore : zone aveugle plus courte que 5 cm |
  | boîte retirée | 576 à 589 mm | objet vers 58 cm |

  - l'écart de +12 à +15 mm de la première série venait du repère de mesure : il disparaît depuis la face
    du capteur. **Critère ± 10 mm respecté de 10 à 30 cm** ; 16 à 17 mesures/s, 0 écho raté ;
  - à 150 mm pile, la ligne `[BILAN]` alterne entre « FREINERAIT » et non : elle compare au seuil sans
    marge. `safety`, lui, ne relâche qu'au-delà de 180 mm (`OBSTACLE_CLEAR_MARGIN_MM`) : pas de va-et-vient en match ;
  - **ultrason validé sur le connecteur GPIO 15, TRIG commun GPIO 21 validé.** Reste : le connecteur GPIO 2.

### T013 — Sens des moteurs avec le brochage du 25/11/2026 (banc test_moteurs)
- **Date** : 25/11/2026
- **Testeurs** : l'équipe
- **Programme utilisé** : `pami/test_bench/moteurs` (environnement `test_moteurs`), brochage D018 :
  gauche IN1 27 / IN2 26, droit IN1 16 / IN2 17, `MOTOR_LEFT_INVERTED = true`, `MOTOR_RIGHT_INVERTED = false`.
- **Critère de réussite** : chaque roue tourne vers l'avant (côté ultrasons) quand on lui demande d'avancer.
- **Résultat** : partiel. Roue droite : bon sens. Roue gauche : à l'envers.
- **Correction** : `MOTOR_LEFT_INVERTED` passe de `true` à `false` dans `config.h`. Les broches ne
  changent pas : le réglage échange en logiciel le rôle des entrées A et B du moteur gauche.
- **À refaire** : `test_moteurs` avec la correction (roue gauche vers l'avant), puis le sens des
  encodeurs avec `test_odometrie` (protocole 1). Ce réglage-là est indépendant de celui des moteurs.

### T014 — Sens des encodeurs avec le brochage du 25/11/2026 (banc test_odometrie)
- **Date** : 25/11/2026
- **Testeurs** : l'équipe
- **Programme utilisé** : `pami/test_bench/odometrie` (environnement `test_odometrie_wifi`), brochage D018,
  `MOTOR_LEFT_INVERTED = false` (T013), `ENCODER_LEFT_INVERTED = false`, `ENCODER_RIGHT_INVERTED = true`.
- **Protocole** : roue gauche tournée à la main vers l'avant, puis roue droite ; puis touche `1` (ligne droite).
- **Critère de réussite** : roue tournée vers l'avant → son compteur augmente.
- **Résultat** : échec. Les **deux** compteurs descendent quand la roue tourne vers l'avant.
- **Mesures** :
  - roue gauche à la main : ticks G 0 → −259 ; roue droite à la main : ticks D 0 → −272 ;
  - touche `1` : PWM positives (G 42 %, D 48 %), mais vitesses mesurées négatives (G −205, D −448 mm/s).
    Le régulateur croit que les roues reculent et monte la PWM (D à 100 %) ; au bout de 0,6 s :
    `[MOT] PANNE : roue bloquee ou encodeur inverse`, tout s'arrête. La détection de panne a joué son rôle.
- **Correction** : `ENCODER_LEFT_INVERTED` false → **true**, `ENCODER_RIGHT_INVERTED` true → **false**.
- **À refaire** : roues à la main (les deux compteurs doivent monter), puis `1` sur cales, puis au sol.
- **Complément (après correction)** : roues à la main, les deux compteurs montent. Touche `1` (ligne de
  250 mm) : le PAMI avance dans le bon sens, sans panne. `DRIVE_SPEED_MM_S` mis à 200 par l'équipe pour ces essais.
  - le PAMI **dévie nettement vers la droite** en réalité (écart non mesuré), alors que l'odométrie croit
    aller droit : y = 2 à 5 mm et cap entre −3° et +3° à l'arrivée. La déviation ne se voit pas dans les
    encodeurs : elle vient de ce qui se passe entre l'axe du moteur et le sol ;
  - frottement probable de la roue gauche sur la coque : à vitesse égale, la roue gauche demande plus de
    PWM que la droite (avant serrage des roues : G 92 % / D 71 % à 200 mm/s ; après : G 41-47 % / D 35-37 %
    vers 220 mm/s), alors que les deux moteurs demandaient la même chose au réglage (T004) ;
  - à faire : supprimer le frottement, vérifier que chaque roue ne glisse pas sur son axe (trait de feutre
    sur la roue et l'axe), puis mesurer l'écart réel sur une ligne de 950 mm (`test_trajectoire`).

### T015 — Déviation vers la droite en ligne droite, PAMI sans coque (banc test_odometrie)
- **Date** : 25/11/2026
- **Testeurs** : l'équipe
- **Programme utilisé** : `pami/test_bench/odometrie` (`test_odometrie_wifi`), brochage D018, sens corrigés
  (T013, T014). `DRIVE_SPEED_MM_S = 350`, `MOTION_ACCEL_MM_S2 = 400`. Ligne droite de 250 mm (touche `1`), 3 fois.
- **Montage** : coque retirée (pour écarter un frottement), au sol.
- **Résultat** : échec. Le PAMI **dévie toujours nettement vers la droite**, même sur 250 mm (écart réel
  non mesuré). Selon l'équipe, il allait droit samedi 21/11 (à 150 mm/s ; écart réel non relevé ce jour-là).
- **Mesures** (les 3 essais sont presque identiques) :
  - ce que croit l'odométrie à l'arrivée : x = 248 mm, y = 4-5 mm, cap −1,0° à −1,5° ; même nombre de ticks
    sur les deux roues (2628 / 2627 au 1er essai). Pour les encodeurs, le PAMI est allé droit ;
  - vitesse maximale environ 330 mm/s ;
  - PWM pendant l'accélération, à vitesse égale : G 67-76 % contre D 51-56 % (vers 300 mm/s,
    l'anticipation de T004 prévoit environ 60 % des deux côtés). Au freinage : G 37-38 % / D 39-41 %.
- **Analyse** :
  - le code de déplacement (`motion`, `encoders`, `motor_driver`, régulateur, odométrie) est identique
    à celui de samedi (commit du 21/11, 21 h 27). Depuis, seuls ont changé dans `config.h` : les broches
    (nouveau brochage D018), le sens des moteurs et des encodeurs (T013, T014), et la vitesse (150 → 350 mm/s) ;
  - une déviation que les encodeurs ne voient pas vient de ce qui se passe entre le comptage des ticks
    et le sol : roue qui patine ou qui glisse sur son axe, roues de diamètres différents, ou encodeur
    qui perd des ticks seulement à grande vitesse. La PWM plus forte à gauche et plus faible à droite
    va dans le sens d'une roue droite qui patine, ou d'un encodeur gauche qui perd des ticks.
- **Essais décidés** :
  1. même ligne à **150 mm/s** (valeur de samedi, mise temporairement dans `config.h`) ;
  2. 10 tours de chaque roue à la main : chaque compteur doit avancer d'environ 19 200 ticks ;
  3. mesure de l'écart réel sur le côté, à 150 et à 350 mm/s.
- **Complément (essai 1, à 150 mm/s)** : le PAMI dévie **toujours vers la droite**, un peu moins qu'à
  350 mm/s (écart réel non mesuré). La vitesse n'est donc pas la seule cause : il y a une dérive de base,
  et elle augmente avec la vitesse. L'équipe pense ne pas avoir bien observé la ligne droite samedi.
  Les surfaces de roulage diffèrent d'un essai à l'autre.

### T016 — Deux ultrasons avec le brochage du 25/11/2026 (banc test_ultrason)
- **Date** : 25/11/2026
- **Testeurs** : l'équipe
- **Programme utilisé** : `pami/test_bench/ultrason` (Wi-Fi), gauche TRIG 25 / ECHO 35, droit TRIG 14 / ECHO 12.
- **Résultat** : partiel.
  - **Gauche** : marche. 16 à 17 mesures par seconde, aucune perdue ; mur vu à 1239-1243 mm ; main suivie
    de 91 à 438 mm.
  - **Droit** : rien. 0 mesure et 0 « sans écho » pour 15 à 17 envois par seconde : la broche ECHO (12)
    ne change jamais d'état. `[SENS] PANNE ultrason droit` au bout de 1 s, et le bilan annonce bien
    « safety FREINERAIT (D) » : la sécurité par défaut fonctionne.
- **Piste principale** : dans le programme Controller de l'équipe, TRIG 14 / ECHO 12 est le « connecteur
  du bas », et le connecteur droit est TRIG 5 / ECHO 4, avec TRIG et ECHO inversés (« miroir »).
- **À faire** : mode de référence (`r`), échange des deux capteurs, vérification du connecteur utilisé.
- **Complément (mode de référence `r`)** : gauche toujours bon (81 à 195 mm main devant, puis mur à
  1234-1251 mm). Droit : « aucune impulsion ECHO en 70 ms » à chacun des plus de 1400 essais, en
  déclenchant le capteur droit seul sur GPIO 14 et en lisant GPIO 12 avec pulseIn(). Le code du firmware
  est donc hors de cause : le signal n'arrive pas sur GPIO 12 (câblage, connecteur, broches, capteur).
- **Complément (résolu)** : l'équipe a corrigé le problème ; l'ultrason droit mesure bien maintenant.
  Cause et mesures à préciser par l'équipe.

### T017 — Bras d'attaque (banc test_bras)
- **Date** : 25/11/2026
- **Testeurs** : l'équipe
- **Programme utilisé** : `pami/test_bench/bras` (D021).
- **Résultat** : réussi selon l'équipe (« le servo marche très bien »). Angles réels, périmètre bras
  déployé et tenue de 3 s : à noter ici pour cocher le critère de réussite du banc.
- **Cause trouvée par l'équipe** : le capteur droit est sur le connecteur droit du PCB, relié à TRIG 5 /
  ECHO 4 (14 / 12 est le connecteur du bas). `config.h` corrigé : `PIN_US_RIGHT_TRIG = 5`, `PIN_US_RIGHT_ECHO = 4`.

### T018 — Échelon à 350 mm/s, roues en l'air (banc test_pid)
- **Date** : 25/11/2026
- **Testeurs** : l'équipe
- **Programme utilisé** : `pami/test_bench/pid`, les deux roues, échelon `e` à 350 mm/s. PAMI sur cales
  d'après l'équipe, mais le banc était réglé « AU SOL » : l'essai s'est arrêté à 1000 mm comptés (2,8 s).
- **But** : savoir si l'écart de PWM entre les roues vient du sol (adhérence) ou des moteurs.
- **Mesures** (régime établi) :

  | Roue | Vitesse moyenne | PWM moyenne | Anticipation (T004, au sol) | Correction P+I | Montée à 90 % | Dépassement |
  |---|---|---|---|---|---|---|
  | Gauche | 354 mm/s | 79,1 % | 69,4 % | +9,7 % | 340 ms | 11 % |
  | Droite | 360 mm/s | 64,7 % | 70,4 % | −5,7 % | 220 ms | 15 % |

- **Analyse** :
  - même roues en l'air, la roue gauche demande environ 14 points de PWM de plus que la droite pour la
    même vitesse. L'écart ne vient donc pas du sol : la transmission gauche (moteur, réducteur, roue)
    est plus dure à faire tourner ;
  - sans charge, une roue devrait demander moins que l'anticipation mesurée au sol : c'est le cas à
    droite (−5,7), pas à gauche (+9,7). Samedi (T004, T005), les deux roues suivaient l'anticipation
    (correction P+I proche de 0) : la gauche est devenue plus dure depuis le remontage ;
  - ce frottement ne crée pas à lui seul une déviation invisible (la régulation égalise les tours de
    roue), mais il retarde la roue gauche à chaque accélération et lui retire de la marge à haute vitesse.
- **À faire** : roues tournées à la main moteurs non alimentés (comparer gauche et droite), vis de la
  roue gauche, roue qui touche le moteur ou le châssis, support moteur ; puis refaire cet essai en
  mode « SUR CALES » (touche `k`).
- **Complément (roue gauche desserrée, vrai mode SUR CALES, 7 échelons à 350 mm/s)** : rien ne change.
  PWM moyenne gauche 79,0 / 79,3 / 76,9 / 76,0 / 75,4 / 79,6 / 74,3 % ; droite 65,1 / 63,7 / 62,6 / 62,9 /
  62,1 / 62,4 / 61,9 %. L'écart reste de 12 à 14 points ; les deux baissent un peu d'un essai à l'autre
  (échauffement). Vitesses moyennes égales à quelques mm/s près : la régulation compense. Montée à 90 % :
  gauche 300-400 ms, droite 200-240 ms. Dépassement droit 15-23 % (anticipation trop forte en l'air : normal).
  La vis de la roue n'est donc pas en cause : la différence est dans le moteur, le réducteur ou leur
  alimentation, ou bien les deux moteurs n'ont pas le même rapport ticks / tour de roue.
  Essai décisif suivant : 10 tours de chaque roue à la main (environ 19 200 ticks attendus de chaque côté).

### T019 — Ticks par tour de roue, 10 tours à la main (banc test_odometrie)
- **Date** : 25/11/2026
- **Testeurs** : l'équipe
- **Protocole** : trait au feutre sur chaque roue, 10 tours exacts vers l'avant à la main, roue par roue.
- **But** : vérifier que les deux moteurs ont le même rapport ticks / tour de roue (même modèle de
  moteur, même encodeur).
- **Mesures** :

  | Roue | Départ | Arrivée | Ticks pour 10 tours | Par tour | Écart avec 1920 |
  |---|---|---|---|---|---|
  | Gauche | 0 | 19 239 | 19 239 | 1923,9 | +0,2 % |
  | Droite | −240 | 19 223 | 19 463 | 1946,3 | +1,4 % |

- **Résultat** : les deux moteurs ont le même rapport (un autre modèle donnerait un écart de 20 % ou
  plus). Hypothèse « moteurs différents » écartée. Il reste un écart de 1,2 % (droite plus haute), soit
  environ 45° de roue sur 10 tours : à confirmer, car il peut venir de l'alignement du trait au départ
  de la roue droite (compteur à −240 au départ).
- **Si l'écart est réel** : à nombre de ticks égal, la roue droite fait 1,2 % de tours de moins, donc le
  PAMI tourne à droite, dans le sens observé. Ordre de grandeur : 4 mm sur 250 mm, environ 50 mm sur
  950 mm, environ 210 mm sur les 1900 mm de la douve.
- **Complément (recomptage roue droite)** : 19 174 → 38 659, soit **19 485 ticks** pour 10 tours
  (1948,5 par tour). Avec la 1re mesure (19 463), l'écart droite / gauche de **1,2 %** est confirmé.

### T020 — Dérive réelle sur une ligne de 800 mm, à 100 et 350 mm/s
- **Date** : 25/11/2026
- **Testeurs** : l'équipe
- **Montage** : `MOTION_ACCEL_MM_S2 = 200`, au sol, ligne de 800 mm.
- **Mesures** : écart réel sur le côté à l'arrivée, vers la droite : **53 mm à 100 mm/s**, **340 mm à 350 mm/s**.
- **Analyse** :
  - l'écart de 1,2 % des ticks par tour (T019) donne environ 38 mm sur 800 mm : il explique presque
    toute la dérive à 100 mm/s ;
  - à 350 mm/s, 340 mm correspondraient à environ 11 % d'écart entre les roues : ce n'est plus un
    défaut de diamètre ou de rapport, c'est un défaut qui grandit avec la vitesse. Avec une
    accélération douce (200 mm/s²), un patinage de cette taille est improbable ;
  - hypothèse : l'encodeur gauche perd des ticks à grande vitesse. L'interruption lit B juste après
    un front de A pour connaître le sens ; si B change peu après A et que l'interruption est un peu
    en retard (Wi-Fi), le tick est compté à l'envers (−1 au lieu de +1). La régulation croit alors la
    roue gauche trop lente et la pousse : d'où les 14 points de PWM en plus même roues en l'air
    (T018), et le PAMI tourne à droite sans que l'odométrie le voie ;
  - depuis le nouveau brochage, les deux réglages `ENCODER_*_INVERTED` ont dû être inversés (T014) :
    les voies A et B sont échangées par rapport à samedi. L'interruption est donc sur l'autre voie.
- **Essai décidé (T021)** : échanger A et B dans `config.h` pour revenir au décodage de samedi.
- **Nuance** : samedi, toutes les lignes étaient à 150 mm/s, et 350 mm/s n'avait jamais été essayé. Un
  défaut qui grandit avec la vitesse pouvait donc déjà exister sans se voir, quel que soit le câblage.
  L'écart de 1,2 % (mêmes moteurs, mêmes encodeurs) était sans doute déjà là, et peut expliquer une
  partie des écarts des carrés (T009 à T011). L'échange de A et B (T021) reste une hypothèse à vérifier.

### T021 — Voies A et B échangées, vitesse constante sur cales + vidéo des roues (banc test_pid)
- **Date** : 25/11/2026
- **Testeurs** : l'équipe (essai et vidéo), analyse de la vidéo image par image
- **Montage** : A et B des deux encodeurs échangés dans `config.h` (gauche A = 32, droite A = 18).
  PAMI sur une boîte, roues en l'air, repère blanc sur chaque pneu, filmé de l'arrière à 30 images/s
  (`pami/pami-vid.mp4`). Essai `c` (vitesse constante) à 350 mm/s, les deux roues, 8 s.
- **Mesures du banc** : vitesses mesurées par les encodeurs égales (gauche 353, droite 350 mm/s), mais
  PWM moyenne **gauche 63 %**, **droite 87 %**. L'écart a changé de côté (avant l'échange : gauche 79 %,
  droite 64 %, T018).
- **Mesures sur la vidéo** (passages du repère blanc, mouvement des images 140 à 380 = 8,0 s) :

  | Roue | Images par tour | Tours/s | Vitesse réelle | Vitesse annoncée par l'encodeur | PWM |
  |---|---|---|---|---|---|
  | Gauche | 17,4 | 1,72 | environ 320 mm/s | 353 mm/s | 63 % |
  | Droite | 13,1 | 2,28 | environ 425 mm/s | 350 mm/s | 87 % |

  Les vitesses réelles collent à ce que prévoit l'anticipation pour ces PWM (317 et 434 mm/s).
- **Résultat** : **à vitesses égales selon les encodeurs, la roue droite tourne en réalité 32 % plus vite
  que la gauche.** Les encodeurs ne mesurent pas la vraie vitesse quand les moteurs tournent vite :
  l'un compte trop (gauche, environ +10 %), l'autre pas assez (droite, environ −18 %). Avant
  l'échange, c'était l'inverse. À la main, moteurs arrêtés (T019), le comptage était presque juste.
- **Analyse** : aucun choix de voie ne donne un comptage juste. Cause probable : des parasites sur les
  fils des encodeurs quand les moteurs sont alimentés (fils des moteurs proches, pull-up internes
  faibles), et un décodage par interruption sensible aux parasites très courts et au retard des
  interruptions. C'est la cause de la dérive vers la droite et de sa forte croissance avec la vitesse.
- **Suite proposée** : compteur matériel de l'ESP32 (PCNT) avec filtre anti-parasites, et réduction
  des parasites côté câblage.

### T022 — Validation du comptage par PCNT (D022)
- **Date** : 25/11/2026
- **Testeurs** : l'équipe (essais et vidéo `vid2.mp4`), analyse de la vidéo image par image
- **Programme** : encodeurs comptés par le PCNT avec filtre de 5 µs, voies A / B et sens de T014.
- **1. Dix tours à la main** (`test_odometrie`) : gauche 0 → 19 477 (**1947,7 ticks par tour**), droite
  2 → 19 387 (**1938,5 par tour**). Les deux compteurs montent en marche avant.
- **2. Vitesse constante 350 mm/s, roues en l'air, filmées** (`test_pid`, `c`) : trois passages
  mesurés. Dernier passage, le plus net : gauche 15,6 images par tour, droite 15,4 (environ 1,94 tour/s,
  360 mm/s). Les autres : 15,8 / 15,5 et 15,3 / 14,5 (repère moins net). **Les deux roues tournent à la
  même vitesse**, à 1 ou 2 % près, contre 32 % d'écart avant le PCNT (T021).
- **3. Ligne droite au sol** (`test_trajectoire`) : écart réel sur le côté d'environ **25 mm vers la
  gauche, à 100 comme à 350 mm/s**. Avant : 53 mm à 100 mm/s et 340 mm à 350 mm/s, vers la droite (T020).
- **Résultat** : réussi. Le défaut qui grandissait avec la vitesse a disparu. Il reste une petite dérive
  **fixe**, la même à toutes les vitesses, dans le sens attendu : la roue gauche compte environ 0,5 %
  de ticks de plus par tour que la droite, donc à ticks égaux elle avance moins et le PAMI tourne à gauche.
- **Suite** : correction par roue (rapport des ticks par tour), puis anticipation (`test_pid`, escalier
  au sol) et étalonnage du diamètre moyen (`test_trajectoire`) à refaire avec le nouveau comptage.

### T023 — Lignes de 800 mm à 350 mm/s avec le rapport droite / gauche 1,0079 (banc test_trajectoire)
- **Date** : 26/11/2026
- **Testeurs** : l'équipe
- **Programme** : `test_trajectoire`, PCNT (D022), `WHEEL_DIAMETER_RIGHT_TO_LEFT_RATIO = 1.0079` (D023),
  `MOTION_ACCEL_MM_S2 = 200`. `v 350`, `l 800`, 4 fois.
- **Mesures** :
  - écart réel sur le côté : **25, 25 et 24 mm à gauche** (une ligne sans mesure). Le banc conseille 1,0159 ;
  - distances tapées `m 78`, `m 80`, `m 79` : des **centimètres** au lieu de millimètres, donc
    inutilisables (le banc en déduisait un diamètre de 5,8 mm). Garde-fou ajouté au banc depuis ;
  - les deux roues demandent la même PWM (79-81 % en croisière), le cap calculé reste entre −0,3° et
    +0,5° : la régulation se comporte pareil des deux côtés.
- **Analyse** : la correction de 0,79 % devait faire disparaître environ 25 mm de dérive sur 800 mm.
  L'écart n'a **pas changé** (25 mm avant, T022 ; 25 mm après). Il ne vient donc probablement pas
  d'un écart de diamètre entre les roues, mais d'une cause qui ne dépend ni de la vitesse ni des roues :
  la plus probable est l'**alignement du PAMI au départ** (1,8° de travers donne 25 mm sur 800 mm).
- **À faire** : ne pas reporter 1,0159. Départ guidé contre une règle posée au sol, puis `l 400` et `l 800`.
  Un PAMI mal aligné donne un écart 2 fois plus grand à 800 qu'à 400 mm ; un écart de roues, 4 fois.

### T024 — Firmware de match, départ `go d`, mission ligne droite (moniteur Wi-Fi)
- **Date** : 26/11/2026
- **Testeurs** : l'équipe
- **Programme** : firmware `esp32dev_wifi`, `ACTIVE_MISSION = STRAIGHT_LINE` (250 mm), profil labo 5 s / 14,5 s,
  PCNT (D022), rapport droite / gauche 1,0079 (D023), affichage d'état seulement après le départ.
- **Résultat** : conforme à ce qui était programmé.
  - moniteur calme avant le départ ; `go d` -> `[COTE] DROITE`, `[TIME] DEPART !`, puis `WAIT_DELAY` ;
  - mouvement autorisé à t = 5,0 s ; ligne droite faite : x = 248 mm, y = 0 mm, cap −0,1° ;
    `[MISS] Mission terminee` ; ticks G 2572 / D 2552 (rapport 1,0078 : la correction par roue s'applique) ;
  - `[TIME] FIN (14.5 s)`, état `END`, servo coupé ; environ 19 000 tours de boucle par seconde ;
  - les deux ultrasons mesurent (distances qui changent quand on passe devant).
- **Remarque** : l'équipe pensait que le PAMI s'arrêtait trop tôt : c'était la fin normale de la mission
  de test (250 mm). Le temps (5 s, 14,5 s) est lu dans le journal, pas encore vérifié au chronomètre.

### T025 — Dérive et distance en ligne droite sur la surface de compétition (banc test_trajectoire)
- **Date** : 26/11/2026
- **Testeurs** : l'équipe
- **Programme** : `test_trajectoire`, PCNT (D022), `WHEEL_DIAMETER_RIGHT_TO_LEFT_RATIO = 1.0079` (D023),
  `MOTION_ACCEL_MM_S2 = 500`. Vitesse : celle du banc, 350 mm/s par défaut (à confirmer). Départ posé à la main.
- **Mesures** (écart sur le côté vers la **droite**, d'après l'équipe ; distance réelle mesurée) :

  | Distance crue | Écarts sur le côté | Moyenne | Distances réelles |
  |---|---|---|---|
  | 1000 mm | 61, 31, 51 mm | 48 mm | 1000, 1000, 1000 mm |
  | 500 mm | 22, 18, 9 mm | 16 mm | 500, 510, 510 mm |

- **Analyse** :
  - **distance** : 1000 mm crus = 1000 mm réels. `WHEEL_DIAMETER_MM = 59.4` reste bon sur cette surface
    (les 7 mm de plus à 500 mm sont de l'ordre de la précision de la mesure) ;
  - **dérive** : un écart 2,9 fois plus grand à 1000 qu'à 500 mm, entre « angle de départ » (×2) et
    « virage régulier » (×4). Si on la sépare en deux parties : environ 1,0° d'angle de départ et un
    virage de 30 mm sur 1000 mm ;
  - le rapport 1,0079, réglé sur le sol du labo, fait à lui seul tourner le PAMI d'environ 39 mm vers
    la droite sur 1000 mm. C'est l'essentiel de la dérive mesurée ici ;
  - écart prévu au bout des 1900 mm de la ligne de la douve selon le rapport :

    | Rapport | Si les deux parties (calcul ci-dessus) | Si seulement un virage | Si seulement un angle de départ |
    |---|---|---|---|
    | 1,0079 (avant) | 142 mm à droite | 176 mm à droite | 85 mm à droite |
    | **1,0000 (retenu)** | **2 mm** | 36 mm à droite | 56 mm à gauche |
    | 0,9981 (calculé sur 1000 mm seulement) | 32 mm à gauche | 2 mm | 89 mm à gauche |

    1,0000 donne le plus petit écart dans le pire des cas. La marge dans la douve est de ±100 mm
    (`docs/douves-coordonnees.md`) ;
  - l'écart entre deux essais identiques (31 à 61 mm sur 1000 mm) est plus grand que ce qui reste à
    corriger. Il vient sans doute de la pose à la main : 1° de travers donne 33 mm sur 1900 mm.
- **Décision** : `WHEEL_DIAMETER_RIGHT_TO_LEFT_RATIO = 1.0000` (D023).
- **Conséquence** : sur le sol du labo, le PAMI devrait maintenant partir à gauche (environ 50 mm sur
  800 mm, d'après T023). C'est attendu : on règle pour la surface du match.
- **À faire** :
  - vérifier sur la surface de compétition : 3 lignes de 1000 mm, puis la vraie ligne de la douve
    (`l 1900` ou le banc `test_parcours`, commande `d`), écart mesuré au bout ;
  - un gabarit de pose dans l'écurie, pour réduire l'écart d'un essai à l'autre.

### T026 — Vérification du rapport 1,0000 sur la surface de compétition (banc test_trajectoire)
- **Date** : 26/11/2026
- **Testeurs** : l'équipe
- **Programme** : `test_trajectoire`, `WHEEL_DIAMETER_RIGHT_TO_LEFT_RATIO = 1.0000` (T025), mêmes réglages
  que T025. Départ posé à la main.
- **Mesures** (écart sur le côté avec signe, tel que noté par l'équipe ; distance réelle) :

  | Distance crue | Écarts sur le côté | Moyenne | Écart-type | Distances réelles |
  |---|---|---|---|---|
  | 1000 mm | +10, +7, −5 mm | +4 mm | 8 mm | 1000, 1000, 1000 mm |
  | 500 mm | +4, +1, −5 mm | 0 mm | 5 mm | 500, 500, 500 mm |

- **Résultat** : **réussi**. La dérive régulière a disparu : 48 mm (T025), puis 4 mm sur 1000 mm.
  - Distances justes à 500 comme à 1000 mm : le diamètre moyen (59,4 mm) est bon sur cette surface.
  - Les 4 mm de moyenne sont plus petits que l'incertitude sur cette moyenne (±5 mm avec 3 essais).
    Ils correspondent à 0,08 % d'écart entre les roues, sous le seuil de 0,2 % du banc : rien à corriger.
    Même si c'était un vrai défaut, il donnerait environ 14 mm sur la ligne de la douve (1900 mm).
  - Il reste un écart **d'un essai à l'autre** (−5 à +10 mm sur 1000 mm). Il ne vient pas d'un réglage :
    il change de sens d'un essai à l'autre, et aucune constante du code ne peut l'annuler.
- **Décision** : on garde 1,0000. Le changer encore reviendrait à corriger du hasard, ce qui augmente
  l'écart d'un essai à l'autre au lieu de le réduire.
- **Pour aller plus loin** : réduire l'écart d'un essai à l'autre (pose au départ, appui avant), puis
  vérifier sur la vraie ligne de la douve (`l 1900`, 3 à 5 fois, avec `e`).

### T027 — Firmware de match, mission douve, côté DROITE : pas de virage vers la douve (moniteur Wi-Fi)
- **Date** : 26/11/2026
- **Testeurs** : l'équipe
- **Programme** : firmware `esp32dev_wifi`, `ACTIVE_MISSION = MOAT`, 350 mm/s, accélération 500 mm/s²,
  rapport droite / gauche 1,0000 (T025), profil labo 5 s / 14,5 s. Départ `go d`, 2 essais.
- **Mesures** (les 2 essais sont identiques) :
  - `[MISS] Debut : 3 etapes, depart a DROITE` : la bonne mission est chargée ;
  - première étape réussie, finie entre 10 et 11 s : `x = 1898 mm, y = 1 mm, cap = -0.1 deg`
    (2e essai : y = 2 mm, cap −0,2°) ;
  - ensuite, les ticks ne bougent presque plus (G 19 637 → 19 639, D 19 634 → 19 636) jusqu'à
    `[TIME] FIN (14.5 s)` : pas de rotation (environ 50° attendus), pas de `[SAFE]`, pas de `[MOT] PANNE`,
    état `MISSION` jusqu'à la fin.
- **Analyse** (cause trouvée dans le code) :
  - à la fin de la ligne droite, le PAMI glisse encore d'environ 10 mm (1898 mm annoncés, 19 637 ticks
    = 1909 mm à la fin), mais la rotation démarre aussitôt, pendant ce glissement ;
  - la rotation surveille que le PAMI reste sur place (somme des distances des roues). Les 10 mm de
    glissement lui font demander aux deux roues de **reculer** d'environ 50 mm/s, plus que sa vitesse
    de départ (40 mm/s) : les deux roues sont commandées en arrière ;
  - `safety` refuse toute marche arrière qui n'est pas une rotation sur place : il freine. `motion`
    repart alors de zéro, redemande la même chose, se fait refuser… sans fin. Le PAMI reste bloqué
    jusqu'à la fin du temps, sans message ;
  - ce blocage arrive dès que le glissement dépasse environ 4 mm. À 150 mm/s (carrés T009 à T011), le
    glissement était plus petit : c'est pour ça que les rotations marchaient jusqu'ici.
- **Suite** : attendre que le PAMI soit vraiment arrêté avant de lancer le mouvement suivant (D024).

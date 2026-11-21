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

# Régler et valider la régulation du PAMI

> **Pour qui** : toute l'équipe, et surtout la personne chargée des essais.
> **Bancs utilisés** : `test_pid` (la vitesse de chaque roue) et `test_trajectoire` (ce que le PAMI fait
> vraiment). Tous deux marchent par USB ou sans câble (`test_pid_wifi`, `test_trajectoire_wifi`,
> voir `docs/guide-pratique-pami.md`, « Sans câble »).
> **Règle d'or des réglages** : du bas vers le haut, un seul réglage à la fois, 3 mesures à chaque fois.

## 1. Ce qu'on régule : la chaîne de commande

```
mission                « avance de 500 mm », « tourne de 90° »
   │
   ▼
profil en trapèze      vitesse voulue à chaque instant : accélère, roule, freine
   │                   (MOTION_ACCEL_MM_S2, DRIVE_SPEED_MM_S, TURN_SPEED_MM_S)
   ▼
correction de cap      la roue en avance ralentit, l'autre accélère (HEADING_KP_MM_S_PER_MM)
   │
   ▼
consigne de chaque roue, en mm/s
   │
   ▼
régulateur de vitesse  PWM = anticipation + P + I, une par roue, toutes les 20 ms
   │                   (LEFT_/RIGHT_MOTOR_DEADBAND_PCT, ..._PCT_PER_MM_S, SPEED_KP/KI)
   ▼
safety → driver → moteur → roue
                             │
encodeur ◄───────────────────┘   → vitesse mesurée (le retour du régulateur)
                                 → odométrie : distance, angle, position (x, y, cap)
```

Il y a deux étages, plus un réglage qui n'est pas de la régulation :
- **la boucle de vitesse** de chaque roue (le régulateur) : on la teste avec `test_pid` ;
- **la trajectoire** (distance, angle, cap) : on la teste avec `test_trajectoire` ;
- **l'étalonnage** (`WHEEL_DIAMETER_MM`, `WHEEL_BASE_MM`) : il transforme les ticks en millimètres et
  en degrés. S'il est faux, la régulation fait parfaitement… ce qu'on lui demande à partir de mesures
  fausses. **C'est la cause la plus fréquente des « mauvais contrôles ».**

## 2. PI ou PID ?

Avant la fusion du 12/11/2026, on avait deux versions : celle de la branche `PID` et celle de
`pami-firmware-v1`. Elles ont **le même régulateur** : **anticipation + P + I**. Aucune n'a de terme D.
Leurs seules différences :

| Différence | Effet aujourd'hui |
|---|---|
| Anticipation propre à chaque roue (`LEFT_…`, `RIGHT_…`) | Aucun : elle vaut encore la valeur commune. Elle servira une fois mesurée (escalier, § 3.1). |
| Anti-emballement : l'intégrale ne grossit plus quand la PWM est à 100 % | Aucun : voir le calcul ci-dessous |
| Limite de PWM du régulateur = `MOTOR_MAX_PWM_PCT` | Aucun : 100 % dans les deux cas |

**Le calcul** : même roue bloquée, la PWM vaut au plus
anticipation (10 + 0,2 × v) + P (0,05 × v) + I (30 max) = 40 + 0,25 × v.
Elle n'atteint 100 % qu'à partir de **v = 240 mm/s**. Or on roule à 150 mm/s (90 en rotation).

**Conclusion** : aujourd'hui, les deux se comportent exactement pareil. Les comparer sur le PAMI
donnerait les mêmes bilans. On a gardé la version de la branche `PID` (c'est celle de `main`) : c'est
la même, plus deux améliorations qui serviront plus tard (anticipation mesurée par roue, vitesses plus
hautes, batterie).

**Pourquoi pas de terme D** :
- le D réagit à la *variation* de l'erreur. Or notre mesure de vitesse est bruitée : 1 tick (0,1 mm)
  en 20 ms = 5 mm/s d'écart d'une mesure à l'autre. Le D amplifierait ce bruit, et les moteurs vibreraient ;
- l'anticipation donne déjà la réactivité que le D apporte d'habitude ;
- on ne le reconsidérerait que si une oscillation résistait à une baisse de KI, puis de KP.

## 3. Les deux bancs : qui répond à quoi

| Question | Banc |
|---|---|
| Chaque roue va-t-elle à la vitesse demandée, vite et sans osciller ? | `test_pid` |
| Le PAMI parcourt-il vraiment 1 m, tourne-t-il vraiment de 360°, referme-t-il son carré ? | `test_trajectoire` |

### 3.1 `test_pid` : échelon, escalier, vitesse constante, courbes

Par USB : `pio run -e test_pid -t upload -t monitor`. Sans câble : `pio run -e test_pid_wifi -t upload`,
puis `pio device monitor -e test_pid_wifi`. Les touches g / d / a **choisissent** les roues
(rien ne tourne) ; e, s et c **lancent** un essai.

**L'échelon (`e`) : « la roue atteint-elle sa vitesse vite, sans la dépasser, et la tient-elle ? »**
- La consigne passe d'un coup de 0 à 150 mm/s pendant 3 s : le cas le plus dur pour le régulateur.
  Comparaison : le 0 à 100 km/h d'une voiture.
- Le bilan donne : le temps pour atteindre 90 % de la vitesse, le dépassement, l'erreur moyenne une fois
  stabilisé, l'ondulation (min et max), et quelle part de la PWM vient de la correction P + I.
- **Sa plus-value** :
  - des chiffres pour comparer deux réglages de KP / KI, au lieu d'une impression ;
  - il teste le régulateur **seul**, sans odométrie ni cap. Si une ligne droite est mauvaise mais que
    l'échelon est bon, le régulateur n'est pas en cause : chercher du côté de l'étalonnage.

**L'escalier (`s`, au sol) : « quelle PWM faut-il à chaque roue pour chaque vitesse ? »**
- 4 paliers de 1,5 s : 50, 100, 150 et 200 mm/s. Le banc relève la PWM qu'il a fallu à chaque palier,
  et trace la droite « PWM = zone morte + pente × vitesse » de chaque roue.
- Il affiche les valeurs à recopier : `LEFT_MOTOR_DEADBAND_PCT`, `LEFT_MOTOR_PCT_PER_MM_S`, et de même
  pour la droite.
- **Sa plus-value** : c'est le réglage le plus rentable.
  - L'anticipation fait l'essentiel du travail : elle donne tout de suite la bonne PWM, sans attendre
    qu'une erreur apparaisse. P et I n'ont plus qu'à corriger de petits écarts.
  - Résultat : la roue est à la bonne vitesse plus vite, plus régulièrement, et **les deux roues se
    comportent pareil**, donc les lignes sont plus droites et la correction de cap travaille moins.
  - Sans elle, on réglerait KP et KI pour compenser un modèle faux : c'est l'erreur classique.
  - Il révèle aussi un moteur plus faible que l'autre (pente différente).
- **Toujours au sol** : sur cales, la roue n'a pas de charge et les valeurs sont fausses.

**La vitesse constante (`c`, sur cales) : « la roue tient-elle sa vitesse quand on la freine ? »**
- 8 s à vitesse fixe. Freinez **légèrement** la roue au doigt : la correction monte et la vitesse revient
  vers la consigne. Au relâchement : petit dépassement, puis retour, sans oscillation.
- **Sa plus-value** : c'est ce qui arrivera sur un tapis, une bosse, ou quand la batterie baisse.
  C'est le travail de l'intégrale (I).

**Les courbes (touche `v`) : « à quoi ressemble la réponse ? »**
- Une ligne toutes les 20 ms : `CSV;t_ms;cibleG;mesureG;pwmG;cibleD;mesureD;pwmD`.
- Pour tracer : copiez les lignes qui commencent par `CSV;` dans Excel, *Données → Convertir*,
  séparateur `;`, puis un graphique « nuage de points avec lignes », `t_ms` en abscisse.
- **Sa plus-value** : le bilan donne des chiffres ; la courbe montre la **forme** de la réponse.
  - un retard, une oscillation, un dépassement se voient au premier coup d'œil ;
  - le moment où la PWM plafonne se voit aussi ;
  - on compare deux réglages sur le même graphique ;
  - à l'oral (règlement général D.2 : expliquer notre robot), une courbe consigne / mesure explique
    la régulation mieux que des mots.
- Les petites dents de scie de ±5 mm/s sont normales : c'est la précision de la mesure (1 tick).

### 3.2 `test_trajectoire` : ce que le PAMI fait vraiment

Par USB : `pio run -e test_trajectoire -t upload -t monitor`. Sans câble :
`pio run -e test_trajectoire_wifi -t upload`, puis `pio device monitor -e test_trajectoire_wifi`.
On tape une commande, puis Entrée (chiffres : pavé numérique, ou Maj sur un clavier AZERTY) :

| Commande | Effet |
|---|---|
| `l 1000` | ligne droite de 1000 mm (50 à 2000) |
| `r 360`, `r -90` | rotation sur place ; positif = vers la gauche |
| `c 300`, `c -300` | carré de 300 mm ; positif = en tournant à gauche |
| `v 100` | vitesse des lignes droites (40 à 150 mm/s) |
| `m 1003` | **mesure réelle** du dernier essai : mm, ou degrés pour une rotation |
| Entrée seule | refaire le même essai |
| `n`, `z`, `i`, `h` | nouvelle série de mesures, position à zéro, réglages, aide |

À la fin de chaque essai, le banc affiche ce que le PAMI **croit** avoir fait, et la PWM la plus forte
qu'il a demandée (la **marge** avant 100 %). Vous mesurez le **réel**, vous le tapez avec `m`, et le banc
compare :

| Comparaison | Ce que ça veut dire | Où chercher |
|---|---|---|
| Cru ≈ commandé, réel ≈ cru | Tout va bien | — |
| Cru ≈ commandé, réel ≠ cru | La régulation a bien travaillé, mais le PAMI **mesure mal** | Étalonnage : le banc calcule la valeur |
| Cru ≠ commandé | La régulation n'a pas fait son travail | `test_pid`, fin de mouvement |

Les mesures s'accumulent en **série** : moyenne, dispersion, et la valeur conseillée de
`WHEEL_DIAMETER_MM` (lignes) ou de `WHEEL_BASE_MM` (rotations).

Sécurité : pendant les lignes droites, le PAMI s'arrête à 150 mm d'un obstacle vu par l'ultrason. Ce
n'est pas l'évitement du match : gardez la main sur l'arrêt d'urgence.

## 4. La campagne de réglage, dans l'ordre

On ne règle pas un étage tant que celui du dessous n'est pas bon. Après chaque changement dans
`config.h` : retéléverser, puis 3 mesures.

| Étape | Banc et commandes | Critère de réussite | Ce qu'on règle |
|---|---|---|---|
| 0. Câblage, encodeurs | PDF de câblage, page 7 ; roue à la main | 1 tour de roue ≈ 1920 ticks, le bon compteur monte | le câblage |
| 1. Anticipation | `test_pid`, au sol : `a` puis `s`, 3 fois | valeurs stables à ±10 % | `LEFT_/RIGHT_MOTOR_DEADBAND_PCT`, `..._PCT_PER_MM_S` |
| 2. Régulateur | `test_pid`, au sol : `a` puis `e` ; sur cales : `c` | montée ≤ 300 ms, dépassement ≤ 15 %, erreur ≤ 5 mm/s, correction P+I < 5 % | KP, KI (§ 7), seulement si besoin |
| 3. Diamètre | `test_trajectoire` : `l 1000`, mesurer, `m …`, 3 à 5 fois | distance à 1 % près ; écart latéral réel < 10 mm sur 500 mm | `WHEEL_DIAMETER_MM` |
| 4. Entraxe (après le diamètre : l'angle en dépend) | `r 360`, puis `r -360`, 3 à 5 fois chacun | 5° près par tour | `WHEEL_BASE_MM` |
| 5. Carrés | `c 300` et `c -300`, 5 fois chacun | retour à moins de 30 mm, dans les deux sens | § 8 |
| 6. Marges | lire « PWM max » à la fin des essais ; refaire 2 et 3 batterie presque vide | PWM max ≤ 70 % en croisière | vitesse, batterie |
| 7. Match | firmware : mission complète, arrêt devant un obstacle | mission en moins de 9,5 s ; arrêt avant le contact | vitesses, seuil d'arrêt |

## 5. Bien mesurer

- **Distance** : 1 m plutôt que 40 cm (une erreur de 5 mm vaut 0,5 % sur 1 m, 1,25 % sur 40 cm).
  Un repère précis sur le PAMI (par exemple l'axe des roues) et un trait de scotch au sol.
- **Angle** : un tour (360°) plutôt que 90°, l'erreur est 4 fois plus visible. Repère : une ligne de
  scotch au sol alignée avec le flanc du PAMI.
  Astuce précise : un pointeur laser scotché sur le PAMI **pendant le test seulement**, et le point sur
  un mur à 2 m : **1° ≈ 35 mm sur le mur**.
- **Trajectoire** : un feutre fixé au centre du PAMI, sur une grande feuille de papier kraft : on voit
  une courbe, un zigzag ou une dérive.
- **Vidéo au ralenti**, filmée d'au-dessus avec une règle au sol : patinage, à-coups, oscillations.
- **Répéter 3 à 5 fois** :
  - une erreur qui revient toujours pareille (**systématique**) se corrige par un réglage ;
  - des résultats qui varient d'un essai à l'autre (**dispersion**) ne se corrigent pas par un réglage :
    patinage, mécanique (roue qui frotte, jeu), batterie. Le banc prévient au-delà de 1 %.

## 6. Corriger : symptôme → cause → réglage

| Symptôme | Cause probable | Correction (`config.h`) |
|---|---|---|
| Distance réelle ≠ distance crue | Diamètre de roue faux | `WHEEL_DIAMETER_MM` : valeur donnée par `test_trajectoire` (= actuel × réel / cru) |
| Angle réel ≠ angle cru | Entraxe faux | `WHEEL_BASE_MM` : valeur donnée par `test_trajectoire` (= actuel × cru / réel) |
| Ligne qui courbe alors que le cap cru reste à 0 | Roues de diamètres différents : l'odométrie ne le voit pas | Mécanique d'abord (pneus, fixation, poids), puis § 8 |
| Zigzag en ligne droite | Correction de cap trop forte, ou roues qui suivent mal leur vitesse | Baisser `HEADING_KP_MM_S_PER_MM` ; sinon `test_pid` |
| Le cap dérive lentement pendant la ligne | Correction de cap trop faible | Monter `HEADING_KP_MM_S_PER_MM` |
| Résultats dispersés, roue qui patine au démarrage | Accélération trop forte | Baisser `MOTION_ACCEL_MM_S2` ou la vitesse ; vérifier l'adhérence |
| PWM max proche de 100 % | Plus de réserve pour corriger | Baisser la vitesse, recharger la batterie, chercher un frottement |
| Une roue n'atteint pas sa vitesse, ou oscille (`test_pid`) | Anticipation mal mesurée, puis gains | Escalier d'abord, puis § 7 |
| Arrêt avant ou après la cible **crue** | Fin de mouvement | `DRIVE_TOLERANCE_MM`, `MOTION_MIN_SPEED_MM_S` |
| `PANNE : roue bloquee` sans raison | Seuils de blocage trop serrés, ou encodeur | Vérifier les encodeurs (page 7) avant `STALL_*` |

## 7. Régler KP et KI (seulement si l'échelon échoue)

Ce que veulent dire les valeurs actuelles :
- `SPEED_KP_PCT_PER_MM_S = 0.05` : 100 mm/s d'écart → 5 % de PWM en plus. Le bruit de mesure (±5 mm/s)
  ne donne que ±0,25 % : **on a de la marge pour monter KP**.
- `SPEED_KI_PCT_PER_MM = 0.3` : 10 mm/s d'écart pendant 1 s → 3 % de PWM en plus.
- `SPEED_INTEGRAL_MAX_PCT = 30` : l'intégrale ne dépasse jamais 30 %.

| Ce que montre l'échelon | Réglage |
|---|---|
| Montée trop lente | D'abord l'anticipation (escalier), puis monter KP |
| Dépassement, ou oscillation qui s'amortit lentement | Baisser KI, puis KP |
| Erreur qui reste après la montée | Monter KI |
| Ondulation forte, roue qui « grogne » | Baisser KP |

**Trouver la marge** : augmenter KP par paliers (× 1,5) jusqu'à ce que l'ondulation du bilan grandisse
nettement, puis retenir environ la moitié de cette valeur. Même idée pour KI, avec le dépassement.
Un seul gain à la fois ; comparer les bilans, et si possible les courbes (`v`).

## 8. Carrés dans les deux sens (méthode UMBmark)

Une ligne droite ne suffit pas à séparer deux erreurs qui se ressemblent : roues de diamètres
différents, et entraxe faux. Le carré dans les deux sens les sépare :
1. `c 300`, 5 fois, en notant **de quel côté** du repère de départ le PAMI s'arrête ;
2. `c -300`, 5 fois, idem.

Lecture :
- les deux sens finissent **du même côté** du départ : les deux roues n'ont pas le même diamètre ;
- ils finissent **de côtés opposés** : l'entraxe est faux (refaire l'étape 4).

Le code n'a pas encore de correction gauche/droite du diamètre : on l'ajoutera **si les mesures le
demandent** (écart latéral > 10 mm sur 500 mm, ou carrés > 30 mm alors que diamètre et entraxe sont
bons). Apportez les mesures des 10 carrés : on fera le calcul ensemble.

## 9. S'assurer que la régulation n'est pas limitante

**Partir des besoins du match** :
- la fin de l'approche est guidée par les capteurs de sol (D009) : l'odométrie doit seulement amener
  le PAMI près de la zone ;
- le mouvement dure au plus 9,5 s (profil labo) : la mission doit tenir dedans ;
- à la vitesse de croisière, la distance de freinage doit rester bien plus courte que le seuil d'arrêt
  de l'évitement (150 mm, règle d'or n° 1).

**Les limites habituelles d'un petit robot à roues**, de la plus fréquente à la plus rare :
1. l'étalonnage de l'odométrie (diamètre, écart gauche/droite, entraxe) ;
2. le patinage (accélération, adhérence) ;
3. une anticipation mal mesurée ;
4. la mesure de vitesse à basse vitesse (± 5 mm/s pèse lourd à 50 mm/s) ;
5. la tension de la batterie ;
6. seulement ensuite, les gains KP et KI.

**Les marges à vérifier, chiffrées** :
- PWM max ≤ 70 % en croisière (affichée par `test_trajectoire`) : il reste de quoi corriger ;
- KP retenu ≈ la moitié de celui qui fait osciller ;
- dispersion des essais répétés < 1 % ;
- les critères tiennent aussi batterie presque vide.

**Les évolutions possibles, seulement si une mesure les justifie** :
- correction du diamètre gauche/droite (§ 8) ;
- régulation toutes les 10 ms au lieu de 20 (si l'échelon est trop lent malgré tout) ;
- gyroscope pour le cap (D004, optionnel) ;
- terme D : pas prévu (§ 2).

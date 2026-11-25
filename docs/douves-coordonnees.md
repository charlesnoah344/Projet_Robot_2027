# Eurobot 2027 — Coordonnées des douves et des écuries

> Document fourni par l'équipe le 24/11/2026, recopié tel quel. Le code l'utilise via `pami/src/config.h`
> (D017) avec des noms anglais : `MOAT_WAYPOINT_MM`, `MOAT_NEAR_MM`, `MOAT_CENTER_MM`, `MOAT_FAR_MM`.
> Le signe de y selon l'interrupteur (§ 6) est appliqué par `forSide()` (`logic/mission_step.h`).

> Relevé sur le plan coté de l'annexe G.1 du règlement **version officielle 1.0** (figure 8), à partir des tracés vectoriels du PDF, précision ±5 mm.
> Les douves, écuries, salles du trône et remparts sont identiques à ceux de la bêta 0.4. Seule différence : la carrière de bord la plus proche du château bleu a été déplacée, et la table est désormais parfaitement symétrique.

## 1. Repère officiel de la table

- Table : 3000 × 2000 mm.
- Origine **(0, 0)** : coin avant gauche vu du public, côté jaune.
- **x** : le long des 3000 mm, vers le côté bleu.
- **y** : le long des 2000 mm, vers le fond de la table (y = 0 côté public).

```
 y=2000  +--------------------------------------------------+  fond
         | [Écurie J]                          [Écurie B]   |
         |   Trône                                Trône      |
         |   jaune          (table)               bleu       |
         | [Écurie J]                          [Écurie B]   |
 y=0     +--------------------------------------------------+  public
        x=0                                              x=3000
```

Une douve est validée si une partie de la projection du PAMI est dans la zone. La ligne qui délimite la zone en fait partie.

## 2. Douves (repère officiel, mm)

Les douves diagonales sont des rectangles de 400 × 200 mm tournés à 45°. Viser le centre : la marge est de ±100 mm en largeur.

### Château bleu (cibles des PAMI jaunes)

| Douve           | Centre       | Coins                                                  |
|-----------------|--------------|--------------------------------------------------------|
| Diagonale avant | (2179, 429)  | (1967, 500) (2109, 641) (2391, 359) (2250, 217)         |
| Centrale        | (1950, 1000) | (1850, 800) (2050, 800) (2050, 1200) (1850, 1200)       |
| Diagonale fond  | (2179, 1571) | (1967, 1500) (2109, 1359) (2391, 1641) (2250, 1783)     |

### Château jaune (cibles des PAMI bleus)

| Douve           | Centre       | Coins                                                  |
|-----------------|--------------|--------------------------------------------------------|
| Diagonale avant | (821, 429)   | (1033, 500) (891, 641) (609, 359) (750, 217)            |
| Centrale        | (1050, 1000) | (950, 800) (1150, 800) (1150, 1200) (950, 1200)         |
| Diagonale fond  | (821, 1571)  | (1033, 1500) (891, 1359) (609, 1641) (750, 1783)        |

## 3. Écuries (repère officiel, mm)

| Écurie      | x           | y            | Côté du PAMI tourné vers l'adversaire |
|-------------|-------------|--------------|---------------------------------------|
| Jaune avant | 0 à 200     | 0 à 300      | droite                                |
| Jaune fond  | 0 à 200     | 1700 à 2000  | gauche                                |
| Bleue avant | 2800 à 3000 | 0 à 300      | gauche                                |
| Bleue fond  | 2800 à 3000 | 1700 à 2000  | droite                                |

## 4. Repère du PAMI (celui à utiliser dans le code)

La table est symétrique dans les deux sens : dans ce repère, **les valeurs sont les mêmes pour les quatre écuries**. Le switch gauche/droite indique seulement le côté vers lequel tourner ; la couleur n'est pas nécessaire.

- **x** : avance depuis notre petit bord (derrière l'écurie).
- **y** : distance depuis le grand bord du côté de l'écurie, vers le centre de la table.

| Point                  | x          | y          |
|------------------------|------------|------------|
| Écurie                 | 0 à 200    | 0 à 300    |
| Point de virage        | 2000       | 210        |
| Douve proche (diag.)   | 2179       | 429        |
| Douve centrale         | 1950       | 1000       |
| Douve éloignée (diag.) | 2179       | 1571       |

### Passage au repère officiel

| PAMI  | X officiel | Écurie avant | Écurie fond  |
|-------|------------|--------------|--------------|
| Jaune | X = x      | Y = y        | Y = 2000 − y |
| Bleu  | X = 3000 − x | Y = y      | Y = 2000 − y |

## 5. Trajet conseillé vers la douve proche

1. Sortir de l'écurie et rouler tout droit à **y ≈ 210 mm** jusqu'à **x ≈ 2000 mm**.
2. Tourner d'environ **50°** vers le centre de la table.
3. Avancer d'environ **280 mm** jusqu'au centre de la douve (2179, 429).

Distance totale ≈ 2,2 m en 15 s (de 85 s à 100 s) : au moins 0,15 m/s de moyenne, viser 0,25 à 0,3 m/s pour garder de la marge en cas d'évitement.

Pourquoi ce couloir à y ≈ 210 :

Obstacles, dans le repère du PAMI (mêmes valeurs pour les quatre écuries) :

| Obstacle                                  | x                                          | y               |
|-------------------------------------------|--------------------------------------------|-----------------|
| Carrières le long du bord (4)             | 590–910, 1090–1410, 1590–1910, 2090–2410   | 0 à 110         |
| Nos remparts près de l'écurie             | 55 à 445                                   | à partir de 305 |
| Carrière centrale (sur la ligne directe)  | 1445 à 1555                                | 340 à 660       |

Au départ du match, chaque carrière contient 3 pierres couchées et empilées (320 × 110 × 110 mm chacune). Le grand robot a pu les déplacer avant 85 s.

La ligne noire et blanche imprimée à y = 250 traverse la douve proche entre x = 2081 et x = 2314 : utile comme repère visuel pendant les tests.

## 6. Valeurs pour `config.h`

```cpp
// Repère PAMI : x = avance depuis notre bord, y = distance au bord côté écurie (mm)
// Source : plan annexe G.1 (figure 8), règlement officiel 1.0 — à confirmer sur la vraie table
constexpr float WAYPOINT_X_MM        = 2000.0f;
constexpr float WAYPOINT_Y_MM        =  210.0f;
constexpr float DOUVE_PROCHE_X_MM    = 2179.0f;
constexpr float DOUVE_PROCHE_Y_MM    =  429.0f;
constexpr float DOUVE_CENTRALE_X_MM  = 1950.0f;
constexpr float DOUVE_CENTRALE_Y_MM  = 1000.0f;
constexpr float DOUVE_ELOIGNEE_X_MM  = 2179.0f;
constexpr float DOUVE_ELOIGNEE_Y_MM  = 1571.0f;
// Switch DROITE (bord de table à droite du PAMI) : le centre de la table est à gauche,
//   donc y augmente vers la gauche du PAMI → virages vers la gauche.
// Switch GAUCHE : y augmente vers la droite du PAMI → virages vers la droite.
constexpr int SIGNE_Y_SWITCH_DROITE = +1;  // +1 = vers la gauche du PAMI
constexpr int SIGNE_Y_SWITCH_GAUCHE = -1;  // -1 = vers la droite du PAMI
```

## 7. Points d'attention

- **Une douve ne compte qu'une fois.** Trois PAMI dans la même écurie avec le même réglage iront tous à la même douve (5 points seulement). Prévoir un deuxième réglage pour qu'un PAMI vise la douve centrale ou éloignée.
- **Les PAMI adverses empruntent le même couloir en sens inverse**, au même moment : décider à l'avance de quel côté s'écarter lors de l'évitement.
- **Mesurer sur une vraie table.** Le règlement (D.1) prévient que les organisateurs construisent l'aire de jeu « avec la plus grande exactitude possible » sans la garantir : prévoir une marge et vérifier sur place.
- **Rappels du règlement 1.0** : écurie de 200 × 300 mm, PAMI contenus dans la zone pendant la préparation (la ligne colorée et le bord de table accolé en font partie), sortie avant 85 s = PAMI retiré, 5 points par douve occupée en fin de match.

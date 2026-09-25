# Règles de jeu Eurobot 2027 — résumé

> Source : « Règlement Eurobot et Eurobot Junior 2027 », **version BÊTA 0.4**
> (`docs/pdf/Eurobot2027_Rules_FR.pdf`).
> En version bêta, aucune réclamation fondée sur ce document n'est acceptée en rencontre.
> **À mettre à jour dès la sortie de la version officielle.**
> Pour une formulation exacte, toujours revenir au PDF. Les réponses de la FAQ officielle
> (www.eurobot.org/faq) données par un arbitre référent font foi.
> Dernière mise à jour du résumé : 25/09/2026.

## Thème et principe
Le roi Arthur, la quête du Graal et Camelot. Il y a 4 actions :
- elles sont indépendantes ;
- aucune n'est obligatoire ;
- l'ordre est libre.

Le règlement recommande de concevoir des systèmes simples et fiables sur un nombre limité d'actions.
Le match dure 100 s.

## Aire de jeu (D.2)
- Table de 3000 × 2000 mm, avec des bordures de 70 mm de haut et 22 mm d'épaisseur.
- Surface en vinyle antidérapant.
- La table peut être faite de plusieurs panneaux. Les jointures imparfaites ne sont pas contestables.
- Les mots gauche, droite, avant, arrière et fond s'entendent du point de vue du public.
- Les zones :
  1. salle du trône (départ et arrivée du robot) ;
  2. cour du château ;
  3. écuries (départ des PAMI) ;
  4. remparts ;
  5. douves ;
  6. carrières de pierre.
- Dimensions et positions exactes : plans en annexe du PDF.

## Zones de départ (D.3)
- Salle du trône : carré de 50 × 50 cm de la couleur de l'équipe, placé sur un côté de la table.
- La ligne colorée et le bord de table adjacent font partie de la zone.
- À la fin de la préparation, la projection verticale du robot doit être entièrement dans sa zone.

## Éléments de jeu (D.4)
- **Pierres** :
  - 30 boîtes en carton de 320 × 110 × 110 mm ;
  - un tag ArUco n° 13 sur chacune des 4 grandes faces ;
  - 3 pierres debout par carrière.
- **Remparts** : zones de construction réservées à chaque équipe, délimitées par une ligne incluse.
  - Zones de mur : rectangles de 150 × 400 mm.
  - Zones de tour : cercles de Ø 200 mm.
- **Cour du château** : entourée par les remparts, elle contient la salle du trône.
- **Douves** : zones autour des remparts, délimitées par une ligne incluse.
- **Chevaliers** : ce sont les PAMI de l'équipe.
  - Ils partent des écuries.
  - Un design et des couleurs harmonisés avec le robot sont souhaités.
- **Graal** : fabriqué par l'équipe, posé dans la salle du trône avant le match.
- **Boulets** : balles en mousse légère et compressible de Ø 45 mm, 10 par équipe.

## Règle transversale
Un élément encore contrôlé par un robot ou un actionneur à la fin du match n'est pas compté.

## E.1 — Construction de la glorieuse Camelot
But : déposer des pierres dans ses remparts en respectant le plan du château (murs, tours, porte).

**Placement d'une pierre**
- Une pierre est placée dans une zone si une partie de sa projection verticale y est.
- Elle compte aussi si elle repose sur une pierre déjà valide pour cette zone.

**Types de construction**
- **Mur** : 3 pierres couchées et empilées, dans une zone rectangulaire.
- **Tour** : 1 pierre debout, dans une zone circulaire.
- **Porte** : 2 pierres debout surmontées d'une pierre couchée, dans une zone rectangulaire.
  Une seule porte par château.

**Contraintes**
- Une seule construction du type prévu par zone. Bonus si la construction respecte le plan.
- Les éléments de jeu peuvent dépasser la hauteur limite, jusqu'à 430 mm.
  Seulement dans les zones de construction et dans les limites du château.
- On peut stocker jusqu'à 3 pierres dans la cour du château.
- Vol de pierres du château adverse :
  - autorisé seulement quand toutes les carrières sont vides ;
  - la formulation sur la destruction du château adverse est **ambiguë** en bêta, à clarifier via la FAQ ;
  - sinon, pénalité de 50 points pour non fair-play.

**Points** :
- p1 par pierre dans une zone de construction ;
- p2 par mur ;
- p3 par tour ;
- p4 par porte.

## E.2 — Il n'y en a qu'un, et c'est le mien ! (le Graal)
**Préparation**
- Le Graal est déposé dans la salle du trône, entièrement dans la zone.
- Le robot ne doit pas le tenir.

**Contraintes du Graal**
- Il respecte les mêmes contraintes de construction qu'un PAMI (voir `docs/reglement-general.md`, F.7).
- Pendant le match, le robot le prend et le pose sur un support : pierre, construction ou robot.
- Les points dépendent de sa hauteur à la fin du match :
  - niveau 1 : une pierre couchée (10 cm) ;
  - niveau 2 : deux pierres couchées (20 cm) ;
  - niveau 3 : trois pierres couchées ou une pierre debout (30 cm) ;
  - niveau 4 : sur le toit du robot, sans actionneur ni fixation, au-dessus de 30 cm.
- Il peut dépasser la hauteur limite, jusqu'à 430 mm, seulement dans les zones de construction
  et la cour du château. En catégorie senior, il ne doit pas bloquer une balise adverse.
- Il ne doit jamais sortir de la zone du château. S'il sort, il est retiré de la table.
- S'il est encore contrôlé par un actionneur à la fin du match, il ne compte pas.

**Points** :
- p5 si le Graal est dans le château ;
- p6 par niveau d'élévation.

## E.3 — Le retour du roi
- Le robot principal doit être arrêté dans sa salle du trône à la fin du match.
- Les PAMI sont exclus de cette action.
- Partiellement en zone : une partie de la projection verticale est dans la zone.
- Totalement en zone : toute la projection verticale est dans la zone.

**Points** :
- p7 si le robot est partiellement dans la zone ;
- p8 s'il y est complètement.

## E.4 — À l'assaut !
**Phase d'attaque** : de la 85e seconde à la fin du match. Les actions faites en dehors ne comptent pas.

**Départ des PAMI**
- Ils sont posés dans les écuries pendant la préparation, entièrement dans la zone.
  La ligne et le bord de table adjacent (22 mm) font partie de la zone.
- **Un PAMI qui sort de l'écurie avant la phase d'attaque est retiré de la table**
  et n'est plus considéré comme en jeu.

**Boulets**
- Ils sont chargés pendant la préparation dans le robot et les PAMI, 1 boulet maximum par PAMI.
- Le robot et les PAMI peuvent les lancer dans le château adverse.
- Un boulet compte si une partie de sa projection verticale est dans la cour adverse à la fin du match.
- Il est interdit d'influencer volontairement la position ou la trajectoire des boulets adverses.

**Douves**
- Un PAMI compte si une partie de sa projection verticale est dans une douve du château adverse
  à la fin du match.

**Attaque d'un PAMI adverse**
- Un actionneur de votre PAMI doit toucher un PAMI adverse à la fin du match.
- Cet actionneur doit avoir une couleur distincte du reste du PAMI.
- Il ne doit pas endommager le PAMI adverse.
- C'est la seule partie autorisée à toucher le PAMI adverse.

**Points** :
- p9 par douve occupée ;
- p10 si au moins un PAMI attaque un PAMI adverse ;
- p11 par boulet dans le château adverse.

## Points (F)
- Le barème n'est **pas publié** en version bêta.
- Les équipes peuvent proposer un équilibrage jusqu'au 13 octobre sur www.eurobot.org/proposition_equilibrage.
- Le barème final sera publié avec la version officielle.

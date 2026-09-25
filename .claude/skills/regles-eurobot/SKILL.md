---
name: regles-eurobot
description: Répond aux questions sur le règlement Eurobot 2027 (règles de jeu et règlement général) et vérifie qu'une idée, un design mécanique ou du code respecte les règles. Utilise cette skill dès qu'il est question de règlement, de conformité, de dimensions ou de masse du PAMI ou du robot, de points, de pénalités, de forfait, de minuteur (85 s, 100 s), d'homologation, d'une action de jeu (pierres, Graal, douves, boulets, salle du trône), ou quand une décision technique pourrait toucher une règle, même si le mot « règlement » n'est pas prononcé.
---

# Règles Eurobot 2027

## Sources, de la plus forte à la plus faible
1. **FAQ officielle** (www.eurobot.org/faq). Les réponses d'un arbitre référent font foi.
   Tu n'y as pas forcément accès. Si une question y est probablement tranchée, dis-le
   et propose la question à poser.
2. **PDF officiels** dans `docs/pdf/` : règles de jeu 2027 (bêta 0.4) et règlement général (v1.3).
3. **Résumés** : `docs/regles-jeu-2027.md` et `docs/reglement-general.md`.

Commence par les résumés, c'est rapide. Vérifie dans le PDF quand :
- la question porte sur une valeur ou une formulation exacte ;
- le résumé ne tranche pas ;
- la réponse a des conséquences importantes (achat, conception mécanique, stratégie).

Pour lire un PDF : `pdftotext -layout docs/pdf/<fichier>.pdf - | grep -n -i -C5 "<mot-clé>"`.
Le PDF des règles 2027 contient un filigrane « VERSION BETA » : des lettres isolées
parasitent le texte extrait. Ignore-les.

## Comment répondre
- Cite toujours la section : « Règles 2027, E.4.b » ou « Règlement général, F.4.c ».
- Sépare clairement trois choses :
  - **ce que dit la règle** ;
  - **ton interprétation** ;
  - **ce qui reste ambigu**.
- Version bêta 0.4 :
  - les points (p1 à p11) ne sont pas fixés ;
  - certaines formulations sont floues (ex. vol de pierres, E.1.b).
  Signale-le au lieu de supposer.
- Ne propose jamais de contourner une règle, ni d'exploiter une ambiguïté contre l'esprit
  du jeu. Le règlement définit l'anti-jeu comme « nuire sans construire ».
  Si une idée s'en approche, dis-le franchement et propose une alternative.
- Quand la réponse change une décision de l'équipe, suggère une entrée dans `docs/decisions.md`.

## Vérifier la conformité d'un design ou de code
Parcours les points qui s'appliquent au cas présenté :

**Dimensions et masse (règlement général F.3, F.7)**
- PAMI : hauteur ≤ 150 mm au départ, plus grand qu'un cube de 100 mm, périmètre ≤ 600 / 700 mm,
  altitude ≤ 350 mm, masse ≤ 1,5 kg, zone de 30 × 30 mm pour l'autocollant.
- Robot : périmètre ≤ 1200 / 1400 mm, hauteur ≤ 350 mm, support de balise à 430 ± 5 mm.

**Sécurité (F.4)**
- BAU conforme et coupant réellement les actionneurs.
- Batterie autorisée, avec ses conditions.
- Lasers, lumière, son, air comprimé dans les limites.

**Évitement (F.6)**
- Toujours actif. Aucune option pour le désactiver.

**Temps**
- PAMI immobile avant 85 s (règles 2027, E.4.b).
- Tout arrêté à 100 s (règlement général, H.3).

**Couleur**
- Fonctionne en bleu et en jaune (F.7).

**Actions 2027**
- Actionneur d'attaque de couleur distincte, seule partie qui touche le PAMI adverse, sans dégâts.
- 1 boulet maximum par PAMI.
- Aucun élément compté s'il est encore tenu à la fin du match.

**Communication (F.6)**
- Rien ne communique avec l'extérieur de la table.

Rends le résultat sous forme de tableau :

| Règle | Section | Statut | Action à faire |
|---|---|---|---|
| … | … | ✅ conforme / ⚠️ à vérifier / ❌ non conforme | … |

Mets « à vérifier » quand l'information manque (mesure non faite, référence inconnue),
et indique comment la vérifier.

## Quand une nouvelle version du règlement sort
1. Place le nouveau PDF dans `docs/pdf/`.
2. Compare-le au résumé section par section. Liste les changements.
3. Mets à jour le résumé, la version et la date en tête de fichier.
4. Signale les impacts sur le code, le matériel et `docs/homologation.md`.

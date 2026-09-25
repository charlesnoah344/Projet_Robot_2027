---
name: homologation
description: Prépare l'homologation statique et dynamique du PAMI et du robot principal — crée et tient à jour la checklist docs/homologation.md, repère ce qui manque ou n'est pas prouvé, liste les actions prioritaires et fait répéter à l'équipe les questions des arbitres. À lancer avec /homologation (ajouter « robot » ou « pami » pour cibler).
disable-model-invocation: true
argument-hint: [pami | robot | tout]
---

# Préparation de l'homologation

Cible : $ARGUMENTS. Si rien n'est précisé, traite le PAMI et le robot principal.

**Sources à lire**
- `docs/reglement-general.md` (sections F.3 à F.7 et I.3)
- `docs/regles-jeu-2027.md`
- `docs/materiel.md`
- `docs/journal-tests.md`
- `pami/CLAUDE.md`
- `docs/homologation.md`, s'il existe

## Déroulé
1. **Créer ou mettre à jour `docs/homologation.md`** avec les checklists ci-dessous.
   Pour chaque point :
   - un statut : ✅ prouvé / ⏳ à faire / ❓ inconnu ;
   - une preuve : une mesure (valeur, date) ou un test du journal (`Txxx`).
2. **Ne mets ✅ que s'il existe une preuve.** Une intention (« on va le faire »)
   ou du code non testé ne suffisent pas. Sans preuve, mets ❓ et dis comment vérifier.
3. **Résume l'état** : nombre de points prouvés et à faire, puis les 3 actions les plus urgentes.
4. **Répétition avec les arbitres** : pose à l'équipe 3 à 5 questions de la liste en bas,
   une à la fois. Attends leur réponse avant de corriger ou de compléter.
   Le règlement (D.2) exige qu'ils sachent expliquer leur robot seuls.
   Ne donne pas les réponses d'avance.

## Checklist PAMI

**Homologation statique**
- [ ] Hauteur ≤ 150 mm au départ (mesurée) et posé uniquement sur la table.
- [ ] Plus grand qu'un cube de 100 mm de côté.
- [ ] Périmètre ≤ 600 mm au départ, ≤ 700 mm une fois déployé.
- [ ] Altitude ≤ 350 mm pendant le match.
- [ ] Masse ≤ 1,5 kg (pesée).
- [ ] Zone libre de 30 × 30 mm pour l'autocollant.
- [ ] BAU rouge, Ø ≥ 20 mm, accessible d'un coup de poing, qui coupe immédiatement les moteurs.
- [ ] Batterie autorisée. Si LiPo : sac ignifuge et chargeur à présenter.
- [ ] Aucune partie saillante ou pointue.
- [ ] Démarrage par cordon ≥ 50 cm, ou par le robot principal. Aucun autre démarrage manuel.
- [ ] Actionneur d'attaque de couleur distincte (si utilisé), inoffensif pour le PAMI adverse.
- [ ] Emporte 1 boulet maximum.
- [ ] Aucune communication avec l'extérieur de la table.

**Homologation dynamique**
- [ ] Reste immobile jusqu'à 85 s, puis sort de l'écurie.
- [ ] S'arrête devant un robot adverse et devant un PAMI adverse, puis repart.
- [ ] Tout est arrêté avant 100 s.
- [ ] Fonctionne en bleu **et** en jaune.
- [ ] 3 matchs d'affilée sur une seule charge.

## Checklist robot principal

**Homologation statique**
- [ ] Périmètre ≤ 1200 mm au départ, ≤ 1400 mm une fois déployé.
- [ ] Hauteur ≤ 350 mm (BAU jusqu'à 375 mm), objets manipulés compris.
- [ ] Support de balise :
  - [ ] dessus à 430 ± 5 mm ;
  - [ ] Velcro crochets sur tout le dessus ;
  - [ ] plein et opaque ;
  - [ ] dans un cercle de Ø 20 cm autour du centre ;
  - [ ] supporte 400 g sans fléchir.
- [ ] Espace libre de 100 × 70 mm sur une face verticale.
- [ ] BAU au sommet, rouge, Ø ≥ 20 mm.
- [ ] Cordon de démarrage ≥ 500 mm.
- [ ] Batteries conformes, avec un jeu de rechange chargé.
- [ ] Le Graal respecte les contraintes d'un PAMI.

**Homologation dynamique**
- [ ] Sort de la salle du trône.
- [ ] Valide au moins une action en 100 s.
- [ ] L'évitement fonctionne.
- [ ] S'arrête à la fin du temps.

## Questions probables des arbitres
- Comment votre PAMI détecte-t-il un adversaire ? Et s'il arrive de biais ?
- Que se passe-t-il si le capteur ultrason est débranché ?
- Comment le PAMI sait-il qu'il doit attendre 85 secondes ? Et s'arrêter à 100 ?
- Montrez-nous l'arrêt d'urgence. Qu'est-ce qu'il coupe exactement ?
- Comment choisissez-vous la couleur ? Montrez-nous les deux.
- Quelle batterie utilisez-vous ? Combien de matchs tient-elle ?
- Qui a conçu et fabriqué chaque partie ?

## Rappels
- Toute modification significative après l'homologation doit être signalée aux arbitres (I.3.c).
- Apporter le poster A1 (nom de l'équipe, noms des membres, nationalité, drapeau).

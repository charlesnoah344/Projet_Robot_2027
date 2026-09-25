# Projet Eurobot 2027 — équipe ECAMlibur

Robots autonomes pour la coupe Eurobot 2027 (thème : le roi Arthur, Camelot et le Graal).
Phase actuelle : développement du PAMI (petit robot secondaire). Le robot principal viendra ensuite.

## Comment travailler avec nous
- Nous sommes une équipe de jeunes, niveau intermédiaire en C++ et en électronique.
- Le règlement (général, D.2) exige que l'équipe conçoive son robot et sache expliquer son
  fonctionnement sans aide. Donc :
  - Explique le pourquoi de chaque choix technique, en français simple.
  - Avant toute modification qui touche plus d'un fichier, propose un plan court et attends notre accord.
  - Préfère du code simple et lisible à du code astucieux.
  - Si on te demande « fais-le », fais-le, mais explique ensuite ce que tu as fait.
- Réponds en français. Dans le code : identifiants en anglais (camelCase), commentaires en français.

## Règles d'or (non négociables)
1. Ne jamais désactiver, contourner ou rendre optionnel l'évitement d'obstacles, même « pour tester ».
   C'est une disqualification (règlement général H.4). On peut réduire la vitesse ou le seuil, jamais couper.
2. L'arrêt d'urgence coupe la puissance des moteurs par le matériel, pas seulement par le logiciel.
3. Minuteur : le PAMI reste immobile dans l'écurie jusqu'à 85 s après la tirette (on vise 85,3 s),
   et tout est arrêté à 99,5 s au plus tard, quel que soit l'état du programme.
4. Pas de `delay()` ni de boucle bloquante dans le code qui tourne pendant le match.
5. Le PAMI doit pouvoir jouer bleu et jaune : les trajets sont écrits une fois et mis en miroir.
6. Unités : millimètres, degrés, millisecondes. Toutes les constantes vont dans `config.h`, avec l'unité dans le nom.

## Carte du projet (ne lis ces fichiers que quand la tâche en a besoin)
- `docs/regles-jeu-2027.md` — règles du jeu 2027. Version BÊTA 0.4 : les points ne sont pas encore fixés.
- `docs/reglement-general.md` — contraintes techniques, déroulement des matchs, pénalités.
- `docs/pdf/` — les PDF officiels, à consulter pour une formulation exacte.
- `docs/materiel.md` — inventaire, pièces manquantes, brochage de l'ESP32 (source de vérité pour les broches).
- `docs/decisions.md` — journal des décisions techniques.
- `docs/journal-tests.md` — résultats des tests sur le matériel réel.
- `docs/homologation.md` — checklist d'homologation (créée par /homologation).
- `pami/` — firmware du PAMI (ESP32, C++, framework Arduino, PlatformIO). Voir `pami/CLAUDE.md`.

## Commandes
- Compiler : `cd pami && pio run`
- Téléverser : `cd pami && pio run -t upload`
- Moniteur série : `cd pami && pio device monitor -b 115200`
- Tests de logique sur PC : `cd pami && pio test -e native`

## Habitudes de travail
- Toute décision technique importante : ajoute une entrée dans `docs/decisions.md`.
- Tout test sur le matériel : consigne le résultat dans `docs/journal-tests.md`.
- Quand une règle du règlement est en jeu, cite la section (ex. « règles 2027, E.4.b »).
- Si une règle est incertaine (version bêta, formulation ambiguë), dis-le et propose une question
  à poser sur la FAQ officielle (www.eurobot.org/faq).
- Ne considère jamais qu'une fonction « marche » tant qu'elle n'a pas été testée sur le vrai PAMI.
- Git : commits petits et fréquents, message en français au format `type: description`
  (types : feat, fix, test, docs, refactor).

---
name: revue-securite
description: Revue de sécurité et de conformité du firmware du PAMI avant un téléversement pour un test sur table, un entraînement, l'homologation ou un match. Vérifie évitement, minuteur 85 s / 100 s, démarrage par tirette, arrêt d'urgence, couleur, code bloquant et modes de debug oubliés. À lancer avec /revue-securite.
disable-model-invocation: true
---

# Revue de sécurité du firmware

Lis tout le code de `pami/src/` et `pami/platformio.ini`. Vérifie chaque point ci-dessous.
Pour chaque statut, donne une preuve sous la forme `fichier:ligne`.
Si tu ne peux pas vérifier un point (code absent, comportement qui dépend du matériel),
mets « à vérifier » et indique le test qui le prouverait (skill `banc-de-test`).
N'affirme jamais qu'un point est conforme sans l'avoir lu dans le code.

**Ne corrige rien pendant la revue.** Propose les corrections, puis attends l'accord de l'équipe.

## Points bloquants

**1. Évitement**
- Aucune voie ne commande le driver moteurs sans passer par `safety`.
  Cherche tous les appels au driver, à `ledcWrite`, `analogWrite` et `digitalWrite` sur les broches moteurs.
- Aucun flag, `#ifdef`, paramètre, commande série ou mode qui désactive ou affaiblit l'évitement
  au-delà de la réduction de seuil prévue.
- Un capteur jugé en panne provoque l'arrêt (sécurité par défaut).

**2. Minuteur**
- Aucun mouvement avant 85 s après le retrait de la tirette. Cible : 85,3 s.
- Arrêt total au plus tard à 99,5 s, vérifié à chaque tour de `loop()`, prioritaire sur
  tous les états, y compris AVOIDANCE et les éventuelles tâches FreeRTOS.
- À la fin : moteurs à zéro, servomoteur détaché, plus aucun actionneur actif.
- Calculs de temps par soustraction (`now - start >= duree`), qui résistent au débordement.

**3. Démarrage**
- Moteurs arrêtés dès le début de `setup()`, avant toute autre initialisation.
- Impossible de partir sans séquence « tirette en place, puis tirette retirée ».
  Un PAMI allumé sans tirette ne doit jamais démarrer seul.
- Si le démarrage peut venir du robot principal : la règle des 85 s est garantie localement par le PAMI.

**4. Boucle non bloquante**
- Aucun `delay()` ni boucle d'attente dans le chemin du match.
- `pulseIn()` avec un timeout court, ou mesure par interruption.

**5. Couleur**
- Lue avant le départ, verrouillée au retrait de la tirette.
- Le trajet jaune est bien le miroir du bleu, sans code dupliqué divergent.

**6. Restes de développement**
- Aucun mode test ou debug actif dans l'environnement de match :
  - vitesse de test ;
  - trajet raccourci ;
  - seuil d'évitement modifié ;
  - attente de 85 s réduite pour gagner du temps ;
  - logs série massifs.
- Aucun code de `test_bench/` compilé dans le firmware de match.

## Points importants (non bloquants)
- Seuil d'arrêt cohérent avec la distance de freinage mesurée (voir `docs/journal-tests.md`).
- Constantes centralisées dans `config.h` et origine des valeurs calibrées commentée.
- Broches de `config.h` identiques à celles de `docs/materiel.md`.
- Boucle assez rapide : estime la durée d'un tour, par exemple par un log de fréquence.
- Watchdog configuré pour redémarrer en cas de blocage : optionnel, à discuter.

## Format du rapport

| # | Point | Statut | Preuve | Correction proposée |
|---|---|---|---|---|
| 1 | … | ✅ / ⚠️ à vérifier / ❌ | `fichier:ligne` | … |

Termine par un verdict :
- **✅ OK pour téléverser** ;
- ou **❌ À corriger avant téléversement**, avec la liste des points bloquants à régler, par ordre de priorité.

Ajoute enfin les tests matériels à refaire si le code de sécurité a changé depuis le dernier test consigné.

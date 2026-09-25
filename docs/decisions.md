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

### D004 — Ajout d'un gyroscope
- **Date** : 25/09/2026
- **Statut** : proposée
- **Contexte** : les moteurs n'ont pas d'encodeurs, donc le PAMI dérivera en ligne droite.
- **Options envisagées** : gyroscope seul, moteurs à encodeurs, gyroscope + encodeurs.
- **Décision** : à confirmer par l'équipe (achat nécessaire).

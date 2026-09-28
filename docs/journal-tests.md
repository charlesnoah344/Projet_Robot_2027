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
- **Résultat** : partiel (1 essai sur 3, certains points pas encore vérifiés).
- **Mesures** :
  - tests de logique : 7 sur 7 réussis, exécutés sur l'ESP32 ;
  - boucle : environ 402 000 tours/s en FORWARD, 413 000 tours/s en END ;
  - départ : `DEPART !` puis `IDLE -> FORWARD` après l'appui sur BOOT ;
  - fin : `FIN DU MATCH (99,5 s)` puis `FORWARD -> END`, entre les lignes t = 99,1 s et t = 100,1 s.
    C'est conforme à l'horloge de l'ESP32. L'état END est resté jusqu'à la fin de l'observation (t = 134 s) ;
  - 2 lignes du moniteur corrompues (64 caractères illisibles à la place du « [ », vers t = 50 s et t = 128 s).
- **Pas encore vérifié** :
  - l'attente de 30 s sans départ (BOOT a été appuyé environ 2 s après le démarrage) ;
  - le 2ᵉ appui ignoré ;
  - le temps au chronomètre ;
  - les essais 2 et 3.
- **Incident de téléversement** : `Wrong boot mode detected (0x13)`. La carte ne passe pas toute seule
  en mode téléversement. Le téléversement a réussi ensuite ; la manipulation (BOOT maintenu pendant
  `Connecting`) reste à confirmer par l'équipe.
- **Conclusion et suite** :
  - comportement conforme sur ce premier essai ;
  - faire 2 essais complets pour les points pas encore vérifiés, puis cocher l'étape 1 dans `pami/CLAUDE.md` ;
  - lignes corrompues : hypothèse, un problème de la liaison USB avec le PC (64 octets, c'est la taille
    d'un paquet USB). Sans effet sur le PAMI, qui n'a pas d'USB en match. À surveiller quand les moteurs
    tourneront : si ça devient plus fréquent, ce sera un signe de parasites électriques.

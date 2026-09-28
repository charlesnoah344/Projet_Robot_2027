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

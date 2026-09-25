---
name: banc-de-test
description: Crée des programmes de test isolés pour un composant ou une fonction du PAMI (moteurs, driver, ultrason, capteurs infrarouges, gyroscope, servomoteur, tirette, arrêt d'urgence, minuteur, ligne droite, virages, trajet complet), avec un protocole et un critère de réussite chiffré, et guide le diagnostic méthodique d'un problème matériel. Utilise-la dès qu'on veut tester, calibrer, mesurer, valider un composant, ou quand quelque chose « ne marche pas », « ne répond pas », « redémarre », « dérive » ou « fait n'importe quoi ».
---

# Banc de test du PAMI

**Principe** : on ne fait confiance à un composant qu'après l'avoir testé seul.
Quand le PAMI complet dysfonctionne, cinq causes se mélangent. Un test isolé n'en a qu'une.

## Créer un programme de test
**Emplacement** : `pami/test_bench/<composant>/main.cpp`, avec un environnement PlatformIO dédié
dans `pami/platformio.ini`. Exemple :
```ini
[env:test_ultrason]
platform = espressif32
board = esp32dev
framework = arduino
monitor_speed = 115200
build_src_filter = -<*> +<sensors.cpp> +<../test_bench/ultrason/>
```
- On n'inclut que les modules du firmware nécessaires, pour tester le vrai code.
- Compile avec `pio run -e test_ultrason`.
- Téléverse avec `pio run -e test_ultrason -t upload`.
- Vérifie que la compilation passe. Si le filtre pose problème, explique l'alternative choisie.

**Contenu obligatoire du programme**
- **En-tête en commentaire** :
  - matériel et montage ;
  - consignes de sécurité ;
  - étapes du protocole ;
  - résultat attendu ;
  - critère de réussite chiffré.
- **Affichage série lisible**, une ligne par mesure, avec l'unité.
  Ex. : `[ULTRA] distance = 213 mm`.
- **Commandes série à une lettre** pour piloter le test sans recompiler.
  Ex. : `a` avancer, `r` reculer, `s` stop, `+` / `-` changer la vitesse.
  Affiche l'aide au démarrage.
- **Valeur par défaut sûre** : au démarrage, rien ne bouge avant une commande.

**Sécurité**
- Premiers tests moteurs sur cales, roues en l'air.
- Vitesse limitée au début.
- BAU câblé et à portée de main.
- Un programme de test peut ne pas contenir d'évitement. Il ne doit donc jamais servir de firmware de match.

## Protocoles types
Adapte-les au matériel réel, et propose un critère de réussite adapté.

**Ultrason**
- Mesure à 100, 200, 500 et 1000 mm d'un obstacle plat. Écart visé < 10 mm.
- Recommence avec :
  - un obstacle en biais de 45° ;
  - un objet fin ;
  - un cube de 10 cm (la taille d'un PAMI adverse) ;
  - une pierre en carton.
- Note les cas où l'ultrason ne voit rien : c'est là que les IR doivent prendre le relais.

**Capteurs IR**
- Mesure la distance réelle de détection.
- Refais le test sous une lampe puissante : les rencontres sont très éclairées.
- Teste différentes couleurs et matières (carton, plastique noir, bordure grise).

**Moteurs et driver**
- Vérifie le sens de rotation de chaque roue.
- Trouve la vitesse minimale qui fait bouger le PAMI (zone morte).
- Mesure la dérive sur 1 m en ligne droite sans correction.

**Distance de freinage**
- À la vitesse de match, mesure la distance parcourue entre l'ordre d'arrêt et l'arrêt complet.
- Elle sert à fixer le seuil d'arrêt de l'évitement.

**Gyroscope**
- Mesure la dérive du cap au repos pendant 100 s.
- Mesure l'erreur sur 10 virages de 90°.

**Ligne droite corrigée**
- Écart latéral après 1 m, sur 5 essais.

**Tirette et BAU**
- Faire 20 essais. Visé : 0 raté.
- Pour le BAU : les moteurs s'arrêtent instantanément et l'ESP32 ne redémarre pas.

**Minuteur**
- Chronomètre au téléphone : mouvement autorisé à 85,3 s, arrêt total à 99,5 s.
- Répète l'essai dans les deux couleurs.

**Évitement en situation**
- Un obstacle placé sur le trajet : le PAMI s'arrête avant le contact.
- On retire l'obstacle : il repart.
- Recommence avec un obstacle en mouvement.

**Autonomie**
- 3 matchs d'affilée sur une seule charge, attente comprise.

## Diagnostiquer un problème
Procède du plus simple au plus complexe. Une seule hypothèse à la fois.
Demande à l'équipe des observations et des mesures plutôt que de deviner.
1. **Alimentation** : tension mesurée au multimètre sur le composant lui-même, pas sur la batterie.
   Vérifie aussi pendant que les moteurs tournent.
2. **Masse commune** : GND de l'ESP32 = GND du driver = GND des capteurs.
3. **Câblage** : conforme à `docs/materiel.md`, et broche identique dans `config.h`.
4. **Niveaux logiques** : 3,3 V ou 5 V, pont diviseur présent si nécessaire.
5. **Composant seul** : programme de test isolé, voire exemple minimal de la bibliothèque.
6. **Seulement ensuite, le code** : logs, valeurs intermédiaires, timing.

Symptômes fréquents :
- **L'ESP32 redémarre quand les moteurs démarrent** : chute de tension.
  Pistes : rampes, condensateurs, alimentation séparée.
- **L'ultrason donne 0 ou des valeurs aléatoires** : ECHO en 5 V non adapté,
  mesures trop rapprochées (< 60 ms), ou obstacle trop proche (< 20-30 mm).
- **Les IR détectent tout le temps** : lumière ambiante, ou potentiomètre de sensibilité mal réglé.
- **Le PAMI tourne au lieu d'avancer** : un moteur est câblé à l'envers,
  ou une broche de sens est inversée dans `config.h`.

## Consigner le résultat
Ajoute une entrée `Txxx` dans `docs/journal-tests.md` en suivant le modèle.
Numérote-la à la suite de la dernière.
- Si des valeurs sont calibrées, reporte-les dans `pami/src/config.h` avec `// Txxx` en commentaire.
- Si le test valide une étape, propose de la cocher dans `pami/CLAUDE.md`.
- Coche aussi la broche dans `docs/materiel.md` si elle est validée.

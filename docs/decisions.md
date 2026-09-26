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

### D004 — Encodeurs pour les distances et la ligne droite, gyroscope optionnel plus tard
- **Date** : 26/09/2026 (proposée le 25/09/2026)
- **Statut** : adoptée
- **Contexte** :
  - le PAMI doit aller droit et mesurer ses déplacements ;
  - la première version de cette entrée supposait des moteurs sans encodeurs. En fait, les moteurs
    FIT0450 ont chacun un encodeur à effet Hall, à 2 voies.
- **Options envisagées** :
  - gyroscope seul : mesure bien le cap, mais pas la distance. Achat nécessaire ;
  - encodeurs seuls : déjà sur les moteurs. Mesurent la distance, la vitesse de chaque roue et
    l'angle de virage. Faussés si une roue glisse ;
  - encodeurs + gyroscope : le plus précis, mais plus de code et un achat.
- **Décision** :
  - encodeurs pour les distances, la ligne droite et les angles de virage ;
  - gyroscope optionnel, plus tard, si les virages dérivent trop.
- **Pourquoi** :
  - les encodeurs sont déjà là ;
  - ils mesurent la distance, ce qu'un gyroscope ne sait pas faire ;
  - ils permettent de réguler la vitesse de chaque roue ;
  - la correction de cap se fait par la différence des distances parcourues par les deux roues.
- **Conséquences** :
  - 4 broches A/B, et des ponts diviseurs si les signaux sont en 5 V (voir `docs/materiel.md`) ;
  - lecture par interruption sur les deux fronts de la voie A. À chaque front, on lit B pour
    connaître le sens de rotation ;
  - si des ticks se perdent, plan B : le compteur matériel PCNT de l'ESP32 ;
  - limite : les roues peuvent glisser, surtout en virage. Le gyroscope pourra compléter plus tard.
    Les broches I2C (GPIO 21/22) restent réservées.

### D005 — Exception temporaire au minuteur pendant le développement
- **Date** : 26/09/2026
- **Statut** : adoptée — **temporaire, à lever avant tout match**
- **Contexte** : pendant le développement, attendre 85 s avant chaque essai ralentit beaucoup les tests.
  La tirette n'existe pas encore.
- **Décision**, pour le firmware de développement uniquement :
  - pas d'attente de 85 s : c'est une exception volontaire de l'équipe à la règle d'or n° 3 ;
  - départ par un appui sur le bouton BOOT de la carte (GPIO 0), jamais automatiquement à l'allumage ;
  - l'arrêt total à 99,5 s après le départ est conservé ;
  - toutes les autres règles d'or s'appliquent. En particulier, l'évitement reste toujours actif
    et toutes les commandes moteurs passent par `safety`.
- **Règles en jeu** :
  - règles 2027, E.4 : un PAMI qui sort de l'écurie avant la phase d'attaque (85 s) est retiré de la table ;
  - règlement général, F.6 « Démarrage » : seul le cordon d'au moins 500 mm est homologué.
    Un bouton ne l'est pas ;
  - règlement général, H.4 : faux départ −50.
- **Conséquences** :
  - le firmware affiche un avertissement à chaque démarrage ;
  - la constante `MATCH_MOVE_ALLOWED_MS` existe dans `config.h`, mais n'est pas encore utilisée ;
  - **avant tout match ou homologation**, il faudra :
    - la tirette ;
    - l'attente de 85,3 s ;
    - le test au chronomètre ;
    - puis passer cette décision à « remplacée ».
    La case correspondante est dans `pami/CLAUDE.md`.

### D006 — Brochage des moteurs adapté au driver MDD3A
- **Date** : 26/09/2026
- **Statut** : adoptée
- **Contexte** : le premier brochage prévoyait 3 broches par moteur (PWM + 2 sens), comme un L298N.
  Le MDD3A n'a que 2 entrées par moteur.
- **Décision** : 2 broches par moteur, gauche GPIO 25/26, droit GPIO 33/32. Commande :
  - A = PWM et B = 0 : avance ;
  - A = 0 et B = PWM : recule ;
  - A = B = 0 : frein.
- **Pourquoi** :
  - c'est le mode de commande du MDD3A (fiche Cytron) ;
  - le frein arrête les roues plus vite que la roue libre, ce qui raccourcit la distance d'arrêt
    devant un obstacle ;
  - PWM à 20 kHz, le maximum accepté par le driver, pour que les moteurs ne sifflent pas.
- **Conséquences** :
  - GPIO 27 et 13 sont libérées et servent aux encodeurs ;
  - un seul fichier (`motor_driver.cpp`) connaît cette logique.

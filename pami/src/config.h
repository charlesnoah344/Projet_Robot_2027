// config.h — toutes les constantes du PAMI : broches, durées, seuils, vitesses.
//
// Règles :
// - l'unité est dans le nom (_MS, _MM, _PCT, _HZ...) ;
// - les broches sont recopiées de docs/materiel.md, qui reste la source de vérité ;
// - une valeur calibrée porte en commentaire le numéro du test qui l'a mesurée (// Txxx) ;
// - ce fichier ne dépend pas d'Arduino, car il sert aussi aux tests sur PC.
#pragma once

#include <stdint.h>

// ---------------------------------------------------------------------------
// Journal série
// ---------------------------------------------------------------------------
constexpr uint32_t SERIAL_BAUD = 115200;  // doit être égal à monitor_speed (platformio.ini)
// false : plus aucun log pendant le fonctionnement (seul le message de démarrage reste).
constexpr bool DEBUG_LOGS = true;
// Un message d'état par seconde : assez pour suivre, sans saturer le port série.
constexpr uint32_t STATUS_LOG_PERIOD_MS = 1000;

// ---------------------------------------------------------------------------
// Wi-Fi (D010) : téléverser et lire les messages sans câble
// ---------------------------------------------------------------------------
// Le PAMI crée son propre réseau (nom et mot de passe : wifi_secrets.h). Son adresse est
// toujours 192.168.4.1, l'adresse par défaut d'un point d'accès ESP32.
constexpr uint16_t WIFI_TELNET_PORT = 23;  // moniteur : pio device monitor -e esp32dev_wifi
constexpr uint8_t WIFI_AP_CHANNEL = 1;     // à changer (1, 6 ou 11) si le canal est encombré
// Messages en attente d'envoi au PC : 32 Ko, soit environ 5 min de messages d'état, ou un match
// Eurobot de 100 s radio coupée. Tampon plein : les nouveaux messages sont perdus (et comptés),
// mais loop() n'attend jamais le réseau.
constexpr uint32_t WIFI_LOG_BUFFER_BYTES = 32768;
constexpr uint32_t WIFI_KEY_BUFFER_BYTES = 64;  // touches tapées dans le moniteur Wi-Fi
// Le réseau est servi toutes les 10 ms : largement assez pour un moniteur, presque rien pour loop().
constexpr uint32_t WIFI_UPDATE_PERIOD_MS = 10;
// PROFIL LABO (D010) : Wi-Fi allumé pendant les essais, pour lire les messages en direct.
// EUROBOT : false. Le règlement général (F.6) interdit toute communication avec l'extérieur de la
// table pendant le match : la radio s'éteint au départ et se rallume à la fin.
constexpr bool WIFI_ON_DURING_MATCH = true;

// ---------------------------------------------------------------------------
// Broches (copie de docs/materiel.md, brochage du 25/11/2026, D018)
// ---------------------------------------------------------------------------
// Driver moteurs MDD3A : 2 entrées par moteur (D006). M1 = gauche, M2 = droite.
// IN1 = entrée A (avancer), IN2 = entrée B (reculer). Le sens réel se vérifie avec test_moteurs.
constexpr uint8_t PIN_MOTOR_LEFT_A = 27;   // M1_IN1
constexpr uint8_t PIN_MOTOR_LEFT_B = 26;   // M1_IN2
constexpr uint8_t PIN_MOTOR_RIGHT_A = 16;  // M2_IN1 (libre sur un module ESP32-WROOM, sans PSRAM)
constexpr uint8_t PIN_MOTOR_RIGHT_B = 17;  // M2_IN2

// Encodeurs à effet Hall. Le compteur matériel (PCNT, D022) compte les fronts de A et lit B pour le sens.
// L'échange de A et B de l'essai T021 n'a rien réglé (le défaut changeait juste de roue) : retour
// aux voies du programme de l'équipe, dont le sens a été vérifié à la main (T014).
constexpr uint8_t PIN_ENCODER_LEFT_A = 33;   // encoderA1
constexpr uint8_t PIN_ENCODER_LEFT_B = 32;   // encoderB1
constexpr uint8_t PIN_ENCODER_RIGHT_A = 19;  // encoderA2
constexpr uint8_t PIN_ENCODER_RIGHT_B = 18;  // encoderB2

// Deux ultrasons HC-SR04, à l'avant gauche et à l'avant droit (D012). Pas de 3e capteur.
// Chacun a maintenant son TRIG (D018). On les déclenche quand même ensemble, comme avant.
// ECHO sort du 5 V : pont diviseur 1 kΩ / 2 kΩ obligatoire sur chaque ECHO (l'ESP32 supporte 3,3 V).
// Gauche = « Sensor 1 » du programme de l'équipe. Droite = connecteur droit du PCB, TRIG 5 / ECHO 4
// (T016 : rien ne revenait sur 14 / 12, qui est le connecteur du bas).
constexpr uint8_t PIN_US_LEFT_TRIG = 25;
constexpr uint8_t PIN_US_LEFT_ECHO = 35;   // entrée seule : parfait pour un ECHO
constexpr uint8_t PIN_US_RIGHT_TRIG = 5;   // broche de démarrage : sans risque pour un TRIG (sortie)
constexpr uint8_t PIN_US_RIGHT_ECHO = 4;   // T016

// Côté GAUCHE / DROITE : plus d'interrupteur. Il se choisit avec la commande de départ (D020).
// Départ : plus de bouton ni de tirette. Commande tapée dans le moniteur (D013, D020).

// Servo SG90 du bras (piloté par LEDC, D008).
constexpr uint8_t PIN_SERVO = 13;

// Lecture du BAU (optionnelle) : broche à définir (docs/materiel.md).

// ---------------------------------------------------------------------------
// Moteurs (driver MDD3A)
// ---------------------------------------------------------------------------
// 20 kHz : au-dessus de ce que l'oreille entend (les moteurs ne sifflent pas),
// et c'est le maximum accepté par le MDD3A.
constexpr uint32_t MOTOR_PWM_FREQ_HZ = 20000;
// 10 bits = 1024 niveaux de vitesse. À 20 kHz, l'ESP32 permet au plus 11 bits.
constexpr uint8_t MOTOR_PWM_RESOLUTION_BITS = 10;
// Canaux du générateur PWM de l'ESP32 (LEDC), un par entrée du driver.
constexpr uint8_t LEDC_CHANNEL_MOTOR_LEFT_A = 0;
constexpr uint8_t LEDC_CHANNEL_MOTOR_LEFT_B = 1;
constexpr uint8_t LEDC_CHANNEL_MOTOR_RIGHT_A = 2;
constexpr uint8_t LEDC_CHANNEL_MOTOR_RIGHT_B = 3;
// true si la roue tourne à l'envers quand on lui demande d'avancer.
// L'AVANT du PAMI est le côté de l'ultrason. Gauche et droite se voient depuis l'arrière,
// en regardant vers l'ultrason. On corrige ici plutôt que de recâbler.
// Brochage du 25/11/2026 (D018) : gauche false / droite false, vérifié avec le banc test_moteurs (T013).
// Avant (premier PCB D012, PCB 2026 5EO D015) : gauche true.
// Le sens dépend du branchement des moteurs sur le driver, pas des broches de l'ESP32.
constexpr bool MOTOR_LEFT_INVERTED = false;  // T013
constexpr bool MOTOR_RIGHT_INVERTED = false;
// PWM maximale autorisée, appliquée par motor_driver, même dans les bancs de test.
// Alimentation de labo à 6 V = tension nominale des FIT0450 : 100 %.
// À revoir avec la batterie : avec 8,4 V, il faudra environ 70 % pour rester vers 6 V en moyenne.
constexpr int MOTOR_MAX_PWM_PCT = 100;

// ---------------------------------------------------------------------------
// Encodeurs à effet Hall des FIT0450 (D004)
// ---------------------------------------------------------------------------
// 960 impulsions par tour de roue et par voie (fiche DFRobot). On compte les 2 fronts de la voie A :
// 1920 ticks par tour. Mesuré à la main (T019) : gauche 1924, droite 1946 à 1948 par tour.
// Le « 1400 » du programme de test du premier PCB ne s'applique pas ici : il comptait 4 fronts par
// impulsion, pas 2, et sa valeur n'a jamais été étalonnée.
constexpr int32_t ENCODER_TICKS_PER_WHEEL_TURN = 1920;
// Diamètre « effectif » : celui qui fait correspondre les ticks à la distance vraiment parcourue
// (pneu écrasé par le poids du PAMI, et imprécision des 1920 ticks par tour compris).
// 60 mm au départ (diamètre donné par l'équipe). 59,4 mm mesurés avec test_trajectoire :
// 5 lignes de 950 mm et 3 de 300 mm, droite « réel = 0,990 × cru − 7,6 mm » (± 0,3 mm). // T006
// Les 7,6 mm perdus à chaque ligne, quelle que soit sa longueur, ne viennent pas du diamètre :
// voir T006 dans docs/journal-tests.md.
constexpr float WHEEL_DIAMETER_MM = 59.4f;  // T006
// Rapport des diamètres effectifs droite / gauche (D023). Les deux roues n'avancent pas exactement
// de la même distance pour un tick (pneus, et 1948 / 1939 ticks par tour mesurés à la main, T022) :
// à ticks égaux, le PAMI tournait d'un côté. > 1 : la roue droite avance plus que la gauche par tick.
// Réglage : ligne droite avec test_trajectoire, puis « e » et l'écart réel sur le côté ; le banc
// calcule la valeur (docs/reglage-regulation.md, § 8).
// À RÉGLER SUR LA SURFACE DU MATCH : la dérive change d'un sol à l'autre (T023 / T025).
// Historique :
// - 1,0079 (T022, sol du labo) : 25 mm vers la gauche sur 800 mm. Sur la surface de compétition, cette
//   valeur faisait tourner le PAMI à droite : 48 mm sur 1000 mm, 16 mm sur 500 mm (T025) ;
// - 1,0079 ajoutait à lui seul environ 39 mm de virage à droite sur 1000 mm. Une fois retiré, il reste
//   environ 8 mm sur 1000 mm, moins que l'écart d'un essai à l'autre (31 à 61 mm), dû au placement.
//   Valeur choisie pour que l'écart soit le plus petit possible au bout des 1900 mm de la ligne de la
//   douve (calcul dans T025).
constexpr float WHEEL_DIAMETER_RIGHT_TO_LEFT_RATIO = 1.0000f;  // T025
// Diamètre de chaque roue : leur moyenne reste WHEEL_DIAMETER_MM, donc l'étalonnage des distances
// (T006) ne change pas. Seul l'écart entre les roues change.
constexpr float LEFT_WHEEL_DIAMETER_MM = 2.0f * WHEEL_DIAMETER_MM / (1.0f + WHEEL_DIAMETER_RIGHT_TO_LEFT_RATIO);
constexpr float RIGHT_WHEEL_DIAMETER_MM = LEFT_WHEEL_DIAMETER_MM * WHEEL_DIAMETER_RIGHT_TO_LEFT_RATIO;
// Distance parcourue par chaque roue pour un tick : périmètre de la roue / ticks par tour.
constexpr float LEFT_MM_PER_TICK = 3.14159265f * LEFT_WHEEL_DIAMETER_MM / ENCODER_TICKS_PER_WHEEL_TURN;
constexpr float RIGHT_MM_PER_TICK = 3.14159265f * RIGHT_WHEEL_DIAMETER_MM / ENCODER_TICKS_PER_WHEEL_TURN;
// true depuis le premier PCB (D012), comme dans le programme de test de l'équipe.
// Sans risque si la sortie de l'encodeur est déjà en 0 / 3,3 V (mesure de l'équipe, montage d'avant) ;
// indispensable si le PCB laisse la sortie « en l'air » à l'état haut.
constexpr bool ENCODER_USE_INTERNAL_PULLUP = true;
// true si le compteur diminue quand on fait tourner la roue vers l'AVANT (côté ultrasons) à la main.
// On corrige ici plutôt que de recâbler.
// Brochage du 25/11/2026 (D018) : gauche true / droite false (T014, roues tournées à la main).
// Le PCNT (D022) décode exactement comme l'ancien code : ces valeurs restent valables.
constexpr bool ENCODER_LEFT_INVERTED = true;    // T014
constexpr bool ENCODER_RIGHT_INVERTED = false;  // T014
// Compteur matériel PCNT (D022).
// Filtre anti-parasites : une impulsion plus courte que ce temps est ignorée. Les parasites des
// moteurs durent bien moins d'une microseconde. Un vrai front arrive au plus vite toutes les 278 µs
// à 350 mm/s, et toutes les 97 µs à 1 m/s : 5 µs laisse une grande marge des deux côtés.
// Le circuit accepte de 13 à 12 787 ns.
constexpr uint32_t ENCODER_GLITCH_FILTER_NS = 5000;
// Le compteur matériel est sur 16 bits : il repart de 0 en arrivant à ± cette valeur. encoders.cpp
// ajoute régulièrement ce qu'il a compté à un total sur 32 bits.
constexpr int16_t ENCODER_PCNT_LIMIT_TICKS = 30000;
// Période de ce ramassage. Il faut moins d'une demi-plage (15 000 ticks, environ 7 tours de roue)
// entre deux ramassages : 10 ms laissent une marge énorme (environ 36 ticks à 350 mm/s).
constexpr uint32_t ENCODER_COLLECT_PERIOD_MS = 10;

// ---------------------------------------------------------------------------
// Mouvement (motion) : régulation de vitesse et trajectoires
// ---------------------------------------------------------------------------
// Toutes ces valeurs sont PROVISOIRES : à régler aux étapes D et E, sur le vrai PAMI.
// Écart entre les points de contact des deux roues. Sert à mesurer les angles :
// angle (rad) = (distance roue droite - distance roue gauche) / WHEEL_BASE_MM.
// Entraxe « effectif » : celui qui fait correspondre les ticks à l'angle vraiment tourné (les pneus
// frottent un peu en tournant sur place : il est souvent un peu plus grand que l'entraxe mesuré à la règle).
// 100 mm au départ (donné par l'équipe). 101,6 mm mesurés avec test_trajectoire : r 360 et r -360,
// 3 fois chacun, angle réel comparé à l'angle cru PAMI arrêté. // T007
// Attention : le PAMI tourne 4° de moins vers la gauche que vers la droite (102,2 / 101,0 mm) ;
// l'entraxe, lui, ne peut corriger que la moyenne. Voir T007 dans docs/journal-tests.md.
constexpr float WHEEL_BASE_MM = 101.6f;  // T007
// La régulation tourne toutes les 20 ms : assez souvent pour corriger, assez rarement pour que
// chaque mesure de vitesse compte plusieurs ticks.
constexpr uint32_t MOTION_PERIOD_MS = 20;
// Vitesse de croisière en ligne droite (D019). 150 mm/s pour les premiers réglages de l'odométrie
// (13/10/2026), puis 350 mm/s pour atteindre la douve à temps : environ 2,2 m de trajet en 8,5 s,
// pour 9,5 s de mouvement au labo (16 s à 150 mm/s).
// Pas 400 : c'est à peu près le maximum des FIT0450 sous 6 V, sans marge pour la régulation.
// PROVISOIRE : l'anticipation a été mesurée jusqu'à 200 mm/s seulement. À revérifier avec test_pid,
// puis l'étalonnage avec test_trajectoire (le glissement à l'arrêt grandit avec la vitesse).
// Essai T015 fait (à 150 mm/s, la déviation reste, plus faible) : retour à 350 (D019). C'est aussi la
// vitesse maximale du banc test_trajectoire (« v 150 » ou « v 350 » sans re-téléverser).
constexpr float DRIVE_SPEED_MM_S = 350.0f;
// Vitesse de chaque roue pendant une rotation sur place (baissée de 120 à 90 mm/s).
// Inchangée : un virage de 51° ne dure que 0,7 s, aller plus vite ferait surtout glisser le cap.
constexpr float TURN_SPEED_MM_S = 90.0f;
// Accélération et freinage : progressifs, pour ne pas patiner ni créer de pic de courant.
// 400 -> 500 mm/s² (D019) : on gagne environ 0,3 s sur le trajet de la douve. PROVISOIRE.
// Passée à 200 par l'équipe pendant les essais du 25/11, remise à 500 le 26/11 pour la mission douve :
// à 200, le trajet dure environ 10,4 s, plus que les 9,5 s de mouvement du labo (arrêt avant la douve).
// Si les roues patinent au démarrage, essayer 400.
constexpr float MOTION_ACCEL_MM_S2 = 500.0f;
// En dessous de cette vitesse, les moteurs risquent de caler (zone morte) : on ne descend pas plus bas.
constexpr float MOTION_MIN_SPEED_MM_S = 40.0f;
// On considère la cible atteinte à moins de 3 mm (ligne droite) ou 2 mm d'arc de roue (rotation).
constexpr float DRIVE_TOLERANCE_MM = 3.0f;
constexpr float TURN_TOLERANCE_MM = 2.0f;

// Régulateur de vitesse de chaque roue : PWM = anticipation + correction proportionnelle + intégrale.
// Anticipation : PWM nécessaire pour décoller (zone morte) + PWM par mm/s voulu.
// Valeurs communes de départ, remplacées par les valeurs mesurées de chaque roue ci-dessous (T004).
// Elles ne servent plus que de valeurs par défaut au constructeur de SpeedController.
constexpr float MOTOR_DEADBAND_PCT = 10.0f;     // PROVISOIRE, avant mesure
constexpr float MOTOR_PCT_PER_MM_S = 0.2f;      // PROVISOIRE, avant mesure
// Deux moteurs ne sont jamais identiques : chaque roue a ses propres valeurs.
// Mesurées au sol avec l'escalier de test_pid (moyenne de 3 essais, alim de labo 6 V, PCB 2026 5EO).
// Valables entre 65 et 205 mm/s (vitesses réellement mesurées pendant l'escalier) : en dessous,
// le frottement au démarrage peut demander plus que la droite ne le prévoit. // T004
// Si la tension change (batterie), refaire l'escalier.
constexpr float LEFT_MOTOR_DEADBAND_PCT = 1.5f;    // T004 (essais : 1,2 / 1,6 / 1,6)
// Pentes : valeurs de T004 × 60 / 59,4. Elles avaient été mesurées en mm/s calculés avec un diamètre de
// 60 mm ; avec 59,4 mm (T006), la même rotation de roue vaut 1 % de mm/s en moins. // T006
constexpr float LEFT_MOTOR_PCT_PER_MM_S = 0.194f;  // T004 (essais : 0,195 / 0,191 / 0,191) × 60/59,4
constexpr float RIGHT_MOTOR_DEADBAND_PCT = 1.1f;   // T004 (essais : 1,0 / 1,0 / 1,2)
constexpr float RIGHT_MOTOR_PCT_PER_MM_S = 0.198f; // T004 (essais : 0,197 / 0,197 / 0,195) × 60/59,4
constexpr float SPEED_KP_PCT_PER_MM_S = 0.05f;  // 100 mm/s d'écart -> 5 % de PWM en plus
constexpr float SPEED_KI_PCT_PER_MM = 0.3f;     // corrige l'écart qui dure (pente, frottements)
constexpr float SPEED_INTEGRAL_MAX_PCT = 30.0f;  // l'intégrale ne peut pas « s'emballer »
// Ligne droite : si la roue gauche a parcouru 1 mm de plus que la droite, on la ralentit de
// 5 mm/s et on accélère la droite d'autant. Même gain pour garder la rotation bien sur place.
constexpr float HEADING_KP_MM_S_PER_MM = 5.0f;

// Roue bloquée ou encodeur inversé : une roue qui reçoit au moins 35 % de PWM mais qui ne tourne pas
// (ou tourne à l'envers) pendant 0,6 s. Le PAMI passe en FAULT et s'arrête.
// Sans ça, un encodeur inversé ferait accélérer le régulateur jusqu'à la pleine vitesse.
constexpr float STALL_MIN_PCT = 35.0f;
constexpr float STALL_MAX_SPEED_MM_S = 15.0f;
constexpr uint32_t STALL_TIME_MS = 600;

// ---------------------------------------------------------------------------
// Missions (logic/missions.h)
// ---------------------------------------------------------------------------
// Trois missions de réglage (grille du labo), et la mission de match.
enum class MissionId {
  STRAIGHT_LINE,  // ligne droite de STRAIGHT_LINE_LENGTH_MM
  ROTATION,       // un tour complet sur place, par quarts de tour
  SQUARE,         // carré de SQUARE_SIDE_MM de côté, retour au départ
  MOAT            // va à la douve de son côté (MOAT_NEAR_MM), puis y reste, bras prêt (D017)
};
// La mission jouée. Pour en changer : modifier cette ligne, puis recompiler et téléverser.
// Ordre conseillé pour les tests : STRAIGHT_LINE, ROTATION, SQUARE, puis MOAT.
// MOAT depuis le 26/11 : les 2 ultrasons marchent (T016, T024) et, à 350 mm/s avec 500 mm/s²,
// le trajet (environ 8,5 s) tient dans les 9,5 s de mouvement du labo (D019).
// Il faut environ 2,2 m de place : 1,9 m tout droit, virage d'environ 51° vers le centre, 28 cm.
constexpr MissionId ACTIVE_MISSION = MissionId::MOAT;

// Il n'y a que 9,5 s de mouvement (de 5 s à 14,5 s) : chaque mission doit tenir dedans,
// avec de la marge pour un détour. PROVISOIRE : à ajuster d'après les vitesses réelles.
// Raccourcies pour les essais d'odométrie au sol (13/10/2026) : 1000 -> 500 mm, 250 -> 200 mm.
constexpr float STRAIGHT_LINE_LENGTH_MM = 250.0f;
constexpr float SQUARE_SIDE_MM = 200.0f;
// Mission douve (D017) : chaque PAMI va à la douve diagonale de SON côté. Les deux PAMI ne visent
// jamais la même douve, ils ne se gênent donc pas.
// Points dans le REPÈRE DE LA TABLE vu par le PAMI (docs/douves-coordonnees.md, plan G.1, ± 5 mm) :
//   x = avance depuis notre petit bord (derrière l'écurie), vers l'adversaire ;
//   y = distance au grand bord côté écurie, en allant vers le centre de la table.
// Ce repère est le même pour les 4 écuries : la douve « de son côté » est toujours MOAT_NEAR_MM.
// logic/missions.h les convertit dans le repère de l'odométrie (départ en (0, 0)), puis forSide()
// fait le miroir selon l'interrupteur : on ne les écrit qu'une fois (règle d'or n° 5).
// Le PAMI compte s'il a une partie de lui dans une douve adverse à la fin du match (règles 2027, E.4) :
// on vise le centre de la douve, pour qu'une erreur d'odométrie de quelques centimètres ne l'en
// fasse pas sortir.
struct TablePoint {
  float xMm;
  float yMm;
};
// PROVISOIRE : milieu de l'essieu du PAMI posé dans l'écurie (x 0–200, y 0–300), cap vers +x.
// y = 210 : même y que le point de virage, la première ligne droite reste dans le couloir entre les
// carrières de bord (y <= 110) et nos remparts (y >= 305). À corriger d'après le PAMI réel.
constexpr TablePoint START_POINT_MM = {100.0f, 210.0f};
// Point de virage : au bout de la ligne droite, le PAMI tourne d'environ 51° vers le centre.
constexpr TablePoint MOAT_WAYPOINT_MM = {2000.0f, 210.0f};
// Centres des 3 douves adverses. Seule MOAT_NEAR_MM est assignée aux PAMI ; les deux autres sont
// gardées pour plus tard (robot principal).
constexpr TablePoint MOAT_NEAR_MM = {2179.0f, 429.0f};     // douve diagonale de notre côté
constexpr TablePoint MOAT_CENTER_MM = {1950.0f, 1000.0f};  // douve centrale (non assignée)
constexpr TablePoint MOAT_FAR_MM = {2179.0f, 1571.0f};     // douve diagonale de l'autre côté (non assignée)

// Point atteint à moins de 30 mm. Cap atteint à moins de 3°.
// Si le PAMI est mal orienté, il se retourne vers le point avant d'avancer.
constexpr float GOTO_REACHED_TOLERANCE_MM = 30.0f;
constexpr float HEADING_TOLERANCE_DEG = 3.0f;

// ---------------------------------------------------------------------------
// Ultrasons HC-SR04 (gauche et droit, D012) et sécurité
// ---------------------------------------------------------------------------
// Les deux capteurs partagent leur TRIG (D015) : ils émettent ensemble, toutes les 60 ms au moins.
// 60 ms : le son précédent et ses échos doivent avoir disparu. Chaque capteur mesure donc environ
// 16 fois par seconde.
constexpr uint32_t US_PERIOD_MS = 60;
// Un ECHO resté haut aussi longtemps n'est pas normal (58 ms au plus sans écho pour nos modules) :
// on déclenche quand même. Ce capteur ne répondra pas et sera déclaré en panne au bout de
// SENSOR_FAULT_TIMEOUT_MS, mais l'autre continue de mesurer : le diagnostic reste clair.
constexpr uint32_t US_ECHO_STUCK_MS = 2 * US_PERIOD_MS;
// Impulsion de déclenchement demandée par la fiche du HC-SR04.
constexpr uint32_t US_TRIGGER_PULSE_US = 10;
// Écho plus long que 12 000 µs (environ 2 m) : « rien dans la portée ». C'est normal, pas une panne.
constexpr uint32_t US_MAX_ECHO_US = 12000;
// Distance renvoyée quand il n'y a rien dans la portée.
constexpr uint16_t US_NO_OBSTACLE_MM = 2000;
// Le son fait 343 m/s, soit 0,343 mm/µs. Il fait l'aller-retour : 0,1715 mm par µs d'écho.
constexpr float US_MM_PER_ECHO_US = 0.1715f;
// Filtre de sécurité (logic/rise_filter.h) : une distance qui s'allonge de plus de 50 mm d'un coup
// doit être vue 4 fois de suite par le même capteur (environ 0,25 s) avant d'être crue.
// Une distance qui diminue est crue tout de suite. Réglé sur les mesures du 15/10/2026 (banc test_ultrason) : jusqu'à 3 échos ratés de
// suite devant un PAMI immobile, ou un saut vers le mur du fond.
constexpr uint16_t US_RISE_THRESHOLD_MM = 50;
constexpr uint8_t US_RISE_CONFIRM_COUNT = 4;
// Aucune mesure valide pendant 1 s : le capteur est considéré en panne (fil débranché, ECHO bloqué).
// Sécurité par défaut : panne = obstacle, le PAMI s'arrête.
constexpr uint32_t SENSOR_FAULT_TIMEOUT_MS = 1000;

// Obstacle plus proche que ce seuil devant l'un OU l'autre ultrason : safety freine, quel que soit
// l'état. PROVISOIRE, volontairement large : à régler d'après la distance de freinage mesurée.
// 150 -> 200 mm avec la vitesse de 350 mm/s (D019) : le temps que la mesure confirme l'obstacle,
// le PAMI parcourt environ 40 mm, puis il glisse en freinant.
// Revers : un seuil plus grand voit plus tôt ce qui est sur le côté (carrières le long du bord).
constexpr uint16_t OBSTACLE_STOP_DISTANCE_MM = 200;
// L'obstacle n'est considéré parti qu'au-delà de seuil + 30 mm.
// Sans cette marge, une mesure qui oscille autour du seuil ferait avancer et freiner en boucle.
constexpr uint16_t OBSTACLE_CLEAR_MARGIN_MM = 30;

// ---------------------------------------------------------------------------
// Évitement (états AVOID_STOP et DETOUR)
// ---------------------------------------------------------------------------
// Stratégie prudente (D016) : s'arrêter, attendre, contourner par le centre, puis reprendre la mission.
// Niveau 1, arrêt : safety freine dès que l'ultrason voit un obstacle devant.
// Niveau 2, attente : obstacle toujours là après 1,5 s -> contournement. La plupart des obstacles de la
// phase PAMI bougent (PAMI adverse, robot qui passe) et s'en vont en 1 à 2 s. 1,5 s est un compromis :
// attendre plus ne sert à rien devant un obstacle fixe, et le temps est compté (14,2 s de mouvement
// pour un PAMI à Eurobot, 9,5 s au labo).
constexpr uint32_t AVOID_WAIT_MS = 1500;
// Niveau 3, contournement par le centre (logic/detour.h) : quart de tour vers le centre de la table
// (à droite pour un départ à gauche, à gauche pour un départ à droite, D014), pas de côté, quart de
// tour retour (parallèle à la route), pause, regard, puis dépassement de l'obstacle.
constexpr float DETOUR_TURN_DEG = 90.0f;
constexpr float DETOUR_SIDE_MM = 200.0f;  // PROVISOIRE : un peu plus que la largeur d'un PAMI
// Pause après le quart de tour retour, avant de regarder : le PAMI finit de glisser, et le filtre de
// l'ultrason accepte une distance qui s'allonge après 4 mesures (environ 0,25 s).
constexpr uint32_t DETOUR_LOOK_MS = 300;
// Le PAMI ne dépasse l'obstacle que si la voie est libre sur plus de 300 mm devant lui ; il avance
// alors de 300 mm, l'évitement toujours actif. PROVISOIRE : à régler d'après la taille des obstacles.
constexpr float DETOUR_PASS_MM = 300.0f;
// Voie encore bloquée au regard : un pas de côté de plus. 2 pas au plus, soit 400 mm de décalage.
constexpr uint8_t DETOUR_MAX_SIDESTEPS = 2;
// Au plus 2 contournements par match (environ 7 s chacun). Ensuite, le PAMI reste arrêté et repart
// dès que la voie se libère : il ne force jamais le passage.
constexpr uint8_t AVOID_MAX_DETOURS = 2;

// ---------------------------------------------------------------------------
// Bras du PAMI (servo SG90, D008)
// ---------------------------------------------------------------------------
// Servo piloté directement par le générateur PWM de l'ESP32 (LEDC), sans bibliothèque :
// ESP32Servo choisirait elle-même un canal et pourrait prendre celui d'un moteur (canaux 0 à 3).
// Canal 4 : il utilise un autre minuteur (timer 2) que les moteurs (timers 0 et 1),
// donc ses 50 Hz ne changent pas les 20 kHz des moteurs.
constexpr uint8_t LEDC_CHANNEL_SERVO = 4;
constexpr uint32_t SERVO_PWM_FREQ_HZ = 50;         // un servo attend une impulsion toutes les 20 ms
constexpr uint8_t SERVO_PWM_RESOLUTION_BITS = 16;  // réglage fin de la largeur d'impulsion
// Largeur d'impulsion pour 0° et pour 180° (fiche du SG90, à vérifier au test).
constexpr uint32_t SERVO_MIN_PULSE_US = 500;
constexpr uint32_t SERVO_MAX_PULSE_US = 2400;
// Attaque (D021, logic/arm_attack.h) : dans la douve, quand l'ultrason voit l'adversaire à moins
// de ARM_TRIGGER_DISTANCE_MM, le servo tourne de 0 à 120°, y reste 3 s, puis revient à 0°.
// Une fois revenu, il peut attaquer de nouveau si l'adversaire est encore là.
// ⚠ Règles 2027, E.4 : l'attaque ne compte que si l'actionneur TOUCHE le PAMI adverse À LA FIN du
// match. Un bras revenu à 0° à la fin ne rapporte rien (voir D021).
// Positions du bras. PROVISOIRES : à régler quand le bras sera monté (banc test_bras).
constexpr float ARM_REST_DEG = 0.0f;     // position de départ, dans le périmètre du PAMI
constexpr float ARM_ATTACK_DEG = 120.0f; // attaque : 120° depuis la position de départ
constexpr uint32_t ARM_ATTACK_HOLD_MS = 3000;  // temps passé à 120°
// Temps laissé au servo pour revenir à 0° avant une nouvelle attaque. Un SG90 fait environ 60° en
// 0,1 s : 120° en 0,2 s, donc 0,5 s laisse de la marge.
constexpr uint32_t ARM_RETURN_MS = 500;
// Le bras attaque quand l'ultrason voit l'adversaire à moins de 60 mm.
// Le bras ne peut dépasser que d'environ 50 mm (grille : périmètre déployé + 10 cm au plus).
// PROVISOIRE : à régler d'après la longueur réelle du bras.
constexpr uint16_t ARM_TRIGGER_DISTANCE_MM = 60;

// ---------------------------------------------------------------------------
// Minuteur de match
// ---------------------------------------------------------------------------
// Profil labo (D007, grille du Mechatronics Lab) : départ après 5 s, arrêt après 15 s.
// Les deux durées comptent depuis la commande de départ (D013).
// Valeurs Eurobot, pour plus tard (règle d'or n° 3, règles 2027 E.4) :
//   MATCH_MOVE_ALLOWED_MS = 85300 et MATCH_STOP_MS = 99500.
// Immobile jusqu'à 5 s après le départ.
constexpr uint32_t MATCH_MOVE_ALLOWED_MS = 5000;
// Arrêt total à 14,5 s : 0,5 s de marge avant les 15 s, comme les 99,5 s d'Eurobot.
constexpr uint32_t MATCH_STOP_MS = 14500;
// Départ (D013) et côté (D020) : taper une de ces commandes dans le moniteur (Wi-Fi ou USB), puis
// Entrée. En minuscules ; majuscules et espaces ne comptent pas (« GO G » et « gog » marchent aussi).
// - go g : PAMI GAUCHE (bord de table à sa gauche, centre à sa droite) ;
// - go d : PAMI DROITE (bord de table à sa droite, centre à sa gauche).
// « go » seul est refusé : le côté doit toujours être choisi exprès, jamais par défaut.
// Un mot plutôt qu'une touche : une touche frappée par erreur ne lance pas le PAMI.
// ⚠ PROFIL LABO SEULEMENT : interdit à Eurobot (règlement général F.6). Là-bas, le PAMI démarre par
// un cordon de 50 cm, ou par le robot principal pendant le match, et le côté devra venir d'ailleurs.
constexpr char START_COMMAND_LEFT[] = "go g";
constexpr char START_COMMAND_RIGHT[] = "go d";
// Ligne refusée avec un message qui rappelle de choisir le côté.
constexpr char START_COMMAND_WITHOUT_SIDE[] = "go";

// ---------------------------------------------------------------------------
// Bancs de test (test_bench/) : jamais utilisés en match
// ---------------------------------------------------------------------------
// Vitesse de départ prudente : la roue tourne lentement, ou pas du tout (zone morte).
constexpr int TEST_BENCH_START_PWM_PCT = 20;
constexpr int TEST_BENCH_PWM_STEP_PCT = 5;       // touches + et -
constexpr int TEST_BENCH_PWM_FINE_STEP_PCT = 1;  // touches > et < : zone morte au pourcent près
// Sécurité : les moteurs s'arrêtent seuls si personne n'a tapé de commande depuis 10 s.
constexpr uint32_t TEST_BENCH_AUTO_STOP_MS = 10000;
// Banc test_odometrie : un essai s'arrête seul au bout de 15 s (le carré dure environ 11 s).
constexpr uint32_t TEST_BENCH_ODOMETRY_MAX_RUN_MS = 15000;
// Une ligne de mesures toutes les 0,2 s pendant un essai, toutes les secondes au repos.
constexpr uint32_t TEST_BENCH_ODOMETRY_LOG_PERIOD_MS = 200;
constexpr uint32_t TEST_BENCH_ODOMETRY_IDLE_LOG_PERIOD_MS = 1000;
// Banc test_ultrason : une ligne de bilan par seconde (nombre de mesures, sans écho, min, max).
constexpr uint32_t TEST_BENCH_US_SUMMARY_PERIOD_MS = 1000;

// Banc test_pid : le régulateur de vitesse (logic/speed_controller.h) seul, roue par roue.
// Échelon et vitesse constante : consigne de départ = vitesse de croisière du match.
constexpr float TEST_BENCH_PID_STEP_SPEED_MM_S = DRIVE_SPEED_MM_S;
constexpr float TEST_BENCH_PID_SPEED_STEP_MM_S = 25.0f;   // touches p et m
constexpr float TEST_BENCH_PID_MAX_SPEED_MM_S = 400.0f;   // FIT0450 sous 6 V : environ 400 mm/s au sol
constexpr uint32_t TEST_BENCH_PID_STEP_DURATION_MS = 3000;
constexpr uint32_t TEST_BENCH_PID_CONSTANT_DURATION_MS = 8000;  // le temps de freiner la roue au doigt
// Le régime établi (moyenne, erreur, ondulation) est mesuré après ce temps : avant, la roue accélère.
constexpr uint32_t TEST_BENCH_PID_SETTLE_MS = 1500;
// Escalier : 4 paliers, de quoi tracer la droite « PWM = zone morte + pente × vitesse ».
// Jusqu'à 350 mm/s depuis D019 (avant : 50 à 200 mm/s), pour couvrir la nouvelle vitesse de croisière.
// Au sol : 1 s × (100 + 200 + 300 + 350) mm/s = 950 mm, sous la limite de 1000 mm.
constexpr float TEST_BENCH_PID_STAIR_SPEEDS_MM_S[] = {100.0f, 200.0f, 300.0f, 350.0f};
constexpr uint32_t TEST_BENCH_PID_STAIR_DURATION_MS = 1000;
constexpr uint32_t TEST_BENCH_PID_STAIR_SETTLE_MS = 500;  // début de chaque palier ignoré
// Sécurité au sol (mode par défaut, sans évitement) : un essai s'arrête après 1000 mm parcourus
// (800 mm avant D019). Il faut donc 1,2 m de sol dégagé devant le PAMI.
constexpr float TEST_BENCH_PID_FLOOR_MAX_DISTANCE_MM = 1000.0f;
constexpr uint32_t TEST_BENCH_PID_LOG_PERIOD_MS = 200;
// Critères de réussite (échelon à 150 mm/s, au sol). Montée = temps pour atteindre 90 % de la consigne.
constexpr float TEST_BENCH_PID_RISE_FRACTION = 0.9f;
constexpr uint32_t TEST_BENCH_PID_RISE_TIME_MAX_MS = 300;
constexpr float TEST_BENCH_PID_OVERSHOOT_MAX_PCT = 15.0f;
constexpr float TEST_BENCH_PID_ERROR_MAX_MM_S = 5.0f;
// Au-delà, la correction P + I fait une trop grosse part du travail : l'anticipation est à mesurer.
constexpr float TEST_BENCH_PID_CORRECTION_HINT_PCT = 5.0f;

// Banc test_parcours : aller à un point tapé au clavier, comme en match, avec UN SEUL ultrason.
// Côté au démarrage du banc ; il se change ensuite avec « c g » ou « c d ».
// true = GAUCHE (bord de table à gauche du PAMI, D017) : le détour se fait par la DROITE du PAMI,
// vers le centre, et la douve (commande « d ») est à sa droite.
constexpr bool TEST_BENCH_PARCOURS_START_ON_LEFT = true;
// L'ultrason du banc : false = connecteur droit (TRIG 5 / ECHO 4), true = gauche (TRIG 25 / ECHO 35).
constexpr bool TEST_BENCH_PARCOURS_USE_LEFT_US = false;
constexpr uint32_t TEST_BENCH_PARCOURS_COUNTDOWN_MS = 10000;  // attente entre « p x y » et le départ
constexpr uint32_t TEST_BENCH_PARCOURS_MAX_RUN_MS = 30000;    // un trajet s'arrête seul après 30 s
constexpr float TEST_BENCH_PARCOURS_MAX_COORD_MM = 2000.0f;   // x et y tapés : entre -2000 et 2000 mm
constexpr uint32_t TEST_BENCH_PARCOURS_LOG_PERIOD_MS = 500;   // une ligne de suivi toutes les 0,5 s
// Après l'arrivée, on attend l'arrêt complet (glissement au freinage) avant d'afficher la position.
constexpr uint32_t TEST_BENCH_PARCOURS_SETTLE_MS = 500;

// Banc test_trajectoire : ligne, rotation et carré aux dimensions tapées au clavier, puis
// correction de l'étalonnage (diamètre des roues, entraxe) à partir des mesures au mètre ruban.
constexpr float TEST_BENCH_TRAJ_DEFAULT_DISTANCE_MM = 500.0f;
constexpr float TEST_BENCH_TRAJ_MIN_DISTANCE_MM = 50.0f;
constexpr float TEST_BENCH_TRAJ_MAX_DISTANCE_MM = 2000.0f;  // prévoir 2,5 m dégagés devant
// Un tour complet par défaut : l'erreur d'angle est 4 fois plus visible qu'à 90°.
constexpr float TEST_BENCH_TRAJ_DEFAULT_ANGLE_DEG = 360.0f;
constexpr float TEST_BENCH_TRAJ_MAX_ANGLE_DEG = 1080.0f;  // 3 tours
constexpr float TEST_BENCH_TRAJ_DEFAULT_SQUARE_MM = 300.0f;
constexpr float TEST_BENCH_TRAJ_MIN_SQUARE_MM = 100.0f;
constexpr float TEST_BENCH_TRAJ_MAX_SQUARE_MM = 1000.0f;
// Un essai s'arrête seul s'il dure plus de 2 fois le temps prévu, plus 5 s.
constexpr uint32_t TEST_BENCH_TRAJ_TIMEOUT_MARGIN_MS = 5000;
// Critères (repris de test_odometrie) : distance à 1 % près (5 mm sur 500 mm), angle à 5° près
// par tour, carré refermé à moins de 30 mm. En dessous, l'étalonnage est bon : rien à changer.
constexpr float TEST_BENCH_TRAJ_DISTANCE_TOLERANCE_PCT = 1.0f;
// Écart entre les roues accepté sans corriger WHEEL_DIAMETER_RIGHT_TO_LEFT_RATIO (commande « e ») :
// 0,2 % donne environ 9 mm sur une ligne de 950 mm, et 36 mm sur les 1900 mm de la ligne de la douve.
constexpr float TEST_BENCH_TRAJ_WHEEL_RATIO_TOLERANCE_PCT = 0.2f;
constexpr float TEST_BENCH_TRAJ_ANGLE_TOLERANCE_DEG_PER_TURN = 5.0f;
constexpr float TEST_BENCH_TRAJ_SQUARE_CLOSURE_MAX_MM = 30.0f;
// Au-delà, les essais répétés ne se ressemblent pas assez : patinage ou mécanique à régler
// avant de toucher aux constantes.
constexpr float TEST_BENCH_TRAJ_DISPERSION_MAX_PCT = 1.0f;

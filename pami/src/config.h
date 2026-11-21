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
// Broches du PCB « Test PCB 2026 5EO » (copie de docs/materiel.md, D015)
// ---------------------------------------------------------------------------
// Driver moteurs MDD3A : 2 entrées par moteur (D006).
constexpr uint8_t PIN_MOTOR_LEFT_A = 32;   // M1A
constexpr uint8_t PIN_MOTOR_LEFT_B = 33;   // M1B
constexpr uint8_t PIN_MOTOR_RIGHT_A = 19;  // M2A
constexpr uint8_t PIN_MOTOR_RIGHT_B = 18;  // M2B

// Encodeurs à effet Hall (« COD G B / COD G V » et « COD D B / COD D V » sur le pinout).
constexpr uint8_t PIN_ENCODER_LEFT_A = 25;
constexpr uint8_t PIN_ENCODER_LEFT_B = 26;
constexpr uint8_t PIN_ENCODER_RIGHT_A = 5;   // broche de démarrage : sans risque avec le pull-up
constexpr uint8_t PIN_ENCODER_RIGHT_B = 17;

// Deux ultrasons HC-SR04, à l'avant gauche et à l'avant droit (D012).
// Le PCB relie leurs deux TRIG à la même broche (D015) : ils émettent toujours en même temps.
// Chacun a sa propre broche ECHO.
constexpr uint8_t PIN_US_TRIG = 21;
constexpr uint8_t PIN_US_LEFT_ECHO = 2;    // broche de démarrage : ECHO est bas au repos, sans risque
constexpr uint8_t PIN_US_RIGHT_ECHO = 15;  // broche de démarrage : idem

// Interrupteur de côté (D014), avec résistance de tirage vers la masse (pull-down) :
// ouvert = état bas = départ à GAUCHE de la table ; fermé = état haut = départ à DROITE.
// Pas prévu sur le pinout du PCB : à câbler entre GPIO 27 et 3,3 V (D015).
// PROVISOIRE : vérifier sur le PAMI, puis marquer « G » et « D » à côté de l'interrupteur.
constexpr uint8_t PIN_SIDE_SELECTOR = 27;

// Départ : plus de bouton ni de tirette. Commande START_COMMAND tapée dans le moniteur (D013).

// Servo SG90 du bras (piloté par LEDC, D008).
constexpr uint8_t PIN_SERVO = 13;

// Accéléromètre I2C prévu sur le PCB. Pas encore utilisé (D004 : optionnel, plus tard).
constexpr uint8_t PIN_I2C_SDA = 22;
constexpr uint8_t PIN_I2C_SCL = 23;

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
// Gauche true / droite false : valeurs du premier PCB (D012), gardées pour le PCB 2026 5EO (D015).
// Le sens dépend du branchement des moteurs sur le driver, pas des broches de l'ESP32.
// À REVÉRIFIER avec le banc test_moteurs : chaque roue doit tourner vers l'avant (côté ultrasons).
constexpr bool MOTOR_LEFT_INVERTED = true;
constexpr bool MOTOR_RIGHT_INVERTED = false;
// PWM maximale autorisée, appliquée par motor_driver, même dans les bancs de test.
// Alimentation de labo à 6 V = tension nominale des FIT0450 : 100 %.
// À revoir avec la batterie : avec 8,4 V, il faudra environ 70 % pour rester vers 6 V en moyenne.
constexpr int MOTOR_MAX_PWM_PCT = 100;

// ---------------------------------------------------------------------------
// Encodeurs à effet Hall des FIT0450 (D004)
// ---------------------------------------------------------------------------
// 960 impulsions par tour de roue et par voie (fiche DFRobot). On compte les 2 fronts de la voie A :
// 1920 ticks par tour. PROVISOIRE : à vérifier en tournant une roue 10 tours à la main.
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
// Distance parcourue par la roue pour un tick : périmètre de la roue / ticks par tour.
constexpr float MM_PER_TICK = 3.14159265f * WHEEL_DIAMETER_MM / ENCODER_TICKS_PER_WHEEL_TURN;
// true depuis le premier PCB (D012), comme dans le programme de test de l'équipe.
// Sans risque si la sortie de l'encodeur est déjà en 0 / 3,3 V (mesure de l'équipe, montage d'avant) ;
// indispensable si le PCB laisse la sortie « en l'air » à l'état haut.
constexpr bool ENCODER_USE_INTERNAL_PULLUP = true;
// true si le compteur diminue quand on fait tourner la roue vers l'AVANT (côté ultrasons) à la main.
// On corrige ici plutôt que de recâbler.
// Gauche false / droite true depuis le 15/10/2026. Avec le PCB 2026 5EO (D015), l'ordre des voies
// A / B a pu changer : À REVÉRIFIER (banc test_odometrie, protocole 1).
constexpr bool ENCODER_LEFT_INVERTED = false;
constexpr bool ENCODER_RIGHT_INVERTED = true;

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
// Vitesse de croisière en ligne droite. Baissée de 200 à 150 mm/s pour les premiers réglages
// de l'odométrie au sol (13/10/2026) : moins de glissement, essais plus faciles à suivre.
// On l'augmentera ensuite si les missions ne tiennent pas dans les 9,5 s.
constexpr float DRIVE_SPEED_MM_S = 150.0f;
// Vitesse de chaque roue pendant une rotation sur place (baissée de 120 à 90 mm/s).
constexpr float TURN_SPEED_MM_S = 90.0f;
// Accélération et freinage : progressifs, pour ne pas patiner ni créer de pic de courant.
constexpr float MOTION_ACCEL_MM_S2 = 400.0f;
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
  MOAT            // va à la douve la plus proche (MOATS_MM), puis y reste, bras prêt (D014)
};
// La mission jouée. Pour en changer : modifier cette ligne, puis recompiler et téléverser.
// Ordre conseillé pour les tests : STRAIGHT_LINE, ROTATION, SQUARE, puis MOAT.
// Pas MOAT tant que MOATS_MM contient des positions de test.
constexpr MissionId ACTIVE_MISSION = MissionId::STRAIGHT_LINE;

// Il n'y a que 9,5 s de mouvement (de 5 s à 14,5 s) : chaque mission doit tenir dedans,
// avec de la marge pour un détour. PROVISOIRE : à ajuster d'après les vitesses réelles.
// Raccourcies pour les essais d'odométrie au sol (13/10/2026) : 1000 -> 500 mm, 250 -> 200 mm.
constexpr float STRAIGHT_LINE_LENGTH_MM = 250.0f;
constexpr float SQUARE_SIDE_MM = 200.0f;
// Mission douve (D014). Les 3 douves, dans le repère d'un départ à GAUCHE de la table :
// départ en (0, 0), x vers l'avant du PAMI au départ, y vers sa gauche (logic/odometry.h).
// Pour un départ à droite, les points sont mis en miroir (y -> -y) par forSide() : on ne les écrit
// qu'une fois (règle d'or n° 5).
// Le PAMI va à la douve la plus proche de son point de départ, puis y reste (étape DEFEND).
// Il compte s'il a une partie de lui dans une douve adverse à la fin du match (règles 2027, E.4) :
// visez un point assez à l'intérieur de la douve pour qu'une erreur d'odométrie de quelques
// centimètres ne l'en fasse pas sortir.
// PROVISOIRE : positions de test (la plus proche n'est volontairement pas la première de la liste).
// À remplacer par les vraies positions, mesurées sur le plan de la table.
struct MoatPoint {
  float xMm;
  float yMm;
};
constexpr MoatPoint MOATS_MM[] = {
    {1200.0f, 300.0f},  // douve 0
    {600.0f, -200.0f},  // douve 1 : la plus proche (632 mm)
    {900.0f, 0.0f},     // douve 2
};
constexpr uint8_t MOAT_COUNT = sizeof(MOATS_MM) / sizeof(MOATS_MM[0]);

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
constexpr uint16_t OBSTACLE_STOP_DISTANCE_MM = 150;
// L'obstacle n'est considéré parti qu'au-delà de seuil + 30 mm.
// Sans cette marge, une mesure qui oscille autour du seuil ferait avancer et freiner en boucle.
constexpr uint16_t OBSTACLE_CLEAR_MARGIN_MM = 30;

// ---------------------------------------------------------------------------
// Évitement (états AVOID_STOP et DETOUR)
// ---------------------------------------------------------------------------
// Niveau 1, arrêt : safety freine dès que l'ultrason voit un obstacle devant.
// Niveau 2, évite : obstacle toujours là après 1 s -> détour. Ce délai laisse à un robot qui passe
// le temps de s'en aller.
constexpr uint32_t AVOID_WAIT_MS = 1000;
// Le détour : quart de tour vers le centre de la table (à droite pour un départ à gauche, à gauche
// pour un départ à droite, D014), puis pas de côté.
constexpr float DETOUR_TURN_DEG = 90.0f;
constexpr float DETOUR_SIDE_MM = 200.0f;  // PROVISOIRE : un peu plus que la largeur d'un PAMI
// Niveau 3, contourne : après le détour, la mission vise de nouveau son point.
// Au-delà de 3 détours, on ne fait plus qu'attendre que la voie se libère (le temps est compté).
constexpr uint8_t AVOID_MAX_DETOURS = 3;

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
// Positions du bras. PROVISOIRES : à régler quand le bras sera monté.
constexpr float ARM_RETRACTED_DEG = 0.0f;  // replié, dans le périmètre du PAMI
constexpr float ARM_DEPLOYED_DEG = 90.0f;  // déployé vers l'avant
// Le bras se déploie quand l'ultrason voit l'adversaire à moins de 60 mm.
// Le bras ne peut dépasser que d'environ 50 mm (grille : périmètre déployé + 10 cm au plus).
// PROVISOIRE : à régler d'après la longueur réelle du bras.
constexpr uint16_t ARM_TRIGGER_DISTANCE_MM = 60;

// ---------------------------------------------------------------------------
// Interrupteur de côté (D014)
// ---------------------------------------------------------------------------
// Lu toutes les 100 ms avant le départ : un interrupteur ne change pas plus vite.
constexpr uint32_t SIDE_READ_PERIOD_MS = 100;

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
// Départ (D013) : taper ce mot dans le moniteur (Wi-Fi ou USB), puis Entrée. En minuscules.
// Un mot plutôt qu'une touche : une touche frappée par erreur ne lance pas le PAMI.
// ⚠ PROFIL LABO SEULEMENT : interdit à Eurobot (règlement général F.6). Là-bas, le PAMI démarre par
// un cordon de 50 cm, ou par le robot principal pendant le match.
constexpr char START_COMMAND[] = "go";

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
constexpr float TEST_BENCH_PID_MAX_SPEED_MM_S = 300.0f;   // FIT0450 sous 6 V : environ 400 mm/s au sol
constexpr uint32_t TEST_BENCH_PID_STEP_DURATION_MS = 3000;
constexpr uint32_t TEST_BENCH_PID_CONSTANT_DURATION_MS = 8000;  // le temps de freiner la roue au doigt
// Le régime établi (moyenne, erreur, ondulation) est mesuré après ce temps : avant, la roue accélère.
constexpr uint32_t TEST_BENCH_PID_SETTLE_MS = 1500;
// Escalier : 4 paliers, de quoi tracer la droite « PWM = zone morte + pente × vitesse ».
// Au sol : 1,5 s × (50 + 100 + 150 + 200) mm/s = 750 mm, sous la limite de 800 mm.
constexpr float TEST_BENCH_PID_STAIR_SPEEDS_MM_S[] = {50.0f, 100.0f, 150.0f, 200.0f};
constexpr uint32_t TEST_BENCH_PID_STAIR_DURATION_MS = 1500;
constexpr uint32_t TEST_BENCH_PID_STAIR_SETTLE_MS = 500;  // début de chaque palier ignoré
// Sécurité au sol (mode par défaut, sans évitement) : un essai s'arrête après 800 mm parcourus.
constexpr float TEST_BENCH_PID_FLOOR_MAX_DISTANCE_MM = 800.0f;
constexpr uint32_t TEST_BENCH_PID_LOG_PERIOD_MS = 200;
// Critères de réussite (échelon à 150 mm/s, au sol). Montée = temps pour atteindre 90 % de la consigne.
constexpr float TEST_BENCH_PID_RISE_FRACTION = 0.9f;
constexpr uint32_t TEST_BENCH_PID_RISE_TIME_MAX_MS = 300;
constexpr float TEST_BENCH_PID_OVERSHOOT_MAX_PCT = 15.0f;
constexpr float TEST_BENCH_PID_ERROR_MAX_MM_S = 5.0f;
// Au-delà, la correction P + I fait une trop grosse part du travail : l'anticipation est à mesurer.
constexpr float TEST_BENCH_PID_CORRECTION_HINT_PCT = 5.0f;

// Banc test_parcours : aller à un point tapé au clavier, comme en match, avec UN SEUL ultrason (GPIO 15).
// Côté de départ écrit en dur pour ce banc (le firmware de match, lui, lit l'interrupteur GPIO 27).
// true = départ à GAUCHE de la table : le détour se fait par la DROITE du PAMI, vers le centre.
constexpr bool TEST_BENCH_PARCOURS_START_ON_LEFT = true;
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
constexpr float TEST_BENCH_TRAJ_ANGLE_TOLERANCE_DEG_PER_TURN = 5.0f;
constexpr float TEST_BENCH_TRAJ_SQUARE_CLOSURE_MAX_MM = 30.0f;
// Au-delà, les essais répétés ne se ressemblent pas assez : patinage ou mécanique à régler
// avant de toucher aux constantes.
constexpr float TEST_BENCH_TRAJ_DISPERSION_MAX_PCT = 1.0f;

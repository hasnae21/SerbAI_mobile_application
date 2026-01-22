/*
 * ROBOT SUIVEUR DE LIGNE AVEC CONTRÔLE BLUETOOTH
 * Version Finale - Optimisée pour fonctionnement réel
 * 
 * Matériel:
 * - Arduino Mega 2560
 * - HC-05 Bluetooth (RX1/TX1)
 * - 3x TCRT5000 IR (A0=Gauche, A1=Milieu, A2=Droite)
 * - 1x HC-SR04 Ultrason (Trig=5, Echo=4)
 * - 2x Moteurs DC + L298N
 * - Batteries 11.1V (Lithium)
 * 
 * Auteur: AHOUZI Hasnae
 * Année: 2026 - RC
 */

// ========== PINS ==========
#define IR_L A0      // Capteur IR Gauche
#define IR_M A1      // Capteur IR Milieu
#define IR_R A2      // Capteur IR Droite
#define TRIG_PIN 5   // Ultrason Trigger
#define ECHO_PIN 4   // Ultrason Echo
#define ENA 9        // PWM Moteur Gauche
#define IN1 8        // Direction Moteur Gauche
#define IN2 7        // Direction Moteur Gauche
#define ENB 10       // PWM Moteur Droit
#define IN3 11       // Direction Moteur Droit
#define IN4 12       // Direction Moteur Droit
#define LED_PIN 13   // LED de statut

// ========== CONSTANTES ==========
const int SEUIL_NOIR = 650;           // Seuil détection ligne (AJUSTER selon environnement)
const int DISTANCE_OBSTACLE = 20;     // Distance min obstacle (cm)
const int VITESSE_LENTE = 80;         // Vitesse 1
const int VITESSE_MOYENNE = 120;      // Vitesse 2
const int VITESSE_RAPIDE = 160;       // Vitesse 3

// ========== VARIABLES D'ÉTAT ==========
enum Mode { ATTENTE, AUTO, MANUEL };
Mode modeActuel = ATTENTE;

bool obstacleDetecte = false;
int vitesseActuelle = VITESSE_MOYENNE;
int vitesseManuel = VITESSE_MOYENNE;
unsigned long dernierHeartbeat = 0;
unsigned long dernierDebugIR = 0;
unsigned long dernierScanObstacle = 0;
bool moteursActifs = false;

// ========== FONCTIONS MOTEURS ==========

void moteurAvancer(int v) {
  analogWrite(ENA, v);
  digitalWrite(IN1, HIGH);
  digitalWrite(IN2, LOW);
  analogWrite(ENB, v);
  digitalWrite(IN3, HIGH);
  digitalWrite(IN4, LOW);
  moteursActifs = true;
}

void moteurReculer(int v) {
  analogWrite(ENA, v);
  digitalWrite(IN1, LOW);
  digitalWrite(IN2, HIGH);
  analogWrite(ENB, v);
  digitalWrite(IN3, LOW);
  digitalWrite(IN4, HIGH);
  moteursActifs = true;
}

void moteurGauche() {
  // Moteur gauche recule, moteur droit avance
  analogWrite(ENA, 100);
  digitalWrite(IN1, LOW);
  digitalWrite(IN2, HIGH);
  analogWrite(ENB, 100);
  digitalWrite(IN3, HIGH);
  digitalWrite(IN4, LOW);
  moteursActifs = true;
}

void moteurDroite() {
  // Moteur gauche avance, moteur droit recule
  analogWrite(ENA, 100);
  digitalWrite(IN1, HIGH);
  digitalWrite(IN2, LOW);
  analogWrite(ENB, 100);
  digitalWrite(IN3, LOW);
  digitalWrite(IN4, HIGH);
  moteursActifs = true;
}

void moteurPivotGauche() {
  // Rotation sur place - les deux moteurs en sens inverse
  analogWrite(ENA, 100);
  digitalWrite(IN1, LOW);
  digitalWrite(IN2, HIGH);
  analogWrite(ENB, 100);
  digitalWrite(IN3, LOW);
  digitalWrite(IN4, HIGH);
  moteursActifs = true;
}

void moteurPivotDroite() {
  // Rotation sur place - les deux moteurs en sens inverse
  analogWrite(ENA, 100);
  digitalWrite(IN1, HIGH);
  digitalWrite(IN2, LOW);
  analogWrite(ENB, 100);
  digitalWrite(IN3, HIGH);
  digitalWrite(IN4, LOW);
  moteursActifs = true;
}

void moteurStop() {
  analogWrite(ENA, 0);
  analogWrite(ENB, 0);
  digitalWrite(IN1, LOW);
  digitalWrite(IN2, LOW);
  digitalWrite(IN3, LOW);
  digitalWrite(IN4, LOW);
  moteursActifs = false;
}

// ========== ULTRASON ==========

int mesurerDistance() {
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);
  
  long duree = pulseIn(ECHO_PIN, HIGH, 30000); // Timeout 30ms
  if (duree == 0) return 999; // Pas d'écho
  return duree * 0.034 / 2;
}

// ========== SUIVI DE LIGNE ==========

void suivreLigne() {
  int L = analogRead(IR_L);
  int M = analogRead(IR_M);
  int R = analogRead(IR_R);
  
  bool noirL = L > SEUIL_NOIR;
  bool noirM = M > SEUIL_NOIR;
  bool noirR = R > SEUIL_NOIR;
  
  // Debug IR toutes les 2 secondes
  if (millis() - dernierDebugIR > 2000) {
    Serial1.print("IR:");
    Serial1.print(L);
    Serial1.print(",");
    Serial1.print(M);
    Serial1.print(",");
    Serial1.println(R);
    dernierDebugIR = millis();
  }
  
  // Logique de suivi
  if (noirM) {
    // Sur la ligne - avancer tout droit
    moteurAvancer(vitesseActuelle);
  }
  else if (noirL && !noirR) {
    // Ligne à gauche - tourner à droite pour se recentrer
    moteurDroite();
    delay(50);
  }
  else if (noirR && !noirL) {
    // Ligne à droite - tourner à gauche pour se recentrer
    moteurGauche();
    delay(50);
  }
  else if (noirL && noirR && !noirM) {
    // Sur une intersection - continuer tout droit
    moteurAvancer(vitesseActuelle);
  }
  else if (!noirL && !noirM && !noirR) {
    // Ligne perdue - chercher
    moteurStop();
    delay(100);
    moteurDroite();
    delay(200);
  }
  else {
    // Cas par défaut
    moteurAvancer(vitesseActuelle * 0.8);
  }
}

// ========== TRAITEMENT COMMANDES ==========

void traiterCommande(char cmd) {
  switch (cmd) {
    case 'T': // Test
      digitalWrite(LED_PIN, HIGH);
      delay(200);
      digitalWrite(LED_PIN, LOW);
      delay(200);
      digitalWrite(LED_PIN, HIGH);
      delay(200);
      digitalWrite(LED_PIN, LOW);
      Serial1.println("TEST:OK");
      Serial1.println("STATUS:Robot fonctionnel");
      break;
      
    case 'A': // Mode Auto
      modeActuel = AUTO;
      digitalWrite(LED_PIN, HIGH);
      moteurStop();
      Serial1.println("MODE:AUTO");
      Serial1.println("STATUS:Suivi de ligne activé");
      break;
      
    case 'S': // Stop (mode attente)
      modeActuel = ATTENTE;
      moteurStop();
      digitalWrite(LED_PIN, LOW);
      Serial1.println("MODE:STOP");
      Serial1.println("STATUS:Robot en attente");
      break;
      
    case 'X': // Arrêt d'urgence
      modeActuel = ATTENTE;
      moteurStop();
      digitalWrite(LED_PIN, LOW);
      // Envoi multiple pour garantir réception
      Serial1.println("ALERTE:ARRET_URGENCE");
      Serial1.println("MODE:ARRET");
      Serial1.println("STATUS:ARRET_COMPLET");
      break;
      
    case 'M': // Mode Manuel
      modeActuel = MANUEL;
      moteurStop();
      digitalWrite(LED_PIN, HIGH);
      Serial1.println("MODE:MANUEL");
      Serial1.println("STATUS:Contrôle manuel actif");
      break;
      
    // === COMMANDES MANUELLES ===
    case 'F': // Avancer
      if (modeActuel == MANUEL) {
        moteurAvancer(vitesseManuel);
        Serial1.println("ACTION:AVANCE");
      }
      break;
      
    case 'B': // Reculer
      if (modeActuel == MANUEL) {
        moteurReculer(vitesseManuel);
        Serial1.println("ACTION:RECULE");
      }
      break;
      
    case 'L': // Tourner gauche
      if (modeActuel == MANUEL) {
        moteurGauche();
        Serial1.println("ACTION:GAUCHE");
      }
      break;
      
    case 'R': // Tourner droite
      if (modeActuel == MANUEL) {
        moteurDroite();
        Serial1.println("ACTION:DROITE");
      }
      break;
      
    case 'G': // Pivoter gauche (sur place)
      if (modeActuel == MANUEL) {
        moteurPivotGauche();
        Serial1.println("ACTION:PIVOTE_GAUCHE");
      }
      break;
      
    case 'D': // Pivoter droite (sur place)
      if (modeActuel == MANUEL) {
        moteurPivotDroite();
        Serial1.println("ACTION:PIVOTE_DROITE");
      }
      break;
      
    case '0': // Stop moteurs
      if (modeActuel == MANUEL) {
        moteurStop();
        Serial1.println("ACTION:STOP_MOTEUR");
      }
      break;
      
    case '1': // Vitesse lente
      vitesseManuel = VITESSE_LENTE;
      if (modeActuel == AUTO) vitesseActuelle = VITESSE_LENTE;
      Serial1.println("VITESSE:LENTE");
      break;
      
    case '2': // Vitesse moyenne
      vitesseManuel = VITESSE_MOYENNE;
      if (modeActuel == AUTO) vitesseActuelle = VITESSE_MOYENNE;
      Serial1.println("VITESSE:MOYENNE");
      break;
      
    case '3': // Vitesse rapide
      vitesseManuel = VITESSE_RAPIDE;
      if (modeActuel == AUTO) vitesseActuelle = VITESSE_RAPIDE;
      Serial1.println("VITESSE:RAPIDE");
      break;
      
    case '?': // Demander status complet
      envoyerStatus();
      break;
      
    default:
      Serial1.print("ERREUR:Commande_inconnue:");
      Serial1.println(cmd);
  }
}

void envoyerStatus() {
  Serial1.print("STATUS:");
  
  // Mode
  Serial1.print("Mode=");
  switch(modeActuel) {
    case ATTENTE: Serial1.print("ATTENTE"); break;
    case AUTO: Serial1.print("AUTO"); break;
    case MANUEL: Serial1.print("MANUEL"); break;
  }
  
  // État moteurs
  Serial1.print("|Moteurs=");
  Serial1.print(moteursActifs ? "ON" : "OFF");
  
  // Vitesse
  Serial1.print("|Vitesse=");
  if (modeActuel == MANUEL) {
    Serial1.print(vitesseManuel);
  } else {
    Serial1.print(vitesseActuelle);
  }
  
  // Obstacle
  Serial1.print("|Obstacle=");
  Serial1.println(obstacleDetecte ? "OUI" : "NON");
  
  // Capteurs IR
  Serial1.print("IR:");
  Serial1.print(analogRead(IR_L));
  Serial1.print(",");
  Serial1.print(analogRead(IR_M));
  Serial1.print(",");
  Serial1.println(analogRead(IR_R));
  
  // Distance
  int dist = mesurerDistance();
  Serial1.print("DIST:");
  Serial1.println(dist);
}

// ========== SETUP ==========

void setup() {
  // Initialisation Serial (pour debug via USB si nécessaire)
  // NOTE: Pas utilisé en fonctionnement normal
  Serial.begin(9600);
  
  // Serial1 pour Bluetooth (HC-05 sur RX1/TX1)
  Serial1.begin(9600);
  
  // Configuration pins
  pinMode(LED_PIN, OUTPUT);
  pinMode(ENA, OUTPUT);
  pinMode(ENB, OUTPUT);
  pinMode(IN1, OUTPUT);
  pinMode(IN2, OUTPUT);
  pinMode(IN3, OUTPUT);
  pinMode(IN4, OUTPUT);
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
  
  digitalWrite(TRIG_PIN, LOW);
  
  // État initial
  moteurStop();
  digitalWrite(LED_PIN, LOW);
  
  // Message de démarrage
  delay(1000); // Attendre que le HC-05 soit prêt
  
  Serial1.println("ROBOT:Initialisation...");
  
  // Séquence de test au démarrage
  digitalWrite(LED_PIN, HIGH);
  delay(300);
  
  // Test moteurs rapide
  moteurAvancer(80);
  delay(200);
  moteurStop();
  delay(200);
  moteurReculer(80);
  delay(200);
  moteurStop();
  
  digitalWrite(LED_PIN, LOW);
  delay(300);
  
  // Clignotement LED de confirmation
  for (int i = 0; i < 5; i++) {
    digitalWrite(LED_PIN, HIGH);
    delay(100);
    digitalWrite(LED_PIN, LOW);
    delay(100);
  }
  
  Serial1.println("ROBOT:PRET");
  Serial1.println("Commandes: T=Test, A=Auto, S=Stop, M=Manuel, ?=Status");
  Serial1.println("Manuel: F=Avant, B=Arriere, L=Gauche, R=Droite, G/D=Pivot, 0=Stop");
  Serial1.println("Vitesse: 1=Lent, 2=Moyen, 3=Rapide");
}

// ========== LOOP PRINCIPALE ==========

void loop() {
  // 1. Lire commandes Bluetooth
  if (Serial1.available()) {
    char cmd = Serial1.read();
    if (cmd != '\n' && cmd != '\r') {
      traiterCommande(cmd);
    }
  }
  
  // 2. Vérifier obstacle (toutes les 300ms)
  if (millis() - dernierScanObstacle > 300) {
    int distance = mesurerDistance();
    
    if (distance > 0 && distance < DISTANCE_OBSTACLE) {
      if (!obstacleDetecte) {
        obstacleDetecte = true;
        moteurStop();
        Serial1.println("ALERTE:OBSTACLE_DETECTE");
        Serial1.print("DIST:");
        Serial1.println(distance);
      }
    } else {
      if (obstacleDetecte) {
        obstacleDetecte = false;
        Serial1.println("STATUS:VOIE_LIBRE");
      }
    }
    
    dernierScanObstacle = millis();
  }
  
  // 3. Exécuter selon mode (si pas d'obstacle)
  if (!obstacleDetecte) {
    switch (modeActuel) {
      case AUTO:
        suivreLigne();
        delay(50);
        break;
        
      case MANUEL:
        // Les commandes manuelles sont gérées en temps réel
        delay(100);
        break;
        
      case ATTENTE:
        // Rien à faire - attente de commandes
        delay(200);
        break;
    }
  } else {
    // Obstacle détecté - maintenir arrêt
    delay(100);
  }
  
  // 4. Heartbeat - envoyer status périodiquement
  if (millis() - dernierHeartbeat > 5000) {
    if (modeActuel != ATTENTE) {
      // Envoyer distance actuelle
      int dist = mesurerDistance();
      Serial1.print("DIST:");
      Serial1.println(dist);
      
      // Envoyer IR
      Serial1.print("IR:");
      Serial1.print(analogRead(IR_L));
      Serial1.print(",");
      Serial1.print(analogRead(IR_M));
      Serial1.print(",");
      Serial1.println(analogRead(IR_R));
    }
    dernierHeartbeat = millis();
  }
}
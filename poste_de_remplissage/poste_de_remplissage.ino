/**
 * e-mail : openprogramming23@gmail.com
 * gitHub : https://github.com/RS-malik-el
 *
 * @AUTEUR : Exaucé KIMBEMBE / @OpenProgramming
 * DATE : 07/12/2023
 * 
 * @Board : Arduino
 * 
 * @Outils:
 * 2 Capteurs ultrason hc-sr04
 * 1 Capteur d'obstacle infrarouge
 * 1 Bouton poussoir
 * 1 Servomoteur
 * 1 Moto-pompe
 * 1 Moto-reducteur (entraînement du tapis)
 * 1 Driver L298N
*/

#ifndef __AVR__
	#error "Selectionner une carte arduino"
#endif 

#include <Servo.h>

// Pins utilisés
#define BOUTON      2 // AttachInterrupt
#define IR_CAPTEUR  3 // Capteur de fin de course tapis
#define PIN_SERVO   4 // Pin connecté au servomoteur
#define MOTO_POMPE  5 // PWM
#define TAPIS       6 // PWM
// Pin capteur ultrason 1 (placé de le poste de remplissage)
#define TRIG_PIN_1  7
#define ECHO_PIN_1  8
// Pin capteur ultrason 2 (Placé de le poste de bouchonnages)
#define TRIG_PIN_2  9
#define ECHO_PIN_2  10

// Constantes
#define ATTENTE     10    // Attente après chaque incrémentation(servomoteur)
#define DISTANCE 	  4     // Distance de detection maximum en cm
#define DETEC_IR    false // Valeur de retour du capteur infrarouge lors de la détection
#define DETEC_US  	true  // Valeur de retour du capteur ultrason lors de la détection
#define ANG_SORTIE 	180   // Angle servomoteur (sortie) 
#define ANG_ENTREE 	80    // Angle servomoteur (entrée)
#define V_TAPIS     70    // Vitesse du tapis en %
#define V2_TAPIS    45    // Vitesse du tapis en % en direction de poste de bouchonnage
#define V_MOTOPOMPE 50    // Vitesse de la motopompe en %
#define PAUSE       2000  // Temps de pause pour permettre au pot de quitté le poste

bool state_tapis = false;// Etat du tapis
bool ir_stop = false;// Permet de redemarrer automatiquement le tapis

Servo servo; // Objet servomoteur

// Prototypes des fonctions
void tapis_ON(const uint8_t v_tapis=V_TAPIS){analogWrite(TAPIS,map(v_tapis,0,100,0,255));state_tapis=true;} // Permet de mettre en marche le tapis
void tapis_OFF(void){analogWrite(TAPIS,0);state_tapis=false;}// Permet d'arrêter le tapis
void gestion_Tapis(void);// Appeler dans la fonction attachInterrupt lors de l'appui du bouton
void gestion_Remplissage(void);// permet le remplissage des pots
void gestion_fin_de_course(void);// Permet d'arrêter automatiquement le tapis
void gestion_Bouchonnage(void); // Permet de gérer la sortie et l'entrée du servomoteur
bool detection(const uint8_t trigPin, const uint8_t echoPin); // detecte si l'objet est près du capteur
void attente(void);// Temps de pause pour permettre au pot de quitté le poste
bool detectionAppui(uint8_t pin=BOUTON);// Fonction permettant de détecter un appui sur un bouton

void setup(){
	// Configuration du servomoteur
	servo.attach(PIN_SERVO);
	servo.write(ANG_ENTREE);

	//Configuration capteur ultrason
	pinMode(TRIG_PIN_1, OUTPUT);
	pinMode(TRIG_PIN_2, OUTPUT);
	pinMode(ECHO_PIN_1, INPUT);
	pinMode(ECHO_PIN_2, INPUT);

	pinMode(TAPIS, OUTPUT);
	pinMode(IR_CAPTEUR, INPUT);
	pinMode(MOTO_POMPE, OUTPUT);
	pinMode(BOUTON, INPUT_PULLUP);
}

void loop(){
	// Vérification de l'appui du bouton
	if(detectionAppui()==true and digitalRead(IR_CAPTEUR)== not DETEC_IR)
		gestion_Tapis();

	if (state_tapis==true or ir_stop==true){
		gestion_fin_de_course();
		// Détection du pot devant le poste de remplissage
		if(detection(TRIG_PIN_1, ECHO_PIN_1)==true)
			gestion_Remplissage();
		// Détection du pot devant le poste de bouchonnage
		if(detection(TRIG_PIN_2, ECHO_PIN_2)==true)
		//	gestion_Remplissage();
			gestion_Bouchonnage();
	}
}

// Fonction permettant de détecter un appui sur un bouton
bool detectionAppui(uint8_t pin){
	if(not digitalRead(pin)==true){
		delay(10);// Attente après chaque appui
		while(not digitalRead(pin)==true){}// Si appui maintenant on ne fait rien
		if(not digitalRead(pin)==false){
			return true;
		}
	}
	return false;
}

void gestion_Tapis(void){
	if(state_tapis==true)
		tapis_OFF();
	else
		tapis_ON();
}

void gestion_Remplissage(void){
	delay(150);
	tapis_OFF(); // Arrêt du tapis

	// Remplissage de la boite
	analogWrite(MOTO_POMPE,map(V_MOTOPOMPE,0,100,0,255));
	delay(5000);
	analogWrite(MOTO_POMPE,0);
	delay(1000);
	
	// Redemarrage du tapis
	if(digitalRead(IR_CAPTEUR)== not DETEC_IR and detection(TRIG_PIN_2, ECHO_PIN_2)== not DETEC_US){
		tapis_ON(V2_TAPIS);
		attente();
	}
}

void gestion_Bouchonnage(){
	tapis_OFF(); // Arrêt du tapis
	// Sortie du servomoteur
	for(int i = servo.read(); i <= ANG_SORTIE; ++i){
		servo.write(i);
		delay(ATTENTE);
	}
	// Entrée du servomoteur
	for(int i = servo.read(); i >= ANG_ENTREE; --i){
		servo.write(i);
		delay(ATTENTE);
	}
	// Redemarrage du tapis
	if(digitalRead(IR_CAPTEUR)== not DETEC_IR and detection(TRIG_PIN_1, ECHO_PIN_1)== not DETEC_US){
		tapis_ON();
		attente();
	}
}

bool detection(const uint8_t trigPin, const uint8_t echoPin){
  unsigned long duration = 0;
  unsigned long distance = 0;

  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);
  
  // Lit la broche echoPin, renvoie le temps de parcours de l'onde sonore en microsecondes
  duration = pulseIn(echoPin, HIGH); 
  distance = duration * 0.034 / 2;
  
  if(distance <= DISTANCE)
  	return true;
  else
  	return false;
}

void gestion_fin_de_course(void){
	// Arrêt du tapis
	if(digitalRead(IR_CAPTEUR)== DETEC_IR and ir_stop==false){
		tapis_OFF();
		ir_stop = true;
	}
	// Redemarrage du tapis
	else if(digitalRead(IR_CAPTEUR)== not DETEC_IR and ir_stop==true){
		tapis_ON();
		ir_stop = false;
	}
}

void attente(void){
	unsigned long init = millis();
	while(digitalRead(IR_CAPTEUR)== not DETEC_IR and detection(TRIG_PIN_1, ECHO_PIN_1)==not DETEC_US or 
				digitalRead(IR_CAPTEUR)== not DETEC_IR and detection(TRIG_PIN_2, ECHO_PIN_2)==not DETEC_US){
		if((millis()-init)>=PAUSE)
			break;
		delay(2);
	}
}

#include <Controllino.h>
#include <EEPROM.h>
#include <avr/wdt.h>

#define PUMP1_PIN      CONTROLLINO_D2
#define PUMP2_PIN      CONTROLLINO_D3
#define PURGE1_PIN     CONTROLLINO_D0
#define PURGE2_PIN     CONTROLLINO_D1

#define PRESSURE1_PIN  CONTROLLINO_A0
#define PRESSURE2_PIN  CONTROLLINO_A1

// EEPROM ADDRESS
#define I_ADDR           0x00
#define NB_PUMP1_ADDR    0x01
#define NB_PUMP2_ADDR    0x05
#define HOUR_PUMP1_ADDR  0x09
#define HOUR_PUMP2_ADDR  0x0D

uint8_t currentStep = 0;

// Sorties
bool pump1 = false;
bool pump2 = false;
bool purge1 = false;
bool purge2 = false;

// Entrées
bool pressure1 = false;
bool pressure2 = false;

// Variables
uint8_t i = 0;
bool resetCycle = false;
uint32_t nbPump1 = 0;
uint32_t nbPump2 = 0;
uint32_t hourPump1 = 0;
uint32_t hourPump2 = 0;
uint32_t pumpTime = 0;

bool nonBlockingDelay(uint8_t id, uint16_t delay_ms) {
    static int16_t currentId = -1;
    static uint32_t startTime = 0;

    if (id != currentId) {
        currentId = id;
        startTime = millis();
    }

    if (millis() - startTime >= delay_ms) {
        return true;
    } else {
        return false;
    }
}

void oneTimePrint(String text) {
    static String textToPrint = "";

    if (text != textToPrint) {
        textToPrint = text;
        Serial.println(textToPrint);
    }
}

void steps() {
    // Étapes
    switch (currentStep) {
        case 0:
            // Conditions initiales (Aucune)
            if (!pressure1 && !pressure2) {
                currentStep = 3;
            } else {
                currentStep = 1;
            }
            break;
        case 1:
            pump1 = true;
            
            if (nonBlockingDelay(1, 2000)) {
                // Tempo 1 (2s)
                currentStep = 2;
            }
            break;
        case 2:
            pump1 = true;
            pump2 = true;

            if (!pressure1 && !pressure2) {
                // Réservoires remplis
                currentStep = 3;
            }
            break;
        case 3:
            purge1 = true;

            if (nonBlockingDelay(2, 500)) {
                // Tempo 2 (500ms)
                currentStep = 4;
            }
            break;
        case 4:
            purge1 = true;
            purge2 = true;

            if (nonBlockingDelay(3, 60000)) {
                // Tempo 3 (60s)
                currentStep = 5;
            }
            break;
        case 5:
            if (pressure1 || pressure2) {
                currentStep = 6;
            }
            break;
        case 6:
            i++;
            currentStep = 7;
            break;
        case 7:
            if (i < 5) {
                // Étape 7
                pump1 = true;
            } else {
                // Étape 7'
                pump2 = true;
            }

            if (nonBlockingDelay(4, 2000) && pressure1 && pressure2) {
                // Étape 7''
                // Tempo 4 (2s)
                pump1 = true;
                pump2 = true;
            }

            if (!pressure1 && !pressure2) {
                // Réservoires remplis
                if (i < 10) {
                    currentStep = 5;
                } else {
                    currentStep = 8;
                }

                // Sauvegarde pour les statistiques
                if (i < 5) {
                    nbPump1++;
                    EEPROM.put(NB_PUMP1_ADDR, nbPump1);
                } else {
                    nbPump2++;
                    EEPROM.put(NB_PUMP2_ADDR, nbPump2);
                }
                
                EEPROM.put(I_ADDR, i);
            }
            break;
        case 8:
            purge1 = true;
            
            if (nonBlockingDelay(5, 500)) {
                // Tempo 5 (500ms)
                currentStep = 9;
            }
            break;
        case 9:
            purge1 = true;
            purge2 = true;

            i = 0;

            if (nonBlockingDelay(6, 15000)) {
                // Tempo 6 (15s)
                currentStep = 5;
            }
            break;
    }
}

void outputs() {
    digitalWrite(PUMP1_PIN, pump1);
    digitalWrite(PUMP2_PIN, pump2);
    digitalWrite(PURGE1_PIN, purge1);
    digitalWrite(PURGE2_PIN, purge2);
}

void inputs() {
    pressure1 = digitalRead(PRESSURE1_PIN);
    pressure2 = digitalRead(PRESSURE2_PIN);
}

void reset() {
    // À chaque cycle, remet les valeurs des entrées et sorties à zéro
    
    // Sorties
    pump1 = false;
    pump2 = false;
    purge1 = false;
    purge2 = false;
    
    // Entrées
    pressure1 = false;
    pressure2 = false;

    // Reset watchdog
    wdt_reset();

    // Si à 3h du matin le boitier n'a pas été éteint, redémarre le cycle
    if (!resetCycle && (uint8_t) Controllino_GetHour() == 3) {
        currentStep = 0;
        resetCycle = true;
    } else if ((uint8_t) Controllino_GetHour() != 3) {
        resetCycle = false;
    }
}

void setup() {
    // put your setup code here, to run once:
    Serial.begin(9600);
    Controllino_RTC_init();

    Serial.println("CONTROLLINO MINI");
    delay(500);
    Serial.println("starting...");
    Serial.println();

    // Controllino_SetTimeDateStrings(__DATE__, __TIME__); /* set compilation time to the RTC chip */
    
    Serial.print("Hour: ");
    Serial.print((uint8_t) Controllino_GetHour());
    Serial.print(" Minute: "); 
    Serial.println((uint8_t) Controllino_GetMinute());
    Serial.println();

    Serial.println("Reading EEPROM...");
    
    EEPROM.get(I_ADDR, i);
    EEPROM.get(NB_PUMP1_ADDR, nbPump1);
    EEPROM.get(NB_PUMP2_ADDR, nbPump2);
    EEPROM.get(HOUR_PUMP1_ADDR, hourPump1);
    EEPROM.get(HOUR_PUMP2_ADDR, hourPump2);

    Serial.print("I value: "); Serial.println(i);
    Serial.println("Statistics:");
    Serial.print("NB Pump1: ");   Serial.println(nbPump1);
    Serial.print("NB Pump2: ");   Serial.println(nbPump2);
    Serial.print("HOUR Pump1: "); Serial.println(hourPump1);
    Serial.print("HOUR Pump2: "); Serial.println(hourPump2);

    // setup pin
    pinMode(PUMP1_PIN, OUTPUT);
    pinMode(PUMP2_PIN, OUTPUT);
    pinMode(PURGE1_PIN, OUTPUT);
    pinMode(PURGE2_PIN, OUTPUT);

    pinMode(PRESSURE1_PIN, INPUT);
    pinMode(PRESSURE2_PIN, INPUT);
    delay(2000);

    Serial.println("Watchdog enabled (250ms)");
    Serial.println();
    delay(500);
    
    // Activation du watchdog (250ms)
    wdt_enable(WDTO_250MS);
}

void loop() {
    // put your main code here, to run repeatedly:
    oneTimePrint("Current step: " + String(currentStep));

    inputs();
    steps();
    outputs();
    
    reset();
}

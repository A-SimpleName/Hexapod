#include "SoftwareSerial.h"

/* 
  RX/TX zuordnen
  RX -> 2, TX -> 3
  BTSerial -> Name der Schnittstelle
*/
SoftwareSerial BTSerial(2, 3);

int LED = 7;

// über Bluetooth vom Smartphone gesendeter Wert
char btDaten;

void setup()
{
  BTSerial.begin(9600);
  Serial.begin(9600);

  // auf serielle Verbindung warten
  while (!Serial);
  delay(1000);
 
  pinMode(LED, OUTPUT);
  
  // Menü im Seriellen Monitor
  Serial.println("1 -> LED an");
  Serial.println("0 -> LED aus");
}

void loop()
{
  // vom Smartphone gesendeten Wert lesen
  btDaten = BTSerial.read();
  if (btDaten == '1')
  {
    // Meldung in der App anzeigen
    BTSerial.print("LED an");

    // Meldung im Seriellen Monitor anzeigen
    Serial.println("LED an");

    digitalWrite(LED, HIGH);
  }

  else if (btDaten == '0')
  {
    BTSerial.print("LED aus");
    Serial.println("LED aus");
    digitalWrite(LED, LOW);
  }
}

/*
  Smart Trash bin

*/


#include <SoftwareSerial.h>
SoftwareSerial gprsSerial(5, 6);

String idTrash = "e-0001";    // Set the smart bin ID
String idCollector;

#define cardSensor 2


/************************************* RFID SENSOR *************************************/

#include <SPI.h>
#include <MFRC522.h>

#define SS_PIN 10
#define RST_PIN 9
 
MFRC522 rfid(SS_PIN, RST_PIN); // Instance of the class

MFRC522::MIFARE_Key key; 

// Init array that will store new NUID 
String readCard[4];
bool sender=1;
bool cardDect;

/************************************* LEVEL SENSOR *************************************/

#define trigPin 7
#define echoPin 8 

long duration; // variable for the duration of sound wave travel
int distance, level; // variable for the distance measurement

/************************************* WEIGHT SENSOR *************************************/

#include "HX711.h"

float calibration_factor= -93000.0; //This value is obtained using the SparkFun_HX711_Calibration sketch

#define LOADCELL_DOUT_PIN  4
#define LOADCELL_SCK_PIN  3

HX711 scale;

int weight;

void setup()
{  
  gprsSerial.begin(9600);
  Serial.begin(9600);

  /************************************* GSM MODULE *************************************/

  Serial.println("Config SIM900...");
  delay(2000);
  Serial.println("Done!...");
  gprsSerial.flush();
  Serial.flush();

  // attach or detach from GPRS service 
  gprsSerial.println("AT+CGATT?");
  delay(100);
  toSerial();

  // bearer settings
  gprsSerial.println("AT+SAPBR=3,1,\"CONTYPE\",\"GPRS\"");
  delay(2000);
  toSerial();

  // bearer settings
  gprsSerial.println("AT+SAPBR=3,1,\"APN\",\"airtelgprs.com\"");
  delay(2000);
  toSerial();
  delay(2000);

  // bearer settings
  gprsSerial.println("AT+SAPBR=1,1");
  delay(2000);
  toSerial();

  // bearer settings
  gprsSerial.println("AT+SAPBR=2,1");
  delay(2000);
  toSerial();
  
  /************************************* RFID SENSOR *************************************/

  SPI.begin(); // Init SPI bus
  rfid.PCD_Init(); // Init MFRC522 

  for (byte i = 0; i < 6; i++) {
    key.keyByte[i] = 0xFF;
  }

  pinMode(cardSensor, INPUT);

  /************************************* LEVEL SENSOR *************************************/

  pinMode(trigPin, OUTPUT); // Sets the trigPin as an OUTPUT
  pinMode(echoPin, INPUT); // Sets the echoPin as an INPUT

  /************************************* WEIGHT SENSOR *************************************/
  
  scale.begin(LOADCELL_DOUT_PIN, LOADCELL_SCK_PIN);
  scale.set_scale(calibration_factor); //This value is obtained by using the SparkFun_HX711_Calibration sketch
  scale.tare(); //Assuming there is no weight on the scale at start up, reset the scale to 0

  /***************************************** BEGINING ***************************************/
 
}

void loop()
{
  // Reset the loop if no new card present on the sensor/reader. This saves the entire process when idle.
  
  cardDect=digitalRead(cardSensor);
  
  if ((cardDect==1) && (sender==0)) {
    communication(1);
    sender=1;
    Serial.println("Collection data sending OK!");
  }
  
  else if (cardDect==1 && (sender==1)) {
    weightSensor();
    levelSensor();
    communication(0);
//    Serial.print("Id Trash bin: ");
//    Serial.println(idTrash);
//    Serial.print("Level= ");
//    Serial.print(level);
//    Serial.println("%");
//    Serial.print("Weight= ");
//    Serial.print(weight);
//    Serial.println("kg");
    Serial.println("Normal update sending OK!");
  }

  getUid();
  communication(0);
  delay(5000);
}

/************************************* GSM MODULE *************************************/

void toSerial()
{
  while(gprsSerial.available()!=0)
  {
    Serial.write(gprsSerial.read());
  }
}

void communication(bool contr)  //controller 0 for monitoring 1 for collection
{
  // url construction
  
  String site = "e-trash.jovalys.com"; //web address to where the trash bin sends the data
  
  String controller;
  if (!contr) controller = "newHistoric.php?idTrash=" + idTrash + "&level=" + String(level) + "&weight=" + String(weight);
  else controller = "newScan.php?idWorker=" + idCollector + "&idTrash=" + idTrash;
  
  // initialize http service
  
  gprsSerial.println("AT+HTTPINIT");
  delay(2000); 
  toSerial();
  
  // set http param value
  
  gprsSerial.println("AT+HTTPPARA=\"URL\",\"" + site + "/controllers/" + controller +"\"");
  delay(5000);
  toSerial();
  
  // set http action type 0 = GET, 1 = POST, 2 = HEAD
  
  gprsSerial.println("AT+HTTPACTION=0");
  delay(6000);
  toSerial();
  
  // read server response
  
  gprsSerial.println("AT+HTTPREAD"); 
  delay(1000);
  toSerial();
//  delay(2000);
  
  gprsSerial.println("");
  gprsSerial.println("AT+HTTPTERM");
  toSerial();
  delay(300);
  
  gprsSerial.println("");  
}

/************************************* LEVEL SENSOR *************************************/

void levelSensor()
{
  // Clears the trigPin condition
  
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  
  // Sets the trigPin HIGH (ACTIVE) for 10 microseconds
  
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);
  
  // Reads the echoPin, returns the sound wave travel time in microseconds
  
  duration = pulseIn(echoPin, HIGH);
  
  // Calculating the distance
  
  distance = duration * 0.034/ 2;  // Speed of sound wave divided by 2 (go and back)
  level = 100-(distance*5.55);      // 5.56 is found by divising 100/18 in order to convert the level in % 

}

/************************************* WEIGHT SENSOR *************************************/

void weightSensor() {
  weight = round(scale.get_units());
}

/************************************* RFID SENSOR *************************************/

void getUid() 
{
  if ( ! rfid.PICC_IsNewCardPresent()) {
    //Serial.println("No Card detected!!");
    return;
  }
  
  // Verify if the NUID has been readed
  if ( ! rfid.PICC_ReadCardSerial())
    return;

  // Store NUID into nuidPICC array
  
  Serial.println("Collection in progress OK!");
  for (byte i = 0; i < 4; i++) {
    readCard[i] = String(rfid.uid.uidByte[i], HEX);
    }
  idCollector=(readCard[0]+readCard[1]+readCard[2]+readCard[3]);
  sender=0;
  Serial.println(idCollector);  

  delay(2000);
  
}

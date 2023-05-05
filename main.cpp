#include <Arduino.h>
#include <esp_now.h>
#include <SPI.h>
#include <MFRC522.h>
#include <WiFi.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <ArduinoJson.h>
#include <PubSubClient.h>


#define SS_PIN  5 
#define RST_PIN 27 
#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels
byte bufferATQA[2];
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);
MFRC522 rfid(SS_PIN, RST_PIN);
int chargecount = 0;
const size_t bufferSize = JSON_OBJECT_SIZE(4) + 30;
bool charging_toggle = false; //bestemmer at den skal begynne å lade når data blir tilsendt
unsigned long old_millis;
unsigned long beginning_chargetime;
int elapsed_chargetime;
float charge_ratio = 0.4; // antall sekund det tar for å lade 1 %
String sendt_status = "";
bool Toggle = false;
typedef struct OutgoingData{ // dataen som sendes til skyen
  String uid; // Sender "ID" en til bilen som skal lades
  byte finnishedstate; // Sender om bilen er ferdig ladet eller ikke
  uint8_t current_charge; // mengden ladning som er i bilen nå
  uint8_t chargetime_left; // tiden som gjenstår til full ladning
} OutgoingData;

typedef struct IncomingData{ // dataen som sendes ifra skyen
  uint8_t wanted_charge; // mengden ladning som ønskes
  uint8_t old_charge; // mengden ladning som er i bilen fra før av
} IncomingData;

IncomingData skydata;
OutgoingData bildata;
WiFiClient espClient;
PubSubClient client(espClient);

const char* ssid = "Nokia";
const char* password = "12345678";
const char* mqtt_server = "192.168.137.209";

void callback(char* topic, byte* message, unsigned int length) {
  String jsonString = "";
  for (int i = 0; i < length; i++) {
    jsonString += (char)message[i];
  }
  StaticJsonDocument<200> doc;
  DeserializationError error = deserializeJson(doc, jsonString);
  if (error) {
    Serial.print("deserializeJson() failed: ");
    Serial.println(error.c_str());
    return;
  }
  skydata.wanted_charge = doc["wanted_charge"].as<uint8_t>();
  skydata.old_charge = doc["old_charge"].as<uint8_t>();
  charging_toggle = true; // starter ladning
  beginning_chargetime = millis(); // starer ladning start-tid
  Serial.println(skydata.wanted_charge);
  Serial.println(skydata.old_charge);
  }


bool GetCarID(){ // Denne funskjonen fungerer ikke enda, må fikses!!!!!!!!
  if (!rfid.PICC_IsNewCardPresent()) { //returnerer false hvis det ikke er scannet en "bil"
    return false;
  }
  if (!rfid.PICC_ReadCardSerial()) { //returnerer false hvis dataen ikke kan leses
    return false;
  }
    bildata.uid = "";
    for (int i = 0; i < rfid.uid.size; i++) {
          bildata.uid.concat(String(rfid.uid.uidByte[i], HEX)); // legger UID dataen inn i bildata.uid
    }
    
    skydata.wanted_charge = 90;
    skydata.old_charge = 23;
    charging_toggle = true;
    bildata.uid.toUpperCase(); // gir alt store bokstaver
    rfid.PICC_HaltA(); //stopper lesing av rfid
    Serial.println(bildata.uid);
    delay(1000);
    return true;  //returnerer true hvis et kort er lest
}

void send_data(){
//char outgoingJsonString[bufferSize];
StaticJsonDocument<256> outgoingJsonDoc;
outgoingJsonDoc["UID"] = bildata.uid;
outgoingJsonDoc["finished_state"] = bildata.finnishedstate;
outgoingJsonDoc["current_charge"] = bildata.current_charge;
outgoingJsonDoc["charge_time_left"] = bildata.chargetime_left;
outgoingJsonDoc["charge_count"] = chargecount;
char buffer[256];
serializeJson(outgoingJsonDoc, buffer);
client.publish("ladestasjon", buffer);
Serial.println(buffer);
Serial.println("Sender data");
delay(1000);
}


void CarUpdate(){
  if(millis()-old_millis >= 500){
    old_millis = millis();
    GetCarID(); // hvis GetCarID er false er det ikke en bil der og UID en settes til null klar for ny bil
    //send_data(); // sender data til skyen HVIS bil er oppdaget
  }
}

void ChargeUpdate(){ 
  elapsed_chargetime = ((millis()-beginning_chargetime)); // ser på hvor lang den totale ladetiden er
  bildata.current_charge = skydata.old_charge + round(elapsed_chargetime/(charge_ratio*1000)); //regner ut hvor mye som er ladet til nå
  bildata.chargetime_left = (skydata.wanted_charge-bildata.current_charge)*charge_ratio; // regner ut tid igjen
  if(bildata.current_charge>=skydata.wanted_charge){ // sjekker om ladningen er ferdig og stopper ladningen
  if(skydata.wanted_charge > 0){ 
    charging_toggle = false;
    bildata.current_charge = skydata.wanted_charge; // Gjør nåværende ladning til ønsket ladning i tilfelle den skulle overstige ønsket
    bildata.finnishedstate = 1;
    chargecount +=1;
    bildata.uid = "";
    send_data(); // Sender data til mqtt
  }
  }
  send_data();
}

void DisplayUpdate(){ //oppdatarer displayet med data ifra globale variabler
  display.clearDisplay();
  display.setCursor(0,0);
  display.print("Wanted charge: ");
  display.print(skydata.wanted_charge);
  display.println("%");
  display.setCursor(0,15);
  display.print("charge: ");
  display.print(bildata.current_charge);
  display.println("%");
  display.setCursor(0,30);
  display.print("Tid igjen:  ");
  display.print(bildata.chargetime_left);
  display.println("  sek");
  display.setCursor(0,45);
  display.print("Curent UID:  ");
  display.print(bildata.uid);
  display.display();
}


void setup() {
  Serial.begin(115200);
  SPI.begin(); 
  rfid.PCD_Init(); 
  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) { 
    Serial.println(F("SSD1306 allocation failed"));
    for(;;);}
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(WHITE);
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);

  old_millis = millis();
  Serial.println("Ready!");
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
}

void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Attempt to connect
    if (client.connect("ESP8266Client")) {
      Serial.println("connected");
      // Subscribe
      client.subscribe("ladestasjon");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}


void loop() {
   //if (!client.connected()) {
  //  reconnect();
  //}
  //client.loop();
  CarUpdate(); // sjekker om det er en bil til stede 
  if(charging_toggle){
    ChargeUpdate(); 
  }
  DisplayUpdate(); 
}






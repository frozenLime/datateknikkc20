//Including the necesarry libraries
#include <esp_now.h>
#include <WiFi.h>
#include <ArduinoJson.h>

//A string holding the accepted RFID card Ids.
String approvedUID = ("7EE0A7D");

//Variables setting the varibles for the three different timers
unsigned long workStart = 0;
unsigned long workEnd = 0;
unsigned long workTime = 0;
unsigned long schoolStart = 0;
unsigned long schoolEnd = 0;
unsigned long schoolTime = 0;
unsigned long lapTimerStart = 0;
unsigned long lapTimerEnd = 0;
unsigned long lapTime = 0;

//ID and state for web override of the access ESPs
uint8_t overrideESP;
uint8_t overrideRfid;

//MAC adresses for the two ESPs who is to recieve access signals
uint8_t esp2[] = {0x30, 0xC6, 0xF7, 0x1F, 0xD6, 0x84};
uint8_t esp4[] = {0x78, 0xE3, 0x6D, 0x08, 0xFC, 0xF8};

//Setting the pins for UART communication
const int RXD1 = 26;
const int TXD1 = 22;

//Local variables for temprarily storing the recieved values from sensors and RFID nodes
uint8_t id;
uint8_t senderID;
String schoolCard;
String workID;
bool sensorMotion;
float sensorTemperature; 
float sensorHumidity;
float sensorLightLevel;

//Numbers for solar power production
float solarEfficiency = 0.022;
float energyProduction = 0; 
float totalEnergyProduction = 0;

//Numbers for earnings based on ESP node
float moneyEarned = 0;
float salary = 7;

//Structr for the ESPnow communication with the slave nodes
typedef struct struct_message{
    uint8_t id;
    String cardUID;
    uint8_t access;
    bool sensorMotion;
    float sensorTemperature; 
    float sensorHumidity;
    float sensorLightLevel;
}

//Structing the needed messages
struct_message;
struct_message transferData;
struct_message myData;

//Setting ESP peer info as peerInfo
esp_now_peer_info_t peerInfo;

//When data is recieved via ESPnow set the local variables to the value of the recieved variables and call the sender check function
void OnDataRecv(const uint8_t * mac, const uint8_t *incomingData, int len) {
  memcpy(&myData, incomingData, sizeof(myData));
  senderID = 0;
  senderID = myData.id;
  schoolCard = myData.cardUID;
  workID = myData.cardUID;
  sensorMotion = myData.sensorMotion;
  sensorTemperature = myData.sensorTemperature; 
  sensorHumidity = myData.sensorHumidity;
  sensorLightLevel = myData.sensorLightLevel;
  senderCheck();
}

//Function for checking the right action based on which node the data is recieved from
void senderCheck(){
  if (senderID == 1){
    sensorReadings();
  }

  if (senderID == 2){
    accessCheckSchool();      
  }

  if (senderID == 3){
    accessCheckWork();
  }
  else{
    return;
  }
}

//Function for processing sensordata from sensornode
void sensorReadings(){
    //generate a JSONdocument to store data
    DynamicJsonDocument transmitJson(256);
    //if the ambient light is low, there will not be any solar power production
    if (sensorLightLevel <= 20){
      energyProduction = 0;
      return;
    }
    //energyproduction is = to the ambient light * with the "efficiency" of the solar panel
    energyProduction = (sensorLightLevel * solarEfficiency);
    totalEnergyProduction += energyProduction;

    //if the sensor node detects motion interact with laptimer
    if (sensorMotion == true){
      //if the timer is at 0, start the timer
      if (lapTimerStart == 0) {
        lapTimerStart = millis();  
        return;
      }
      //if the timer != stop the timer and store the time since start. Convert to seconds and set motion to false
      lapTimerEnd = millis();
      lapTime += lapTimerEnd - lapTimerStart;
      lapTimerStart = 0;
      lapTime = lapTime/1000;
      sensorMotion = false;
  }
  //store the variables in the JSON doc
    transmitJson["id"] = senderID;
    transmitJson["temperature"] = sensorTemperature;
    transmitJson["humidity"] = sensorHumidity;
    transmitJson["lightLevel"] = sensorLightLevel;
    transmitJson["energyProduction"] = energyProduction;
    transmitJson["totalEnergyProduction"] = totalEnergyProduction;
    transmitJson["lapTime"] = lapTime;
    //send the JSONdoc over UART to the MQTTESP
    serializeJson(transmitJson, Serial1);
    serializeJsonPretty(transmitJson, Serial);   
}

//Check if access is allowed based on RFID reading at the "work node" 
void accessCheckWork(){
  //generate a JSONdoc
  DynamicJsonDocument transmitJson(256);
  //store the senderESP and RFID card id in the JSON doc
  transmitJson["id"] = senderID;
  transmitJson["uid"] = workID;

  //if the UID of th RFID card is not = to the stored accepted cards
  if (workID != approvedUID){ 
    //delete the temp UID storage
    workID.clear();  
    //set access "bool" to 0
    transferData.access = 0;
    //store the same access bool in the JSONdoc as well.
    transmitJson["access"] = transferData.access;
    // Send message via ESP-NOW
    esp_now_send(esp4, (uint8_t *) &transferData, sizeof(transferData));
    //send the JSONdoc thru UART to the MQTTnode 
    serializeJson(transmitJson, Serial1);
    return;
  }
  //else set access to (if the UID matches)
  transferData.access = 1;
  //store the same access bool in the JSONdoc as well. 
  transmitJson["access"] = transferData.access;
  // Send message via ESP-NOW
  esp_now_send(esp4, (uint8_t *) &transferData, sizeof(transferData));
  //if the work timer is at 0
  if (workStart == 0) {
    //set start time to current millis
    workStart = millis(); 
    //store the access bool in the JSONdoc again
    transmitJson["access"] = transferData.access;
    //Send the JSONdoc to the MQTT node via UART
    serializeJson(transmitJson, Serial1);
    return; 
  }

  //set the work end time to now millis, subtract the start time, convert to min, reset the timer
  workEnd = millis();
  workTime += workEnd - workStart;
  workStart = 0;
  workTime = workTime/60000;

  //earnings = salary (pr. minute) * worktime (minutes)
  moneyEarned =+ (workTime*salary);
  //delete the temp. stored UID
  workID.clear();
  //store earned money in the JSONdoc, along with work time
  transmitJson["earnings"] = moneyEarned;
  transmitJson["workTime"] = workTime;
  //send the JSONdoc to the MQTTnode
  serializeJson(transmitJson, Serial1);   
}

//Check if access is allowed based on RFID reading at the "school node" 
void accessCheckSchool(){
  //generate a JSONdoc
  DynamicJsonDocument transmitJson(256);
  //store the senderESP and RFID card id in the JSONdoc
  transmitJson["id"] = senderID;
  transmitJson["uid"] = schoolCard;
  //If the recieved RFID UID is != to the stored UID
  if (schoolCard != approvedUID){ 
    //delete the temp. UID storage
    schoolCard.clear();  
    //set the struct acces "bool" to 0 (no access)!, and store the same value in a JSONdoc
    transferData.access = 0;
    transmitJson["access"] = transferData.access;
    // end the message via ESPnow
    esp_now_send(esp2, (uint8_t *) &transferData, sizeof(transferData));
    //send the JSONdoc to the MQTTnode
    serializeJson(transmitJson, Serial1);
    return;
  }
  //set the access bool to "1" (access ok!) and store the same value in a JSONdoc
  transferData.access = 1;
  transmitJson["access"] = transferData.access;
  // Send the message via ESPnow
  esp_now_send(esp2, (uint8_t *) &transferData, sizeof(transferData));
  // Check if the card belongs to the worker
  if (schoolStart == 0) {
    // Worker is punching in
    schoolStart = millis();
    //clear the temp. storage of RFID card UID
    schoolCard.clear(); 
    //set the access bool to "1" (access ok!) and store the same value in a JSONdoc
    transmitJson["access"] = transferData.access;
    //send the JSONdoc over UART to the MQTT node
    serializeJson(transmitJson, Serial1);
    return;   
  }

  //Set the end time of the timer to current millis
  schoolEnd = millis();
  //calculate the time, reset the start time and convert millis to minutes
  schoolTime += schoolEnd - schoolStart;
  schoolStart = 0;
  schoolTime = schoolTime/60000;
  //clear the temp. RFID card UID 
  schoolCard.clear(); 
  //store the time value in th JSONdoc
  transmitJson["schooltime"] = schoolTime;
  //send the JSONdoc over UART to the MQTT node
  serializeJson(transmitJson, Serial1); 
}

//if weboverride is called
void schoolWebOverride(){
  //if override RFID is 1, send 1 to the "lock" via ESPnow to unlock and display message
  if(overrideRfid == 1){
    transferData.access = 1;
    esp_now_send(esp2, (uint8_t *) &transferData, sizeof(transferData));
  }
  else{
    //else, send 0 to the "lock" via ESPnow to display a message and sound the buzzer
    transferData.access = 0;
    esp_now_send(esp2, (uint8_t *) &transferData, sizeof(transferData));
  }
  //reset the overrideESP "adress"
  overrideESP = 0;
}

// if web override is called
void workWebOverride(){
  //if override RFID is 1, send 1 to the "lock" via ESPnow to unlock and display message
  if(overrideRfid == 1){
      transferData.access = 1;
      esp_now_send(esp4, (uint8_t *) &transferData, sizeof(transferData));
    }
    else{
      //else, send 0 to the "lock" via ESPnow to display a message and sound the buzzer
      transferData.access = 0;
      esp_now_send(esp4, (uint8_t *) &transferData, sizeof(transferData));
    }
    //reset the overrideESP "adress"
    overrideESP = 0;
}
// }
 

void setup() {
  //Start serial communication
  Serial.begin(115200);
  //Begin UART communication on the set pins
  Serial1.begin(115200, SERIAL_8N1, RXD1, TXD1);

  // Set device as a Wi-Fi Station
  WiFi.mode(WIFI_STA);

  // Init ESP-NOW
  if (esp_now_init() != ESP_OK) {
    return;
  }
    //Register "school node" for ESPnow
    memcpy(peerInfo.peer_addr, esp2, 6);
  if (esp_now_add_peer(&peerInfo) != ESP_OK){
    return;
  }
  //Register "work node" for ESPnow
  memcpy(peerInfo.peer_addr, esp4, 6);
  if (esp_now_add_peer(&peerInfo) != ESP_OK){
    return;
  }
  //Call the function to recieve ESPnow data
  esp_now_register_recv_cb(OnDataRecv);
}
 
void loop() {
  //If the UART communication port is open for communication
   if (Serial1.available() > 0){
     //store the incoming UART data in a string
    String incoming_data = Serial1.readString();
    //generate a JSONdoc
    StaticJsonDocument<200> temp;
    //deserialize the incoming data
    deserializeJson(temp, incoming_data);
    //store the recieved id and access variables as local variables
    overrideESP = temp["id"];
    overrideRfid = temp["access"];

    //check which esp to override (1 or 2), then go to the function corresponding
    if(overrideESP == 1){
      schoolWebOverride();
    }
    if(overrideESP == 2){
      workWebOverride();
    }
  }
}

//Including all used libraries
#include <Arduino.h>
#include <ESP_now.h>
#include <WiFi.h>
#include <Adafruit_AHTX0.h>

//Setting this esp id to 3
uint8_t espID = 1;

//I/O ports used by sensors
const int ultraSonicTx = 18;
const int ultraSonicRx = 19;
const int climateTx = 21;
const int climateRx = 22;
const int lightRx = 35;

//Variables for storing sensor values and previous values
float humidityValue;
float tempValue;
bool motion;
bool dark;
unsigned long duration;
float distance;
float previousHumidity;
float previousTemp;
float lightValue;
float previousLightValue;

//Variables for calculation distance to object (US sensor) 
float soundSpeed = 0.034;
float usTimeDelay = 800;
const unsigned long ultrasonicDelayShort = 0.002;
const unsigned long ultrasonicDelayLong = 0.01;
const unsigned long ultrasonicWait = 1;

//Previous time for millis counter
unsigned long previousTime = 0;


//Initalising the AHT sensor
Adafruit_AHTX0 aht;

//Setting the reciever ESPs MAC adress 
uint8_t broadcastAddress[] = {0xE8, 0x31, 0xCD, 0xE6, 0x39, 0x94};

//Setting up ESP now
esp_now_peer_info_t peerInfo;


void setup(){
    //Start the AHT sensor
    aht.begin();
    //Set WIFI mode to stationary
    WiFi.mode(WIFI_STA);
    //Setting the MAC adress, channel and encryption of ESPnow
    memcpy(peerInfo.peer_addr, broadcastAddress, 6);
    peerInfo.channel = 0;
    peerInfo.encrypt = false;

    esp_now_init();

    if (esp_now_add_peer(&peerInfo) != ESP_OK){
        return;
    }

    //Setting pinmodes
    pinMode(ultraSonicTx, OUTPUT);
    pinMode(ultraSonicRx, INPUT);
    pinMode(climateTx, OUTPUT);
    pinMode(climateRx, INPUT);
    pinMode(lightRx, INPUT);

    //Sensing the I2C (AHT) temp sensor and reads the values to store as previous values
    sensors_event_t humidity, temp;
    aht.getEvent(&humidity, &temp);
    previousTemp = temp.temperature;
    previousHumidity = humidity.relative_humidity;


}

//Defining and structing a message for ESP now communication
typedef struct struct_message{
    uint8_t id;
    String cardUID;
    uint8_t access;
    bool sensorMotion;
    float sensorTemperature; 
    float sensorHumidity;
    float sensorLightLevel;
}
struct_message;
struct_message transferData;


//Function "to do motion detection"
void movementDetection(){
    transmit();
}

//Function "check temp"
void weatherChange(){
  transmit();
}

//Function "if the ambient lux change"
void daylight(){
    transmit();
}

//Function for setting the struct equal to the local variables and send them via ESPnow
void transmit(){
    transferData.id = espID;
    transferData.sensorMotion = motion;
    transferData.sensorLightLevel = lightValue;
    transferData.sensorTemperature = tempValue;
    transferData.sensorHumidity = humidityValue;
    esp_err_t result = esp_now_send(broadcastAddress, (uint8_t *) &transferData, sizeof(transferData));
}


void loop(){
    sensors_event_t humidity, temp;
    aht.getEvent(&humidity, &temp);
    tempValue = temp.temperature;
    humidityValue = humidity.relative_humidity;

    //If temp or humidity goes beyond thresholds, transmit the values
    if ((tempValue < (previousTemp - 0.5))||(tempValue > (previousTemp + 0.5))||(humidityValue < (previousHumidity - 1))||(humidityValue> (previousHumidity + 1))){
      previousTemp = tempValue;
      previousHumidity = humidityValue;;
      weatherChange();
    }

    //Accessing the function to check the "light conditions and solar production(external calculation)"
    lightValue = analogRead(lightRx);
    lightValue = map(lightValue, 0, 4095, 0, 100);
    if ((lightValue < (previousLightValue - 3))||(lightValue > (previousLightValue + 3))){
      previousLightValue = lightValue;
      daylight();
    }

    
  //Set current millis and save to variable
  unsigned long currentTime = millis();
    //Set ultrasonic to off
    digitalWrite(ultraSonicTx, LOW);
    //Send ultrasonic pulse if time has passed the delay
    if (currentTime - previousTime >= ultrasonicDelayShort) {
         digitalWrite(ultraSonicTx, HIGH);
         previousTime = currentTime;
    }
    
    //Set ultrasonic pulse to low if timer has passed
    if (currentTime - previousTime >= ultrasonicDelayLong) {
         digitalWrite(ultraSonicTx, LOW);
         previousTime = currentTime;
    }

    //Read the duration since pulse sendt based on pulse in function
    duration = pulseIn(ultraSonicRx, HIGH);
    
    //Calculate the distance to the object based on the duration and the sound of speed/2
    distance = duration * (soundSpeed/2);

    //If the time was longer than the delay, set the current time as previous time 
    if (currentTime - previousTime >= usTimeDelay) {
    previousTime = currentTime; 

  }
    //Cheking if the distance measured is over 2cm (min) and under 20 cm (maximum), then trigger movement detection
    if((distance < 20)||(distance >2)){
        motion = true;
        movementDetection();
      }
    else{
       motion = false;
    }
}

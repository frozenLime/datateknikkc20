//Including the used libraries
#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>

//Setting the UART ports
const int RXD1 = 26;
const int TXD1 = 22;

//WiFi information
const char* ssid = "Nokia";
const char* password = "12345678";

//Info for MQTT server
const char* mqtt_server = "192.168.137.195";
const int mqtt_port = 1883;

//Topics to communicate on
const char* subscribe_topic = "control";
const char* publish_topic = "fullmessage";


// WiFi client (setting up WiFi)
WiFiClient wifi_client;

// Setting up MQTT clietn
PubSubClient mqtt_client(wifi_client);

void setup() {
  //Serial communications begin
  Serial.begin(115200);

  //Begin UART communication
  Serial1.begin(115200, SERIAL_8N1, RXD1, TXD1);
  
  //Connect to WiFi, print error message if unsuccessfull
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }
  Serial.println("Connected to WiFi");

  // Connect to MQTT broker, print connecting messages and print error message if failed
  mqtt_client.setServer(mqtt_server, mqtt_port);
  mqtt_client.setCallback(mqtt_callback);
  while (!mqtt_client.connected()) {
    Serial.println("Connecting to MQTT broker...");
    if (mqtt_client.connect("Arduino")) {
      Serial.println("Connected to MQTT broker");
      mqtt_client.subscribe(subscribe_topic);
    } else {
      Serial.print("MQTT connection failed, rc=");
      Serial.println(mqtt_client.state());
      delay(1000);
    }
  }
}

void loop() {
  // Check for incoming MQTTmessages
  mqtt_client.loop();

  //If serial is available
  if (Serial1.available() > 0) {
    //generate JSONdoc
    DynamicJsonDocument transmitJson(256);
    //desialize the incoming JSONdoc
    deserializeJson(transmitJson, Serial1);
    //create a buffer
    char sender[256];
    //serialize the message to the buffer
    serializeJson(transmitJson, sender);
    //publish the buffer to the MQTT topic
    mqtt_client.publish(publish_topic, sender);
  }
}

void mqtt_callback(char* topic, byte* payload, unsigned int length) {
  // Handle incoming MQTT message
  Serial.print("Received message on topic: ");
  Serial.println(topic);
  String message = "";
  for (int i = 0; i < length; i++) {
    message += (char)payload[i];
  }
  Serial1.println(message);
}


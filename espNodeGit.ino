#include <PubSubClient.h>
#include "DHT.h"
#include "WiFiEspAT.h"

// Uncomment one of the lines bellow for whatever DHT sensor type you're using!
#define DHTTYPE DHT11   // DHT 11
//#define DHTTYPE DHT21   // DHT 21 (AM2301)
//#define DHTTYPE DHT22   // DHT 22  (AM2302), AM2321

// Change the credentials below, so your ESP8266 connects to your router
const char* ssid = "-------------";
const char* password = "----------";

// Change the variable to your Raspberry Pi IP address, so it connects to your MQTT broker
const char* mqtt_server = "----------------";

// Emulate Serial1 on pins 6/7 if not present
#ifndef HAVE_HWSERIAL1
#include "SoftwareSerial.h"
SoftwareSerial Serial1(6, 7); // RX, TX
#endif

// Initializes the espClient. You should change the espClient name if you have multiple ESPs running in your home automation system
//WiFiEspClient espClient;
WiFiClient espClient;
PubSubClient client(espClient);

// DHT Sensor - GPIO 5 = D1 on ESP-12E NodeMCU board
const int DHTPin = 8;

// Lamp - LED - GPIO 4 = D2 on ESP-12E NodeMCU board
//const int lamp = 4;
//#define lamp 4

int Water_sensor_level;

int Motion_sensor = 0;


#define SIGNAL_PIN A5

#define SENSOR_MIN 200

#define SENSOR_MAX 560

int value = 0; // variable to store the sensor value
int level = 0; // variable to store the water level

const int MOTION_SENSOR_PIN = 11;   // Arduino pin connected to the OUTPUT pin of motion sensor
int motionStateCurrent      = LOW; // current  state of motion sensor's pin
int motionStatePrevious     = LOW; // previous state of motion sensor's pin



#define water_pump A1

// Initialize DHT sensor.
DHT dht(DHTPin, DHTTYPE);

// Timers auxiliar variables
long now = millis();
long lastMeasure = 0;

// Don't change the function below. This functions connects your ESP8266 to your router
void setup_wifi() {
  delay(10);
  // We start by connecting to a WiFi network
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.print("WiFi connected - ESP IP address: ");
  Serial.println(WiFi.localIP());
}

// This functions is executed when some device publishes a message to a topic that your ESP8266 is subscribed to
// Change the function below to add logic to your program, so when a device publishes a message to a topic that 
// your ESP8266 is subscribed you can actually do something
void callback(char* topic, byte* message, unsigned int length) {
  Serial.print("Message arrived on topic: ");
  Serial.print(topic);
  Serial.print(". Message: ");
  String messageTemp;
  
  for (int i = 0; i < length; i++) {
    //Serial.print((char)message[i]);
    messageTemp += (char)message[i];
  }
  Serial.println(messageTemp);
  //Serial.println();
  // Feel free to add more if statements to control more GPIOs with MQTT

  ;/*// If a message is received on the topic room/lamp, you check if the message is either on or off. Turns the lamp GPIO according to the message
  if(strcmp(topic, "room/lamp")==0){
      Serial.print("Changing Room lamp to ");
      if(messageTemp == "on"){
        digitalWrite(lamp, HIGH);
        Serial.println("on");
      }
      else if(messageTemp == "off"){
        digitalWrite(lamp, LOW);
        Serial.println("off");
      }
  }*/
  if(strcmp(topic, "esp/pump")==0){
    Serial.print("Changing Water pump to ");
      if(messageTemp == "on"){
        digitalWrite(water_pump, LOW);
        Serial.println("on");
      }
      else if(messageTemp == "off"){
        digitalWrite(water_pump, HIGH);
        Serial.println("off");
      }
  }
  //Serial.println();
}

// This functions reconnects your ESP8266 to your MQTT broker
// Change the function below if you want to subscribe to more topics with your ESP8266 
void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Attempt to connect
    /*
     YOU MIGHT NEED TO CHANGE THIS LINE, IF YOU'RE HAVING PROBLEMS WITH MQTT MULTIPLE CONNECTIONS
     To change the ESP device ID, you will have to give a new name to the ESP8266.
     Here's how it looks:
       if (client.connect("ESP8266Client")) {
     You can do it like this:
       if (client.connect("ESP1_Office")) {
     Then, for the other ESP:
       if (client.connect("ESP2_Garage")) {
      That should solve your MQTT multiple connections problem
    */
    if (client.connect("ESP8266Client")) {
      Serial.println("connected");  
      // Subscribe or resubscribe to a topic
      // You can subscribe to more topics (to control more LEDs in this example)
      client.subscribe("room/lamp");
      client.subscribe("esp/pump");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

// The setup function sets your ESP GPIOs to Outputs, starts the serial communication at a baud rate of 115200
// Sets your mqtt broker and sets the callback function
// The callback function is what receives messages and actually controls the LEDs
void setup() {
  //pinMode(lamp, OUTPUT);
  
  dht.begin();
  pinMode(MOTION_SENSOR_PIN, INPUT);
  
  // initialize serial for debugging
  Serial.begin(115200);
  // initialize serial for ESP module
  Serial1.begin(9600);
  // initialize ESP module
  WiFi.init(&Serial1);
  setup_wifi();
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);
  digitalWrite(water_pump, HIGH);
  pinMode(water_pump, OUTPUT);

}

// For this project, you don't need to change anything in the loop function. Basically it ensures that you ESP is connected to your broker
void loop() {
  
  if (!client.connected()) {
    reconnect();
  }
  if(!client.loop())
    client.connect("ESP8266Client");

  now = millis();
  // Publishes new temperature and humidity every 30 seconds

  if (now - lastMeasure > 30000) {
    lastMeasure = now;
    client.loop();
    delay(100);
    value = analogRead(SIGNAL_PIN); // read the analog value from sensor
    level = map(value, SENSOR_MIN, SENSOR_MAX, 0, 4); // 10 levels
    if(level < 0){
      level = 0;
    }
    else if(level > 4){
      level  = 4;
    }
    static char waterLevel[7];
    dtostrf(level, 6, 0, waterLevel);
    delay(1000);

    motionStatePrevious = motionStateCurrent;             // store old state
    motionStateCurrent  = digitalRead(MOTION_SENSOR_PIN); // read new state

    static char motionState[7];

    if (motionStatePrevious == LOW && motionStateCurrent == HIGH) { // pin state change: LOW -> HIGH
      Serial.println("Motion detected!");
      Motion_sensor = 1;
    }
    else
    if (motionStatePrevious == HIGH && motionStateCurrent == LOW) { // pin state change: HIGH -> LOW
      Serial.println("Motion stopped!");
      Motion_sensor = 0;
    }
    dtostrf(Motion_sensor, 6, 0, motionState);

    // Sensor readings may also be up to 2 seconds 'old' (its a very slow sensor)
    float h = dht.readHumidity();
    // Read temperature as Celsius (the default)
    float t = dht.readTemperature();
    // Read temperature as Fahrenheit (isFahrenheit = true)
    float f = dht.readTemperature(true);

    // Check if any reads failed and exit early (to try again).
    if (isnan(h) || isnan(t) || isnan(f)) {
      Serial.println("Failed to read from DHT sensor!");
      return;
    }

    // Computes temperature values in Celsius
    float hic = dht.computeHeatIndex(t, h, false);
    static char temperatureTemp[7];
    dtostrf(t, 6, 2, temperatureTemp);
    
    // Uncomment to compute temperature values in Fahrenheit 
    // float hif = dht.computeHeatIndex(f, h);
    // static char temperatureTemp[7];
    // dtostrf(hif, 6, 2, temperatureTemp);

     if(level < 2){
      //Serial.println("Water Pump: on");
      digitalWrite(water_pump, LOW);
      delay(10000);
      //Serial.println("Water Pump: off");
      digitalWrite(water_pump, HIGH);
    }
    
    static char humidityTemp[7];
    dtostrf(h, 6, 2, humidityTemp);

    // Publishes Temperature and Humidity values
    client.publish("esp/temp", temperatureTemp);
    client.publish("esp/humidity", humidityTemp);
    client.publish("esp/waterlevel", waterLevel);
    if(Motion_sensor == 0){
      client.publish("esp/motion", "Não detectada");
    }
    else if(Motion_sensor == 1){
      client.publish("esp/motion", "Detectada");
    }
    client.publish("esp/motion_chart", motionState);
    
    Serial.print("Humidity: ");
    Serial.print(h);
    Serial.print(" % Temperature: ");
    Serial.print(t);
    Serial.print(" *C ");
    Serial.print(f);
    Serial.print(" *F Heat index: ");
    Serial.print(hic);
    Serial.print(" *C ");
    Serial.print("Water Level (Value): ");
    Serial.println(value);
  }
}
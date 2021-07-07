/*Header Files*/
#include "Def.h"
#include "Var.h"

/*Variables*/
int temperature; 
int humidity;
int soilMoisture;
int moisturePercentage;

/*Variables*/
int temperatureLowerLimit;
int temperatureUpperLimit;
int humidityLowerLimit;
int humidityUpperLimit;
int waterLowerLimit;
boolean motorSwitch = false;
boolean motorAutoMode = false;

#define DhtType DHT11
DHT dht(D1,DhtType); // initial DHT sensor 

const char* ssid = "SLT FIBER";         //Your Wifi router name
const char* password = "0112747861";    //your wifi password

const char* mqtt_server = "test.mosquitto.org"; 
const char* outTopic = "ENTC/EN2560/out/zeagro";
const char* inTopic = "ENTC/EN2560/in/zeagro";

/*Requirments for ESp8266*/
WiFiClient espClient;
PubSubClient client(espClient);
unsigned long lastMsg = 0;
#define MSG_BUFFER_SIZE	(100)
char msg[MSG_BUFFER_SIZE];
int value = 0;

void setup_wifi() {
  delay(10);
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  randomSeed(micros());

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");

  /*Reading incoming message*/
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  }
  
  Serial.println();

  /*Convert incoming message to JSON*/
  
  DynamicJsonDocument doc(1024);
  deserializeJson(doc, payload);

/*Assigning incoming values from user*/
  temperatureLowerLimit = doc["tL"];
  temperatureUpperLimit = doc["tU"];
  humidityLowerLimit = doc["hL"];
  humidityUpperLimit = doc["hU"];
  waterLowerLimit = doc["wL"];
  motorSwitch = doc["mS"];
  motorAutoMode = doc["mA"];
 
}

void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Create a random client ID
    String clientId = "ESP8266Client-";
    clientId += String(random(0xffff), HEX);
    // Attempt to connect
    if (client.connect(clientId.c_str())) {
      Serial.println("connected");
      client.subscribe(inTopic);
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      delay(5000);                                // Wait 5 seconds before retrying
    }
  }
}

void setup() {
  pinMode(temHumDigital,INPUT);
  pinMode(soilAnalog,INPUT);
  pinMode(relayAnalog,OUTPUT);
  pinMode(tempLED,OUTPUT);
  pinMode(humLED,OUTPUT);
  pinMode(moistureLED,OUTPUT);    
  Serial.begin(115200);
  dht.begin();
  setup_wifi();
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);
}

void loop() {
 
  if (!client.connected()) {
    reconnect();
  }
  client.loop();

/*Reading Sensor Values*/
  temperature = readSensorTemperature(temperatureLowerLimit,temperatureUpperLimit);
  humidity = readSensorHumidity(humidityLowerLimit,humidityUpperLimit);
  delay(100);
  soilMoisture = readSensorWaterLevel(waterLowerLimit, motorAutoMode, motorSwitch);
  moisturePercentage = (100 - ((soilMoisture/1023.00)*100));// put your main code here, to run repeatedly
  delay(100);

/*Creating outgoing message*/
  String message = "{\"temp\": "+String(temperature)+",\"hum\": "+String(humidity)+",\"water\": "+String(80)+"}";

/*Sending message*/
  unsigned long now = millis();
  if (now - lastMsg > 5000) {
    lastMsg = now;
    snprintf (msg, MSG_BUFFER_SIZE,message.c_str());
    Serial.print("Publish message: ");
    Serial.println(msg);
    client.publish(outTopic,msg);
  }
}

/*Reaading water level*/
int readSensorWaterLevel(int lowerLimit, boolean isAutoMode, boolean isSwitch) {
  int soilVal = analogRead(soilAnalog);
  float moisturePercentage = (100 - ((soilVal/1023.00)*100));
  delay(100);

/*Set warning outputs*/
  if((lowerLimit > moisturePercentage && isAutoMode==1) || isSwitch==1){
    digitalWrite(relayAnalog,HIGH);  //relay analog pin high
    digitalWrite(moistureLED,HIGH);
    delay(300);
  }
  else{
    digitalWrite(relayAnalog,LOW);
    digitalWrite(moistureLED,LOW);
    delay(300);
  }
  return soilVal;
}

/*Reading temperature*/
int readSensorTemperature(int lowerLimit, int upperLimit) {
  int temp = dht.readTemperature();

/*Set warning outputs*/
  if (lowerLimit > temp || temp > upperLimit){
    digitalWrite(tempLED,HIGH);
    delay(300);
    digitalWrite(tempLED,LOW);
    delay(300);
    Serial.println("exceed"+String(temp));
  }
  else{
    digitalWrite(tempLED,LOW);
    delay(300);
    Serial.println("normal"+String(temp));
  }
  return temp;

}

/*Reading humidity*/
int readSensorHumidity(int lowerLimit, int upperLimit){
  int hum = dht.readHumidity();

/*Set warning outputs*/
  if (lowerLimit < hum && hum < upperLimit){
    digitalWrite(humLED,LOW);
    delay(300);
  }
  else{
    digitalWrite(humLED,HIGH);
    delay(300);
    digitalWrite(humLED,LOW);
    delay(300);
  }
  return hum;
}







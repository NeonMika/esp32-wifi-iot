#include <WiFi.h>
#include <PubSubClient.h>
#include <OneWire.h>
#include <DallasTemperature.h>

// Define to which pin of the Arduino the 1-Wire bus is connected:
#define ONE_WIRE_BUS_PIN_1 32
#define ONE_WIRE_BUS_PIN_2 33
#define ONE_WIRE_BUS_PIN_3 25
#define ONE_WIRE_BUS_PIN_4 26
#define ONE_WIRE_BUS_PIN_5 27

#define RED_LED_PIN 22
#define GREEN_LED_PIN 23

OneWire oneWire1(ONE_WIRE_BUS_PIN_1);
OneWire oneWire2(ONE_WIRE_BUS_PIN_2);
OneWire oneWire3(ONE_WIRE_BUS_PIN_3);
OneWire oneWire4(ONE_WIRE_BUS_PIN_4);
OneWire oneWire5(ONE_WIRE_BUS_PIN_5);

DallasTemperature sensors1(&oneWire1);
DallasTemperature sensors2(&oneWire2);
DallasTemperature sensors3(&oneWire3);
DallasTemperature sensors4(&oneWire4);
DallasTemperature sensors5(&oneWire5);

const char* ssid = "Panic at the Cisco";
const char* password =  "...";

int measurements = 0;

// MQTT
WiFiClient wifiClient;
PubSubClient mqttClient(wifiClient);
const char *mqttBroker = "192.168.8.XX";
const int mqttPort = 1883;
char clientId[100] = "esp32/sensor-client/";
// const char *mqtt_username = "...";
// const char *mqtt_password = "...";

void setup() {
  // Begin serial communication at a baud rate of 9600:
  Serial.begin(9600);
  while (!Serial) {
    ; // wait for serial port to connect. Needed for native USB port only
  }

  pinMode(RED_LED_PIN, OUTPUT);
  pinMode(GREEN_LED_PIN, OUTPUT);
  digitalWrite(RED_LED_PIN, HIGH);
  digitalWrite(GREEN_LED_PIN, HIGH);
  
  delay(1000);

  // Serial.printf("Test1\n");
  strcat(clientId, String(WiFi.macAddress()).c_str());
  // Serial.printf("This board is now called %s\n", clientId);
  // Serial.printf("Test2\n");


  WiFi.persistent(false);
  WiFi.setAutoConnect(false);
  WiFi.setAutoReconnect(true);
  //WiFi.setTxPower(WIFI_POWER_2dBm);

  reconnect();

  // Serial.print(deviceCount);
  // Serial.println(" sensors found");
}

void reconnect() {
  digitalWrite(RED_LED_PIN, HIGH);
  digitalWrite(GREEN_LED_PIN, LOW);
  reconnectWiFi();
  reconnectMQTT();
}

void reconnectWiFi() {
  switch (WiFi.status()) {
    case WL_CONNECTED:
      digitalWrite(GREEN_LED_PIN, HIGH);
      digitalWrite(RED_LED_PIN, HIGH);
      return;
    case WL_IDLE_STATUS: // Serial.println("Current WiFi state: Idle");
      break;
    case WL_NO_SHIELD: // Serial.println("Current WiFi state: No wifi shield");
      break;
    case WL_CONNECT_FAILED: // Serial.println("Current WiFi state: Wifi connect failed");
      break;
    case WL_CONNECTION_LOST: // Serial.println("Current WiFi state: Wifi connection lost");
      break;
    case WL_DISCONNECTED: // Serial.println("Current WiFi state: Wifi disconnected");
      break;
  }

  int state = LOW;

  while (WiFi.status() != WL_CONNECTED) {
    digitalWrite(RED_LED_PIN, HIGH);
    //WiFi.mode(WIFI_OFF);
    //delay(1000);
    //WiFi.mode(WIFI_STA);
    //delay(1000);
    WiFi.begin(ssid, password);
    Serial.print("Reconnecting to WiFi from device ");
    Serial.println(String(WiFi.macAddress()).c_str());
    for (int dots = 0; dots < 75 && WiFi.status() != WL_CONNECTED; dots++) {
      Serial.print(".");
      if (state == LOW) state = HIGH;
      else state = LOW;
      digitalWrite(RED_LED_PIN, state);
      delay(200);
    }
    Serial.println();
  }

  digitalWrite(GREEN_LED_PIN, HIGH);
  digitalWrite(RED_LED_PIN, HIGH);

  // Serial.print("Connected to WiFi.\nLocal IP address: ");
  // Serial.println(WiFi.localIP());
}

void reconnectMQTT() {
  while (!mqttClient.connected()) {
    mqttClient.setServer(mqttBroker, mqttPort);
    mqttClient.setCallback(mqttCallback);
    // Serial.printf("Reconnecting to MQTT broker as %s\n", clientId);

    if (mqttClient.connect(clientId)) { //, mqtt_username, mqtt_password)) {
      // Serial.println("Connected to MQTT broker");
    } else {
      // Serial.printf("Failed to connect to MQTT broker with state %d. Retry in 2 seconds.\n", mqttClient.state());
      delay(2000);
    }
  }
  digitalWrite(RED_LED_PIN, LOW);
}

void loop() {
  reconnect();

  handleSensors();

  measurements++;

  /*
    if (measurements % 10 == 0) {
    for (int sensor = 0;  sensor < deviceCount;  sensor++) {
      Serial.print("Sensor ");
      Serial.print(sensor + 1);
      Serial.print(" (all) : ");
      sum = 0.0;
      for (int j = 0; j < measurements; j++) {
        sum += data[sensor][j];
      }
      Serial.print(" AVG ");
      Serial.print(sum / measurements);
      Serial.print("; ");
      for (int j = 0; j < measurements; j++) {
        Serial.print(data[sensor][j]);
        //Serial.print(data0[j]);
        Serial.print(";");
        sum += data[sensor][j];
      }
      Serial.println();
    }
    }
  */

  // Serial.println();

  delay(10000);
}

void handleSensors() {
  // Reset one-wire sensors (allows to add sensors on-the-fly)
  sensors1.begin();
  int deviceCount = sensors1.getDeviceCount();
  sensors1.requestTemperatures();

  float sum = 0.0;
  for (int sensor = 0;  sensor < deviceCount;  sensor++) {
    int sensorStart1 = sensor + 1;
    char topic[100] = "";
    strcpy(topic, clientId);
    strcat(topic, "/temp/sensor/");
    strcat(topic, String(sensorStart1).c_str());

    Serial.print("Sensor ");
    Serial.print(sensorStart1);
    Serial.print(" : ");
    float tempC = sensors1.getTempCByIndex(sensor);
    sum += tempC;
    Serial.print(tempC);
    Serial.print("C | ");
    mqttClient.publish(topic, String(tempC).c_str());
  }
  sensors2.begin();
  int prevDeviceCount = deviceCount;
  deviceCount += sensors2.getDeviceCount();
  sensors2.requestTemperatures();
  for (int sensor = 0;  sensor + prevDeviceCount < deviceCount;  sensor++) {
    int sensorStart1 = sensor + prevDeviceCount + 1;
    char topic[100] = "";
    strcpy(topic, clientId);
    strcat(topic, "/temp/sensor/");
    strcat(topic, String(sensorStart1).c_str());

    Serial.print("Sensor ");
    Serial.print(sensorStart1);
    Serial.print(" : ");
    float tempC = sensors2.getTempCByIndex(sensor);
    sum += tempC;
    Serial.print(tempC);
    Serial.print("C | ");
    mqttClient.publish(topic, String(tempC).c_str());
  }
  sensors3.begin();
  prevDeviceCount = deviceCount;
  deviceCount += sensors3.getDeviceCount();
  sensors3.requestTemperatures();
  for (int sensor = 0;  sensor + prevDeviceCount < deviceCount;  sensor++) {
    int sensorStart1 = sensor + prevDeviceCount + 1;
    char topic[100] = "";
    strcpy(topic, clientId);
    strcat(topic, "/temp/sensor/");
    strcat(topic, String(sensorStart1).c_str());

    Serial.print("Sensor ");
    Serial.print(sensorStart1);
    Serial.print(" : ");
    float tempC = sensors3.getTempCByIndex(sensor);
    sum += tempC;
    Serial.print(tempC);
    Serial.print("C | ");
    mqttClient.publish(topic, String(tempC).c_str());
  }
  sensors4.begin();
  prevDeviceCount = deviceCount;
  deviceCount += sensors4.getDeviceCount();
  sensors4.requestTemperatures();
  for (int sensor = 0;  sensor + prevDeviceCount < deviceCount;  sensor++) {
    int sensorStart1 = sensor + prevDeviceCount + 1;
    char topic[100] = "";
    strcpy(topic, clientId);
    strcat(topic, "/temp/sensor/");
    strcat(topic, String(sensorStart1).c_str());

    Serial.print("Sensor ");
    Serial.print(sensorStart1);
    Serial.print(" : ");
    float tempC = sensors4.getTempCByIndex(sensor);
    sum += tempC;
    Serial.print(tempC);
    Serial.print("C | ");
    mqttClient.publish(topic, String(tempC).c_str());
  }
  sensors5.begin();
  prevDeviceCount = deviceCount;
  deviceCount += sensors5.getDeviceCount();
  sensors5.requestTemperatures();
  for (int sensor = 0;  sensor + prevDeviceCount < deviceCount;  sensor++) {
    int sensorStart1 = sensor + prevDeviceCount + 1;
    char topic[100] = "";
    strcpy(topic, clientId);
    strcat(topic, "/temp/sensor/");
    strcat(topic, String(sensorStart1).c_str());

    Serial.print("Sensor ");
    Serial.print(sensorStart1);
    Serial.print(" : ");
    float tempC = sensors5.getTempCByIndex(sensor);
    sum += tempC;
    Serial.print(tempC);
    Serial.print("C | ");
    mqttClient.publish(topic, String(tempC).c_str());
  }
  float avgTempC = sum / deviceCount;
  Serial.print(" AVG ");
  Serial.println(avgTempC);

  digitalWrite(GREEN_LED_PIN, LOW);
  char topic[100] = "";
  strcpy(topic, clientId);
  strcat(topic, "/temp/sensor/avg");
  mqttClient.publish(topic, String(avgTempC).c_str());

  topic[0] = '\0';
  strcpy(topic, clientId);
  strcat(topic, "/heap/free");
  mqttClient.publish(topic, String(ESP.getFreeHeap()).c_str());

  topic[0] = '\0';
  strcpy(topic, clientId);
  strcat(topic, "/ip");
  mqttClient.publish(topic, WiFi.localIP().toString().c_str());
  delay(100);
  digitalWrite(GREEN_LED_PIN, HIGH);
}

void mqttCallback(char *topic, byte *payload, unsigned int length) {
  // Serial.print("Message arrived in topic: ");
  // Serial.println(topic);
  // Serial.print("Message:");
  // for (int i = 0; i < length; i++) {
  //     Serial.print((char) payload[i]);
  // }
  // Serial.println();
  // Serial.println("-----------------------");
}

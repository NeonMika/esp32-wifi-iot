#include <WiFi.h>
#include <PubSubClient.h>
#include <OneWire.h>
#include <DallasTemperature.h>

// Do not use GPIO 6 - GPIO 11 (connected to SPI-Flash on board)
// Pin 34 - 39 are only GPI (input only)
// Touchsensor pins (I have no idea what they are doing): GPIO 0, 2, 4, 12, 13, 14, 15, 27, 32, 33
// I2C: GPIO 21, 22
// VSPI: 5, 18, 19, 23
// HSPI: 12, 13, 14, 15
// Strapping pins for flashing: 0, 2, 4, 5, 12, 15
// Change state during startup: 1, 3, 5, 6 - 11, 14, 15
// UART: 1, 3, 16, 17
// Define to which pin of the Arduino the 1-Wire bus is connected:
#define ONE_WIRE_BUS_PIN_1 21
#define ONE_WIRE_BUS_PIN_2 5
#define ONE_WIRE_BUS_PIN_3 18
#define ONE_WIRE_BUS_PIN_4 19
#define ONE_WIRE_BUS_PIN_5 23

#define RED_LED_PIN 0
#define GREEN_LED_PIN 2

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
const char* password =  "xxx"; // TODO!

int measurements = 0;

// MQTT
WiFiClient wifiClient;
PubSubClient mqttClient(wifiClient);
const char *mqttBroker = "192.168.XXX.YYY"; // TODO!
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
  digitalWrite(RED_LED_PIN, LOW);
  digitalWrite(GREEN_LED_PIN, LOW);
  
  delay(1000);

  strcat(clientId, String(WiFi.macAddress()).c_str());
  Serial.printf("This board is now called %s\n", clientId);

  WiFi.persistent(false);
  WiFi.setAutoConnect(false);
  WiFi.setAutoReconnect(true);
  //WiFi.setTxPower(WIFI_POWER_2dBm);

  reconnect();
}

void reconnect() {
  digitalWrite(RED_LED_PIN, LOW);
  digitalWrite(GREEN_LED_PIN, LOW);
  reconnectWiFi();
  reconnectMQTT();
}

void reconnectWiFi() {
  switch (WiFi.status()) {
    case WL_CONNECTED:
      Serial.println("Reconnecting WiFi - Already connected");
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
    digitalWrite(RED_LED_PIN, LOW);
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

  Serial.print("WiFi state: ");
  switch (WiFi.status()) {
    case WL_CONNECTED:
      Serial.println("WL_CONNECTED");
      break;
    case WL_IDLE_STATUS: 
      Serial.println("WL_IDLE_STATUS");
      break;
    case WL_NO_SHIELD: 
      Serial.println("WL_NO_SHIELD");
      break;
    case WL_CONNECT_FAILED:
      Serial.println("WL_CONNECT_FAILED");
      break;
    case WL_CONNECTION_LOST: 
      Serial.println("WL_CONNECTION_LOST");
      break;
    case WL_DISCONNECTED: 
      Serial.println("WL_DISCONNECTED");
      break;
    default:
      Serial.println("Unknown type");
      break;
  }
  digitalWrite(RED_LED_PIN, HIGH);

  Serial.print("Connected to WiFi.\nLocal IP address: ");
  Serial.println(WiFi.localIP());
}

void reconnectMQTT() {
  int mqttTryCount = 0;
  digitalWrite(GREEN_LED_PIN, LOW);

  if(mqttClient.connected()) {
    digitalWrite(GREEN_LED_PIN, HIGH);
    Serial.println("Reconnecting MQTT broker - Already connected");
    return;
  }

  Serial.println("Reconnecting MQTT broker ...");
    
  while (!mqttClient.connected()) {
    mqttTryCount++;
    mqttClient.setServer(mqttBroker, mqttPort);
    mqttClient.setCallback(mqttCallback);
    // Serial.printf("Reconnecting to MQTT broker as %s\n", clientId);

    if (mqttClient.connect(clientId)) { //, mqtt_username, mqtt_password)) {
      Serial.println("Connected to MQTT broker");
      digitalWrite(GREEN_LED_PIN, HIGH);
    } else {
      Serial.printf("Failed to connect to MQTT broker with state %d. Retry in 2 seconds.\n", mqttClient.state());
      digitalWrite(GREEN_LED_PIN, HIGH);
      delay(100);  
      digitalWrite(GREEN_LED_PIN, LOW);
      delay(2000);
      if(mqttTryCount == 5) {
        reconnect();
        return;
      }
    }
  }
  digitalWrite(GREEN_LED_PIN, HIGH);
}

void loop() {
  Serial.printf("Measurement loop #%d\n", measurements);
  
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
    Serial.print(" (Wire 1): ");
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
    Serial.print(" (Wire 2): ");
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
    Serial.print(" (Wire 3): ");
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
    Serial.print(" (Wire 4): ");
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
    Serial.print(" (Wire 5): ");
    float tempC = sensors5.getTempCByIndex(sensor);
    sum += tempC;
    Serial.print(tempC);
    Serial.print("C | ");
    mqttClient.publish(topic, String(tempC).c_str());
  }
  float avgTempC;
  if(deviceCount > 0) {
    avgTempC = sum / deviceCount;
  } else {
    avgTempC = -255;
  }
  Serial.print(" AVG ");
  Serial.println(avgTempC);

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

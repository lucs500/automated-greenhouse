#include <DHT.h>            // Biblioteca de sensores DHT
#include <ESP8266WiFi.h>    // Biblioteca WiFI
#include <PubSubClient.h>   // Biblioteca MQTT

// Conexão ao servidor MQTT
const char* mqtt_server = "#IPRPI";
const char* mqtt_user = "#USUÁRIO"; // Usuário configurado no broker
const char* mqtt_password = "#SENHAMQTT"; // Senha configurada no broker
const char* clientId = "ESP8266Client";

// Credenciais do WiFi
const char* ssid = "#SSID";
const char* password = "#SENHAWIFI";

#define DHTPIN 15      // D8
#define DHTTYPE DHT11  // Tipo do sensor

#define SOIL_PIN 13    // D7
#define LDR_PIN 12     // D6

#define FAN_PIN 2   // D4
#define PUMP_PIN 4   // D2
#define PWM_FAN_PIN 14   // D4

DHT dht(DHTPIN, DHTTYPE);
WiFiClient espClient;
PubSubClient client(espClient);

void setup() {
  Serial.begin(9600);

  pinMode(SOIL_PIN, OUTPUT);
  pinMode(LDR_PIN, OUTPUT);
  digitalWrite(SOIL_PIN, LOW);
  digitalWrite(LDR_PIN, LOW);   // Desligando ambos os sensores analógicos inicialmente

  dht.begin();

  setup_wifi();
  client.setServer(mqtt_server, 1883);

  pinMode(FAN_PIN, OUTPUT);
  pinMode(PUMP_PIN, OUTPUT);

  digitalWrite(FAN_PIN, LOW);
  digitalWrite(PUMP_PIN, LOW);  // Desligando ventoinha e bomba inicialmente

  // Set the MQTT callback
  client.setCallback(mqttCallback);

  pinMode(PWM_FAN_PIN, OUTPUT);
  analogWrite(PWM_FAN_PIN, 0);  // Ajustando pulso PWM para 0
}

void loop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  // Leitura dos parâmetros do DHT11
  float humidity = dht.readHumidity();
  float temperature = dht.readTemperature();

  char temp_buff[8];
  char hum_buff[8];

  dtostrf(t, 1, 2, temp_buff);
  dtostrf(h, 1, 2, hum_buff);  // Transformando medidas float em string

  // Leitura YL-38 & YL-69
  digitalWrite(SOIL_PIN, HIGH);  // Liga o sensor de umidade do solo
  delay(100);                    // Delay para leitura e estabilização
  int soilValue = analogRead(A0);
  digitalWrite(SOIL_PIN, LOW);   // Desliga o sensor
  
  // Leitura LDR
  digitalWrite(LDR_PIN, HIGH);
  delay(100);
  int ldrValue = analogRead(A0);
  digitalWrite(LDR_PIN, LOW);

  String soilMsg = String(soilValue);
  String ldrMsg = String(ldrValue);

  client.publish("home/temperature", temp_buff);
  client.publish("home/humidity", hum_buff);
  client.publish("home/soilMoisture", soilMsg.c_str());
  client.publish("home/luminosity", ldrMsg.c_str());  // Nome dos tópicos publicados

  delay(2000);  // Medidas enviadas a cada 2 segundos
}

void mqttCallback(char* topic, byte* payload, unsigned int length) {
  String topicStr = topic;

  // Convert the payload to a string
  payload[length] = '\0';
  String payloadStr = String((char*)payload);

  if (topicStr == "home/fanControl") {
    if (payloadStr == "ON") {
      digitalWrite(FAN_PIN, HIGH);
    } else {
      digitalWrite(FAN_PIN, LOW);
    }
  }

  if (topicStr == "home/pumpControl") {
    if (payloadStr == "ON") {
      digitalWrite(PUMP_PIN, HIGH);
    } else {
      digitalWrite(PUMP_PIN, LOW);
    }
  }
  if (topicStr == "home/fanSpeedControl") {
    int pwmValue = payloadStr.toInt();
    if (pwmValue >= 0 && pwmValue <= 1023) {
      analogWrite(PWM_FAN_PIN, pwmValue);
    }
  }
}

void setup_wifi() {
  delay(10);
  Serial.println();
  Serial.print("Conectando a ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi conectada");
  Serial.println("Endereço de IP: ");
  Serial.println(WiFi.localIP());
}

void reconnect() {
  while (!client.connected()) {
    Serial.print("Estabelecendo conexão MQTT...");
    if (client.connect(clientId, mqtt_user, mqtt_password)) {
      Serial.println("conectado");
    } else {
      Serial.print("falha, rc=");
      Serial.print(client.state());
      Serial.println(" tente novamente em 5 segundos");
      delay(5000);
    }
  }
  client.subscribe("home/fanControl");
  client.subscribe("home/pumpControl");
  client.subscribe("home/fanSpeedControl");
}

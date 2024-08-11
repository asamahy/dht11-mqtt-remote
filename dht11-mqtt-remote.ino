#include <WiFi.h>
#include <PubSubClient.h>
#include <DHT11.h>
DHT11 dht11(4);

// Primary network credentials
const char* ssid1 = "SSID1";
const char* password1 = "PASS1";

// Backup network credentials
const char* ssid2 = "SSID2";
const char* password2 = "PASS2";

const char* mqtt_server = "MQTT-SERVER";

WiFiClient espClient;
PubSubClient client(espClient);

unsigned long lastMsg = 0;
unsigned long timeout = 10000; // 10 seconds timeout
float temp = 0;
float hum = 0;

void connect_to_wifi(const char* ssid, const char* password) {
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  unsigned long startAttemptTime = millis();

  while (WiFi.status() != WL_CONNECTED && millis() - startAttemptTime < timeout) {
    delay(500);
    Serial.print(".");
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("");
    Serial.print("WiFi connected to: ");
    Serial.println(ssid);
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println("");
    Serial.print("Unable to connect to: ");
    Serial.println(ssid);
  }
}

void setup_wifi() {
  connect_to_wifi(ssid1, password1);

  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("Trying backup WiFi...");
    connect_to_wifi(ssid2, password2);
  }

  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("Failed to connect to any WiFi network.");
  }
}

void reconnect() {
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    String clientId = "ESP8266Client-";
    clientId += String(random(0xffff), HEX);

    if (client.connect(clientId.c_str())) {
      Serial.println("Connected");
      client.publish("mytopic", "Connected");
      client.subscribe("mytopic");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      delay(5000);
    }
  }
}

void connected_wifi() {
  if (WiFi.status() == WL_CONNECTED) {
    digitalWrite(2, HIGH);
  } else {
    digitalWrite(2, LOW);
    Serial.println("WiFi Disconnected, trying to reconnect...");
    setup_wifi();
  }
}

void connected_mqtt() {
  if (client.connected()) {
    digitalWrite(15, HIGH);
  } else {
    digitalWrite(15, LOW);
    reconnect();
  }
}

void getDHT() {
  int temperature = dht11.readTemperature();
  int humidity = dht11.readHumidity();

  if (temperature != DHT11::ERROR_CHECKSUM && temperature != DHT11::ERROR_TIMEOUT &&
      humidity != DHT11::ERROR_CHECKSUM && humidity != DHT11::ERROR_TIMEOUT) { 

    String temp = String(temperature);
    Serial.print("Temperature: ");
    Serial.println(temp);
    client.publish("temp", temp.c_str());

    String hum = String(humidity);
    Serial.print("Humidity: ");
    Serial.println(hum);
    client.publish("hum", hum.c_str());

    delay(1000);
  } else {
    if (temperature == DHT11::ERROR_TIMEOUT || temperature == DHT11::ERROR_CHECKSUM) {
      Serial.print("Temperature Reading Error: ");
      Serial.println(DHT11::getErrorString(temperature));
    }
    if (humidity == DHT11::ERROR_TIMEOUT || humidity == DHT11::ERROR_CHECKSUM) {
      Serial.print("Humidity Reading Error: ");
      Serial.println(DHT11::getErrorString(humidity));
    }
  }
}

void setup() {
  pinMode(2, OUTPUT);
  pinMode(15, OUTPUT);
  Serial.begin(115200);
  setup_wifi();
  client.setServer(mqtt_server, 1883);
}

void loop() {
  connected_wifi();
  connected_mqtt();
  getDHT();
}

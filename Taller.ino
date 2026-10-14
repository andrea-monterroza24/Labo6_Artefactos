
#include <WiFi.h>
#include <PubSubClient.h>
#include <DHT.h>

// ======================================================
// CONFIGURACION WIFI
// ======================================================

const char* WIFI_SSID = "Lizieee";
const char* WIFI_PASSWORD = "Andrea123456";


// ======================================================
// CONFIGURACION MQTT
// ======================================================

const char* MQTT_SERVER = "10.129.40.31";
const int MQTT_PORT = 1883;

// Topics utilizados por el dashboard React.
const char* TOPIC_TEMPERATURA = "clima/esp32/temperatura";
const char* TOPIC_HUMEDAD = "clima/esp32/humedad";
const char* TOPIC_LUZ = "clima/esp32/luz";


// ======================================================
// CONFIGURACION DE SENSORES
// ======================================================

#define DHT_PIN 4
#define DHT_TYPE DHT11
#define LDR_PIN 32

DHT dht(DHT_PIN, DHT_TYPE);


// ======================================================
// CONFIGURACION MQTT Y TIEMPO
// ======================================================

WiFiClient wifiClient;
PubSubClient mqttClient(wifiClient);

// Enviar una lectura cada 2 segundos.
const unsigned long INTERVALO_ENVIO = 2000;

unsigned long ultimoEnvio = 0;


// ======================================================
// CONECTAR A WIFI
// ======================================================

void conectarWiFi() {

  Serial.print("Conectando a WiFi...");

  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println();
  Serial.println("WiFi conectado");

  Serial.print("IP del ESP32: ");
  Serial.println(WiFi.localIP());
}


// ======================================================
// CONECTAR A MQTT
// ======================================================

void conectarMQTT() {

  while (!mqttClient.connected()) {

    Serial.print("Conectando a Mosquitto...");

    // Crear un Client ID diferente para este ESP32.
    String clientId = "ESP32-";
    clientId += String(
      (uint32_t)(ESP.getEfuseMac() & 0xFFFFFFFF),
      HEX
    );

    if (mqttClient.connect(clientId.c_str())) {

      Serial.println(" conectado");

    } else {

      Serial.print(" error, rc=");
      Serial.print(mqttClient.state());
      Serial.println(". Reintentando en 2 segundos...");

      delay(2000);
    }
  }
}


// ======================================================
// LEER Y PUBLICAR SENSORES
// ======================================================

void publicarSensores() {

  // 1. Obtener la temperatura
  float temperatura = dht.readTemperature();
  // 2. Obtener la humedad
  float humedad = dht.readHumidity(); 

  // 3. Obtener la lectura del sensor de iluminacion
  int lecturaLDR = analogRead(LDR_PIN);
  int luz = map(lecturaLDR, 0, 4095, 0, 100);
  luz = constrain(luz, 0, 100);

  // 4. Preparar los valores para poder publicarlos mediante MQTT
  char temp[16];
  char hum[16];
  char iluminacion[16];

  dtostrf(temperatura, 1, 2, temp);
  dtostrf(humedad, 1, 2, hum);
  sprintf(iluminacion, "%d", luz);

  // 5. Publicar cada dato en su topic correspondiente
  bool t = mqttClient.publish(TOPIC_TEMPERATURA, temp, true);
  bool h = mqttClient.publish(TOPIC_HUMEDAD, hum, true);
  bool l = mqttClient.publish(TOPIC_LUZ, iluminacion, true);

  // 6. Mostrar las mediciones en el Monitor Serial 
  Serial.println("Lecturas de los sensores:");

  Serial.print("Temperatura: ");
  Serial.println(temperatura);

  Serial.print("Humedad: ");
  Serial.println(humedad);

  Serial.print("Lectura de luz: ");
  Serial.println(lecturaLDR);

  Serial.print("Luz en porcentaje: ");
  Serial.println(luz);

  Serial.println("================================");
};


// ======================================================
// CONFIGURACION INICIAL
// ======================================================

void setup() {

  Serial.begin(115200);

  dht.begin();
  pinMode(LDR_PIN, INPUT);
  analogReadResolution(12);

  conectarWiFi();

  mqttClient.setServer(MQTT_SERVER, MQTT_PORT);

  delay(2000);
}

// ======================================================
// CICLO PRINCIPAL
// ======================================================

void loop() {

  // Si se pierde WiFi, volver a conectarse.
  if (WiFi.status() != WL_CONNECTED) {
    conectarWiFi();
  }

  // Si se pierde MQTT, volver a conectarse.
  if (!mqttClient.connected()) {
    conectarMQTT();
  }

  // Mantener la conexion MQTT activa.
  mqttClient.loop();

  // Publicar sensores cada 2 segundos.
  if (millis() - ultimoEnvio >= INTERVALO_ENVIO) {

    ultimoEnvio = millis();

    publicarSensores();
  }
}

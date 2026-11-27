#include <WiFi.h>
#include <PubSubClient.h>
#include <Wire.h>
#include <Adafruit_VL53L0X.h>

// =====================================
// 1. CONFIGURACION WIFI
// =====================================

const char* WIFI_SSID = "NOMBRE_DE_TU_WIFI";
const char* WIFI_PASSWORD = "CONTRASENA_DE_TU_WIFI";

// =====================================
// 2. CONFIGURACION MQTT
// =====================================

const char* MQTT_SERVER = "test.mosquitto.org";
const int MQTT_PORT = 1883;

const char* TOPIC = "levitador/prototipo_7A31/telemetria";

const char* CLIENT_ID = "ESP32_Levitador_7A31";

WiFiClient espClient;
PubSubClient mqttClient(espClient);

// =====================================
// 3. PINES
// =====================================

#define SDA_PIN 21
#define SCL_PIN 22
#define PWM_PIN 27

// =====================================
// 4. CONFIGURACION PWM
// =====================================

#define PWM_CHANNEL 0
#define PWM_FREQUENCY 1000
#define PWM_RESOLUTION 8

const float DISTANCIA_MIN = 0.0;
const float DISTANCIA_MAX = 450.0;

const float GANANCIA = 2.5;
const float VOLTAJE_ESP32 = 3.3;

// =====================================
// 5. PARAMETROS DEL PROYECTO
// =====================================

const float ALTURA_TUBO = 450.0;
const float SETPOINT = 150.0;

Adafruit_VL53L0X sensor;

// Temporizadores
unsigned long tiempoPublicacion = 0;
unsigned long ultimoIntentoMQTT = 0;

const unsigned long INTERVALO_MQTT = 500;
const unsigned long INTERVALO_RECONEXION = 5000;

// =====================================
// 6. CONEXION WIFI
// =====================================

void conectarWiFi() {

  Serial.println();
  Serial.println("Conectando a WiFi...");

  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println();
  Serial.println("WiFi conectado correctamente");

  Serial.print("Direccion IP: ");
  Serial.println(WiFi.localIP());
}

// =====================================
// 7. CONEXION MQTT
// =====================================

void conectarMQTT() {

  if (WiFi.status() != WL_CONNECTED) {
    return;
  }

  if (!mqttClient.connected()) {

    unsigned long ahora = millis();

    if (ahora - ultimoIntentoMQTT >= INTERVALO_RECONEXION) {

      ultimoIntentoMQTT = ahora;

      Serial.println("Conectando a MQTT...");

      if (mqttClient.connect(CLIENT_ID)) {

        Serial.println("MQTT conectado correctamente");

      } else {

        Serial.print("Error MQTT. Codigo: ");
        Serial.println(mqttClient.state());

      }
    }
  }
}

// =====================================
// 8. PUBLICAR DATOS MQTT
// =====================================

void publicarDatos(float distancia, float altura, bool zonaUtil) {

  char mensaje[200];

  snprintf(
    mensaje,
    sizeof(mensaje),
    "{\"distancia_mm\":%.1f,\"altura_mm\":%.1f,\"setpoint_mm\":%.1f,\"zona_util\":%s}",
    distancia,
    altura,
    SETPOINT,
    zonaUtil ? "true" : "false"
  );

  Serial.print("Datos preparados: ");
  Serial.println(mensaje);

  if (mqttClient.connected()) {

    if (mqttClient.publish(TOPIC, mensaje)) {

      Serial.println("MENSAJE ENVIADO A MQTT CORRECTAMENTE");

    } else {

      Serial.println("ERROR AL PUBLICAR EN MQTT");

    }

  } else {

    Serial.println("MQTT DESCONECTADO. No se pudo publicar.");

  }
}

// =====================================
// 9. CONFIGURACION INICIAL
// =====================================

void setup() {

  Serial.begin(115200);
  delay(1000);

  Serial.println();
  Serial.println("================================");
  Serial.println("   LEVITADOR NEUMATICO ESP32");
  Serial.println("================================");

  // Iniciar sensor
  Wire.begin(SDA_PIN, SCL_PIN);

  // Iniciar PWM
  ledcSetup(PWM_CHANNEL, PWM_FREQUENCY, PWM_RESOLUTION);
  ledcAttachPin(PWM_PIN, PWM_CHANNEL);
  ledcWrite(PWM_CHANNEL, 0);

  // Comprobar sensor
  Serial.println("Iniciando sensor VL53L0X...");

  if (!sensor.begin()) {

    Serial.println("ERROR: Sensor no detectado");

    while (true) {
      ledcWrite(PWM_CHANNEL, 0);
      delay(1000);
    }
  }

  Serial.println("Sensor conectado correctamente");

  // Conectar WiFi
  conectarWiFi();

  // Configurar MQTT
  mqttClient.setServer(MQTT_SERVER, MQTT_PORT);
  mqttClient.setBufferSize(256);

  Serial.println("--------------------------------");
  Serial.println("Sistema iniciado");
}

// =====================================
// 10. BUCLE PRINCIPAL
// =====================================

void loop() {

  // Mantener conexion MQTT
  if (WiFi.status() == WL_CONNECTED) {

    conectarMQTT();
    mqttClient.loop();

  }

  // Leer sensor
  VL53L0X_RangingMeasurementData_t medicion;

  sensor.rangingTest(&medicion, false);

  if (medicion.RangeStatus != 4) {

    float distancia = medicion.RangeMilliMeter;

    distancia = constrain(
      distancia,
      DISTANCIA_MIN,
      DISTANCIA_MAX
    );

    // Calcular altura desde el fondo
    float altura = ALTURA_TUBO - distancia;

    altura = constrain(altura, 0.0, ALTURA_TUBO);

    // Zona util: distancia entre 150 y 450 mm
    bool zonaUtil = (distancia >= 150.0 && distancia <= 450.0);

    // Calcular PWM para salida analogica
    int pwm = round(
      (distancia / DISTANCIA_MAX) * 255.0
    );

    pwm = constrain(pwm, 0, 255);

    ledcWrite(PWM_CHANNEL, pwm);

    float voltaje = (pwm / 255.0) *
                    VOLTAJE_ESP32 *
                    GANANCIA;

    // Mostrar medicion
    Serial.print("Distancia: ");
    Serial.print(distancia);
    Serial.print(" mm | Altura: ");
    Serial.print(altura);
    Serial.print(" mm | PWM: ");
    Serial.print(pwm);
    Serial.print(" | Voltaje estimado: ");
    Serial.print(voltaje, 2);
    Serial.println(" V");

    // Publicar cada 500 ms
    unsigned long ahora = millis();

    if (ahora - tiempoPublicacion >= INTERVALO_MQTT) {

      tiempoPublicacion = ahora;

      publicarDatos(distancia, altura, zonaUtil);

    }

  } else {

    ledcWrite(PWM_CHANNEL, 0);

    Serial.println("Sensor fuera de rango");

  }

  delay(100);
}
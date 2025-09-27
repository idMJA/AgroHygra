#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <ESPmDNS.h>
#include <ArduinoJson.h>
#include "DHT.h"
#include <PubSubClient.h>
#include <WiFiClientSecure.h>

// ========== KONFIGURASI WIFI ==========
const char *ssid = "MJLANA";       // Ganti dengan nama WiFi Anda
const char *password = "lihatjam"; // Ganti dengan password WiFi Anda

// ========== KONFIGURASI MQTT ==========
// Public MQTT broker for testing (no authentication required)
// For production, use HiveMQ Cloud with TLS and authentication
const char *MQTT_HOST = "broker.hivemq.com";    // Free public broker
const int MQTT_PORT = 1883;                     // Non-TLS port for testing
const char *MQTT_USERNAME = "";                 // No username for public broker
const char *MQTT_PASSWORD = "";                 // No password for public broker
const char *MQTT_CLIENT_ID = "AgroHygra-ESP32"; // Unique client ID

// MQTT Topics
const char *TOPIC_SENSORS = "agrohygra/sensors";
const char *TOPIC_PUMP_COMMAND = "agrohygra/pump/command";
const char *TOPIC_PUMP_STATUS = "agrohygra/pump/status";
const char *TOPIC_SYSTEM_STATUS = "agrohygra/system/status";
const char *TOPIC_LOGS = "agrohygra/logs";

// Alternative free MQTT brokers:
// - broker.hivemq.com:1883 (public, no auth, no TLS)
// - test.mosquitto.org:1883 (public, no auth, no TLS)
// For production, use TLS + authentication (HiveMQ Cloud, CloudMQTT, AWS IoT, etc.)

// ========== KONFIGURASI PIN ==========
// Wiring notes:
// - Capacitive soil moisture sensor: VCC -> 3.3V, GND -> GND, AOUT -> analog input (SOIL_PIN)
//   Use 3.3V to power the sensor when connecting to ESP32. Many breakout boards work with 3.3V.
// - If your sensor breakout already has a pull-up or other circuitry, that's fine. Avoid powering the sensor
//   from the ESP32 3.3V if the sensor will drive a heavy load; use a separate 3.3V/5V supply and common GND.
#define DHT_PIN 4      // Pin sensor suhu/kelembapan udara (DHT11)
#define DHT_TYPE DHT11 // Jenis sensor DHT
// Recommended analog pins for ESP32 (ADC1): 32, 33, 34, 35, 36, 39
// Use an ADC1 pin to avoid conflicts with WiFi (ADC2 is shared with WiFi)
// Use GPIO34 as default for the capacitive soil sensor's AOUT
#define SOIL_PIN 34 // Pin analog untuk capacitive soil moisture sensor (AOUT -> this pin)
// MQ-135 air quality sensor wiring: VCC -> 5V or 3.3V, GND -> GND, AO -> analog pin, DO -> digital pin (optional)
// AO gives analog reading (0-4095 on ESP32), DO gives digital threshold output (HIGH/LOW based on onboard potentiometer)
// Use ADC1 pins to avoid WiFi conflicts. For digital pin, use any available GPIO.
#define MQ135_AO_PIN 35 // Pin analog untuk MQ-135 AO (air quality analog output)
#define MQ135_DO_PIN 32 // Pin digital untuk MQ-135 DO (digital threshold output) - optional
// Relay wiring: VCC, GND, IN. Relay module output pins: COM (common), NO (normally open), NC (normally closed).
// For a pump, wire the pump's power supply through COM -> NO (pump powered when relay ON) for normally-open behavior.
// Many 1-channel relay modules are active-low for the IN pin (drive LOW to activate). If unsure, test carefully.
// If your relay module requires 5V VCC and a separate JD-VCC, follow the module documentation. Use an external power supply for the pump and common ground between ESP32 and relay module when needed.
#define RELAY_PIN 27 // Pin relay untuk pompa air (IN pin)
// Set to true if your relay module is active-low (IN pulled LOW to activate relay). Most cheap modules are active-low.
#define RELAY_ACTIVE_LOW true
#define LED_STATUS_PIN 2 // Pin LED built-in ESP32 untuk indikator status

// ========== KONFIGURASI SENSOR & SISTEM ==========
const int SOIL_DRY_VALUE = 3000;    // Nilai ADC saat tanah kering (kalibrasi)
const int SOIL_WET_VALUE = 1000;    // Nilai ADC saat tanah basah (kalibrasi)
const int MOISTURE_THRESHOLD = 30;  // Batas kelembapan untuk mulai menyiram (%)
const int MOISTURE_STOP = 70;       // Batas kelembapan untuk berhenti menyiram (%)
const int MAX_PUMP_TIME = 60;       // Maksimal pompa nyala (detik) untuk safety
const int SENSOR_READ_INTERVAL = 2; // Interval pembacaan sensor (detik)

// Safety: do not auto-start pump immediately after boot. Require a short delay
// and require multiple consecutive "dry" readings to avoid false positives.
const unsigned long BOOT_SAFE_DELAY = 15000; // ms to wait after boot before auto irrigation
const int REQUIRED_CONSECUTIVE_DRY = 2;      // number of consecutive dry readings required before starting pump

// ========== KONFIGURASI MQ-135 AIR QUALITY ==========
const int MQ135_CLEAN_AIR_VALUE = 500;     // Typical ADC value in clean air (calibrate in fresh air)
const int MQ135_POLLUTED_THRESHOLD = 1500; // ADC value indicating poor air quality (adjust based on environment)
const float MQ135_VOLTAGE_REF = 3.3;       // Reference voltage for ADC (3.3V for ESP32)
const float MQ135_ADC_MAX = 4095.0;        // 12-bit ADC max value
// Basic conversion constants (approximate - for accurate ppm readings, use proper calibration curve)
const float MQ135_RL_VALUE = 20.0;    // Load resistance on sensor board (kOhm) - check your module
const float MQ135_RO_CLEAN_AIR = 3.6; // Sensor resistance in clean air (kOhm) - calibrate this

// ========== VARIABEL GLOBAL ==========
DHT dht(DHT_PIN, DHT_TYPE);
WebServer server(80);
WiFiClient espClient; // Use regular WiFiClient for non-TLS
PubSubClient mqttClient(espClient);

unsigned long lastSensorRead = 0;
bool pumpActive = false;
unsigned long pumpStartTime = 0;
int soilMoisture = 0;
float temperature = 0;
float humidity = 0;
int airQuality = 0;         // MQ-135 air quality (0-100 scale, lower is better)
int airQualityRaw = 0;      // Raw ADC reading from MQ-135
bool airQualityGood = true; // Digital threshold status from DO pin

// Statistik sistem
unsigned long totalWateringTime = 0;
int wateringCount = 0;
int consecutiveDryCount = 0; // counter for consecutive dry readings

// MQTT variables
unsigned long lastMqttSensorPublish = 0;
const unsigned long MQTT_SENSOR_INTERVAL = 2000; // Publish sensor data every 2 seconds
unsigned long lastMqttReconnect = 0;
const unsigned long MQTT_RECONNECT_INTERVAL = 5000; // Try reconnect every 5 seconds

// ========== FUNGSI SENSOR ==========
int readSoilMoisture()
{
  // Read multiple samples and average for stability
  const int samples = 10;
  long sum = 0;
  for (int i = 0; i < samples; i++)
  {
    sum += analogRead(SOIL_PIN);
    delay(10);
  }
  int rawValue = sum / samples;

  // ADC on ESP32 returns 0-4095 (12-bit) for default resolution; adjust SOIL_DRY/WET values accordingly
  // Konversi ke persentase (0-100%)
  int moisture = map(rawValue, SOIL_DRY_VALUE, SOIL_WET_VALUE, 0, 100);
  moisture = constrain(moisture, 0, 100); // Batasi nilai 0-100%

  return moisture;
}

// ========== FUNGSI MQ-135 AIR QUALITY ==========
int readAirQualityRaw()
{
  // Read multiple samples and average for stability
  const int samples = 10;
  long sum = 0;
  for (int i = 0; i < samples; i++)
  {
    sum += analogRead(MQ135_AO_PIN);
    delay(10);
  }
  return sum / samples;
}

int calculateAirQualityPercent(int rawValue)
{
  // Convert raw ADC to air quality percentage (0-100, lower is better)
  // Higher ADC value = more pollution = worse air quality (higher percentage)
  int qualityPercent = map(rawValue, MQ135_CLEAN_AIR_VALUE, MQ135_POLLUTED_THRESHOLD, 0, 100);
  return constrain(qualityPercent, 0, 100);
}

float calculatePPM(int rawValue)
{
  // Basic PPM calculation (approximate - requires proper calibration for accuracy)
  // This gives a rough estimate. For precise measurements, use datasheet calibration curves.
  if (rawValue <= 0)
    return 0;

  float voltage = (rawValue / MQ135_ADC_MAX) * MQ135_VOLTAGE_REF;
  float rs = ((MQ135_VOLTAGE_REF * MQ135_RL_VALUE) / voltage) - MQ135_RL_VALUE;
  float ratio = rs / MQ135_RO_CLEAN_AIR;

  // Simplified conversion to CO2 equivalent ppm (very approximate)
  float ppm = 116.6020682 * pow(ratio, -2.769034857);
  return constrain(ppm, 10, 2000); // Reasonable range for indoor air
}

void readAllSensors()
{
  soilMoisture = readSoilMoisture();
  temperature = dht.readTemperature();
  humidity = dht.readHumidity();

  // Read MQ-135 air quality sensor
  airQualityRaw = readAirQualityRaw();
  airQuality = calculateAirQualityPercent(airQualityRaw);
  airQualityGood = digitalRead(MQ135_DO_PIN) == LOW; // DO pin is typically LOW for good air quality

  // Cek apakah pembacaan DHT berhasil
  if (isnan(temperature))
    temperature = 0;
  if (isnan(humidity))
    humidity = 0;

  // Update consecutive dry counter used by autoIrrigation safety
  if (soilMoisture <= MOISTURE_THRESHOLD)
  {
    consecutiveDryCount++;
  }
  else
  {
    consecutiveDryCount = 0;
  }
}

// ========== FUNGSI KONTROL POMPA ==========
void startPump()
{
  if (!pumpActive)
  {
    // Activate relay according to module logic level
    if (RELAY_ACTIVE_LOW)
      digitalWrite(RELAY_PIN, LOW);
    else
      digitalWrite(RELAY_PIN, HIGH);
    pumpActive = true;
    pumpStartTime = millis();
    wateringCount++;
    Serial.println("🚰 POMPA DINYALAKAN - Kelembapan tanah rendah");

    // Notify via MQTT
    if (mqttClient.connected())
    {
      mqttClient.publish(TOPIC_PUMP_STATUS, "ON");
      mqttClient.publish(TOPIC_LOGS, "Pump started automatically");
    }
  }
}

void stopPump()
{
  if (pumpActive)
  {
    // Deactivate relay according to module logic level
    if (RELAY_ACTIVE_LOW)
      digitalWrite(RELAY_PIN, HIGH);
    else
      digitalWrite(RELAY_PIN, LOW);
    pumpActive = false;

    // Catat waktu penyiraman
    unsigned long wateringDuration = (millis() - pumpStartTime) / 1000;
    totalWateringTime += wateringDuration;

    Serial.printf("🛑 POMPA DIMATIKAN - Durasi: %lu detik\n", wateringDuration);

    // Notify via MQTT
    if (mqttClient.connected())
    {
      mqttClient.publish(TOPIC_PUMP_STATUS, "OFF");
      String logMsg = "Pump stopped - Duration: " + String(wateringDuration) + "s";
      mqttClient.publish(TOPIC_LOGS, logMsg.c_str());
    }
  }
}

// ========== LOGIKA IRIGASI OTOMATIS ==========
void autoIrrigation()
{
  // Safety: do not auto-start immediately after boot
  if (millis() < BOOT_SAFE_DELAY)
  {
    // optional: if pump is currently active we still enforce safety timeout elsewhere
    Serial.println("⚠️  Menunda auto-irigasi (boot safe delay)");
    return;
  }

  // Mulai penyiraman jika kelembapan rendah
  if (!pumpActive && soilMoisture <= MOISTURE_THRESHOLD)
  {
    // require multiple consecutive dry readings to avoid false triggers
    if (consecutiveDryCount >= REQUIRED_CONSECUTIVE_DRY)
    {
      startPump();
    }
    else
    {
      Serial.printf("⚠️  Terbaca kering (%d%%) — but waiting for %d consecutive readings (have %d)\n", soilMoisture, REQUIRED_CONSECUTIVE_DRY, consecutiveDryCount);
    }
  }

  // Hentikan penyiraman jika kelembapan sudah cukup
  if (pumpActive && soilMoisture >= MOISTURE_STOP)
  {
    stopPump();
  }

  // Safety: hentikan pompa jika sudah terlalu lama nyala
  if (pumpActive && (millis() - pumpStartTime) >= (MAX_PUMP_TIME * 1000UL))
  {
    Serial.println("⚠️  SAFETY: Pompa dimatikan karena timeout!");
    stopPump();
  }
}

// ========== MQTT FUNCTIONS (PubSubClient) ==========
void mqttCallback(char *topic, byte *payload, unsigned int length)
{
  // Convert payload to string
  String message;
  for (unsigned int i = 0; i < length; i++)
  {
    message += (char)payload[i];
  }

  Serial.printf("📨 MQTT message received on %s: %s\n", topic, message.c_str());

  // Handle pump commands
  if (strcmp(topic, TOPIC_PUMP_COMMAND) == 0)
  {
    if (message == "ON" || message == "on" || message == "1")
    {
      startPump();
      mqttClient.publish(TOPIC_PUMP_STATUS, "ON");
      mqttClient.publish(TOPIC_LOGS, "Pump started via MQTT command");
    }
    else if (message == "OFF" || message == "off" || message == "0")
    {
      stopPump();
      mqttClient.publish(TOPIC_PUMP_STATUS, "OFF");
      mqttClient.publish(TOPIC_LOGS, "Pump stopped via MQTT command");
    }
    else
    {
      // Try to parse JSON command
      JsonDocument cmdDoc;
      DeserializationError error = deserializeJson(cmdDoc, message);
      if (!error && cmdDoc["pump"].is<bool>())
      {
        bool pumpCommand = cmdDoc["pump"];
        if (pumpCommand)
        {
          startPump();
          mqttClient.publish(TOPIC_PUMP_STATUS, "ON");
        }
        else
        {
          stopPump();
          mqttClient.publish(TOPIC_PUMP_STATUS, "OFF");
        }
      }
    }
  }
}

void publishSensorData()
{
  if (!mqttClient.connected())
  {
    Serial.println("❌ Cannot publish sensor data - MQTT not connected");
    return;
  }

  // Create smaller JSON to avoid buffer issues
  JsonDocument sensorDoc;
  sensorDoc["device"] = MQTT_CLIENT_ID;
  sensorDoc["time"] = millis() / 1000;  // shorter key
  sensorDoc["soil"] = soilMoisture;     // shorter key
  sensorDoc["temp"] = temperature;
  sensorDoc["hum"] = humidity;
  sensorDoc["air"] = airQuality;
  sensorDoc["airRaw"] = airQualityRaw;
  sensorDoc["airGood"] = airQualityGood;
  sensorDoc["ppm"] = (int)calculatePPM(airQualityRaw);
  sensorDoc["pump"] = pumpActive;
  sensorDoc["count"] = wateringCount;
  sensorDoc["wtime"] = totalWateringTime;
  sensorDoc["uptime"] = millis() / 1000;

  String sensorPayload;
  serializeJson(sensorDoc, sensorPayload);
  
  Serial.printf("🔧 JSON payload size: %d bytes\n", sensorPayload.length());

  // Try to publish to main topic
  bool mainPublished = mqttClient.publish(TOPIC_SENSORS, sensorPayload.c_str());
  Serial.printf("📤 Published sensor data to %s: %s (success: %s)\n", 
                TOPIC_SENSORS, sensorPayload.c_str(), mainPublished ? "YES" : "NO");
  
  // If main publish fails, try simple test message
  if (!mainPublished) {
    bool testPublished = mqttClient.publish("test/simple", "ESP32 test message");
    Serial.printf("📤 Simple test publish: %s\n", testPublished ? "YES" : "NO");
    
    // Try even simpler JSON
    String simpleJson = "{\"device\":\"ESP32\",\"soil\":" + String(soilMoisture) + ",\"temp\":" + String(temperature) + "}";
    bool simplePublished = mqttClient.publish("test/json", simpleJson.c_str());
    Serial.printf("📤 Simple JSON publish (%d bytes): %s\n", simpleJson.length(), simplePublished ? "YES" : "NO");
  }
  
  // Check MQTT client state after publish
  Serial.printf("🔧 MQTT State after publish: %d\n", mqttClient.state());
}

void connectToMqtt()
{
  if (WiFi.status() != WL_CONNECTED || mqttClient.connected())
    return;

  if (millis() - lastMqttReconnect < MQTT_RECONNECT_INTERVAL)
    return;
  lastMqttReconnect = millis();

  Serial.println("🔗 Connecting to MQTT broker...");
  Serial.printf("🔧 Broker: %s:%d\n", MQTT_HOST, MQTT_PORT);
  Serial.printf("🔧 Client ID: %s\n", MQTT_CLIENT_ID);

  if (mqttClient.connect(MQTT_CLIENT_ID))
  {
    Serial.println("✅ Connected to MQTT broker");
    Serial.printf("🔧 MQTT State: %d (Connected)\n", mqttClient.state());

    // Subscribe to pump command topic
    if (mqttClient.subscribe(TOPIC_PUMP_COMMAND))
    {
      Serial.printf("📥 Subscribed to %s\n", TOPIC_PUMP_COMMAND);
    }
    else
    {
      Serial.printf("❌ Failed to subscribe to %s\n", TOPIC_PUMP_COMMAND);
    }

    // Publish system status
    JsonDocument statusDoc;
    statusDoc["device"] = MQTT_CLIENT_ID;
    statusDoc["status"] = "online";
    statusDoc["ip"] = WiFi.localIP().toString();
    statusDoc["uptime"] = millis() / 1000;
    statusDoc["firmware"] = "AgroHygra v1.0";

    String statusPayload;
    serializeJson(statusDoc, statusPayload);
    
    bool statusPublished = mqttClient.publish(TOPIC_SYSTEM_STATUS, statusPayload.c_str(), true); // retained
    Serial.printf("📤 System status published: %s (success: %s)\n", 
                  statusPayload.c_str(), statusPublished ? "YES" : "NO");

    // Log connection
    bool logPublished = mqttClient.publish(TOPIC_LOGS, "Device connected to MQTT broker");
    Serial.printf("📤 Log published: %s\n", logPublished ? "YES" : "NO");
    
    // Test publish a simple message
    bool testPublished = mqttClient.publish("test/agrohygra", "ESP32 is online and publishing");
    Serial.printf("📤 Test message published: %s\n", testPublished ? "YES" : "NO");
  }
  else
  {
    Serial.printf("❌ MQTT connection failed, rc=%d. Trying again in %d seconds\n",
                  mqttClient.state(), MQTT_RECONNECT_INTERVAL / 1000);
    Serial.println("❌ MQTT State meanings: -4=timeout, -3=connection lost, -2=connect failed, -1=disconnected, 0=connected");
  }
}

void setupMqtt()
{
  // Set MQTT buffer size (default is only 256 bytes, too small for JSON)
  mqttClient.setBufferSize(512);
  
  // Set MQTT server and callback
  mqttClient.setServer(MQTT_HOST, MQTT_PORT);
  mqttClient.setCallback(mqttCallback);

  Serial.printf("🔧 MQTT configured for %s:%d (buffer size: 512)\n", MQTT_HOST, MQTT_PORT);
}

// ========== WEB SERVER - HALAMAN UTAMA ==========
void handleRoot()
{
  String html = R"(
<!DOCTYPE html>
<html>
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>AgroHygra - Smart Irrigation</title>
    <style>
        body { font-family: Arial, sans-serif; margin: 20px; background: #f0f8f0; }
        .container { max-width: 600px; margin: 0 auto; background: white; padding: 20px; border-radius: 10px; box-shadow: 0 4px 8px rgba(0,0,0,0.1); }
        h1 { color: #2e7d32; text-align: center; margin-bottom: 30px; }
        .sensor-card { background: #e8f5e8; padding: 15px; margin: 10px 0; border-radius: 8px; border-left: 4px solid #4caf50; }
        .sensor-value { font-size: 24px; font-weight: bold; color: #1b5e20; }
        .sensor-label { font-size: 14px; color: #666; }
        .status { padding: 10px; border-radius: 5px; text-align: center; margin: 15px 0; font-weight: bold; }
        .status.on { background: #c8e6c9; color: #2e7d32; }
        .status.off { background: #ffcdd2; color: #c62828; }
        .controls { text-align: center; margin: 20px 0; }
        .btn { padding: 12px 24px; margin: 5px; border: none; border-radius: 5px; cursor: pointer; font-size: 16px; text-decoration: none; display: inline-block; }
        .btn-primary { background: #4caf50; color: white; }
        .btn-secondary { background: #ff9800; color: white; }
        .btn:hover { opacity: 0.8; }
        .stats { background: #f5f5f5; padding: 15px; border-radius: 8px; margin-top: 20px; }
        .refresh { font-size: 12px; color: #666; text-align: center; margin-top: 15px; }
    </style>
    <script>
        // Auto refresh setiap 30 detik
        setTimeout(function(){ location.reload(); }, 30000);
    </script>
</head>
<body>
    <div class="container">
        <h1>🌱 AgroHygra</h1>
        <h2 style="text-align: center; color: #666;">Smart Irrigation System</h2>
        
        <div class="sensor-card">
            <div class="sensor-label">Kelembapan Tanah</div>
            <div class="sensor-value">)" +
                String(soilMoisture) + R"(%</div>
        </div>
        
        <div class="sensor-card">
            <div class="sensor-label">Suhu Udara</div>
            <div class="sensor-value">)" +
                String(temperature, 1) + R"(°C</div>
        </div>
        
        <div class="sensor-card">
            <div class="sensor-label">Kelembapan Udara</div>
            <div class="sensor-value">)" +
                String(humidity, 1) + R"(%</div>
        </div>
        
        <div class="sensor-card">
            <div class="sensor-label">Kualitas Udara (MQ-135)</div>
            <div class="sensor-value">)" +
                String(airQuality) + R"(% | )" + String(airQualityRaw) + R"( ADC</div>
            <div class="sensor-label">Status: )" +
                String(airQualityGood ? "🟢 Baik" : "🔴 Buruk") + R"( | ~)" + String((int)calculatePPM(airQualityRaw)) + R"( ppm</div>
        </div>
        
        <div class="status )" +
                String(pumpActive ? "on" : "off") + R"(">
            Status Pompa: )" +
                String(pumpActive ? "🟢 AKTIF" : "🔴 MATI") + R"(
        </div>
        
        <div class="controls">
            <a href="/pump/on" class="btn btn-primary">🚰 Nyalakan Pompa</a>
            <a href="/pump/off" class="btn btn-secondary">🛑 Matikan Pompa</a>
        </div>
        
        <div class="stats">
            <h3>📊 Statistik Sistem</h3>
            <p><strong>Total Penyiraman:</strong> )" +
                String(wateringCount) + R"( kali</p>
            <p><strong>Total Waktu Pompa:</strong> )" +
                String(totalWateringTime) + R"( detik</p>
            <p><strong>Batas Penyiraman:</strong> ≤)" +
                String(MOISTURE_THRESHOLD) + R"(%</p>
            <p><strong>Batas Berhenti:</strong> ≥)" +
                String(MOISTURE_STOP) + R"(%</p>
            <p><strong>Uptime:</strong> )" +
                String(millis() / 1000) + R"( detik</p>
            <p><strong>WiFi Status:</strong> )" +
                String(WiFi.status() == WL_CONNECTED ? "Terhubung (" + WiFi.localIP().toString() + ")" : "AP Mode") + R"(</p>
            <p><strong>MQTT Status:</strong> )" +
                String(mqttClient.connected() ? "🟢 Connected" : "🔴 Disconnected") + R"(</p>
        </div>
        
        <div class="stats">
            <h3>🌐 Remote Access</h3>
            <p><strong>MQTT Topics:</strong></p>
            <p style="font-size: 12px; font-family: monospace;">
            📤 Sensor: )" +
                String(TOPIC_SENSORS) + R"(<br>
            📥 Pump Control: )" +
                String(TOPIC_PUMP_COMMAND) + R"(<br>
            📊 System Status: )" +
                String(TOPIC_SYSTEM_STATUS) + R"(<br>
            📋 Logs: )" +
                String(TOPIC_LOGS) + R"(
            </p>
            <p><strong>Pump Commands:</strong> Send "ON"/"OFF" or {"pump":true/false}</p>
        </div>
        
        <div class="refresh">
            🔄 Halaman akan refresh otomatis setiap 30 detik<br>
            <a href="/api/data" style="color: #4caf50;">📡 Data JSON</a>
        </div>
    </div>
</body>
</html>
)";

  server.send(200, "text/html", html);
}

// ========== WEB SERVER - KONTROL POMPA ==========
void handlePumpOn()
{
  startPump();
  server.sendHeader("Location", "/");
  server.send(302, "text/plain", "Pompa dinyalakan");
}

void handlePumpOff()
{
  stopPump();
  server.sendHeader("Location", "/");
  server.send(302, "text/plain", "Pompa dimatikan");
}

// ========== API JSON ==========
void handleAPI()
{
  JsonDocument json;

  json["soilMoisture"] = soilMoisture;
  json["temperature"] = temperature;
  json["humidity"] = humidity;
  json["airQuality"] = airQuality;
  json["airQualityRaw"] = airQualityRaw;
  json["airQualityGood"] = airQualityGood;
  json["airQualityPPM"] = (int)calculatePPM(airQualityRaw);
  json["pumpActive"] = pumpActive;
  json["wateringCount"] = wateringCount;
  json["totalWateringTime"] = totalWateringTime;
  json["moistureThreshold"] = MOISTURE_THRESHOLD;
  json["moistureStop"] = MOISTURE_STOP;
  json["uptime"] = millis() / 1000;
  json["mqttConnected"] = mqttClient.connected();
  json["wifiConnected"] = WiFi.status() == WL_CONNECTED;
  json["ipAddress"] = WiFi.localIP().toString();
  json["mqttTopics"]["sensors"] = TOPIC_SENSORS;
  json["mqttTopics"]["pumpCommand"] = TOPIC_PUMP_COMMAND;
  json["mqttTopics"]["pumpStatus"] = TOPIC_PUMP_STATUS;
  json["mqttTopics"]["systemStatus"] = TOPIC_SYSTEM_STATUS;

  String response;
  serializeJson(json, response);

  server.send(200, "application/json", response);
}

// ========== STATUS LED ==========
void updateStatusLED()
{
  static unsigned long lastBlink = 0;
  static bool ledState = false;

  if (pumpActive)
  {
    // Kedip cepat saat pompa aktif
    if (millis() - lastBlink >= 250)
    {
      ledState = !ledState;
      digitalWrite(LED_STATUS_PIN, ledState);
      lastBlink = millis();
    }
  }
  else
  {
    // Nyala terus saat standby
    digitalWrite(LED_STATUS_PIN, HIGH);
  }
}

// ========== SETUP ==========
void setup()
{
  Serial.begin(115200);
  Serial.println("\n🌱 AgroHygra - Smart Irrigation System");
  Serial.println("=====================================");

  // Setup pin
  pinMode(RELAY_PIN, OUTPUT);
  pinMode(LED_STATUS_PIN, OUTPUT);
  pinMode(MQ135_DO_PIN, INPUT); // MQ-135 digital output pin
  // Pastikan pompa mati saat start (respect relay active-low option)
  if (RELAY_ACTIVE_LOW)
    digitalWrite(RELAY_PIN, HIGH);
  else
    digitalWrite(RELAY_PIN, LOW);
  // LED off initially
  digitalWrite(LED_STATUS_PIN, LOW);

  // Inisialisasi sensor DHT
  dht.begin();

  // Koneksi WiFi
  Serial.print("🔗 Menghubungkan ke WiFi: ");
  Serial.println(ssid);

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  int wifiTimeout = 0;
  while (WiFi.status() != WL_CONNECTED && wifiTimeout < 30)
  {
    delay(1000);
    Serial.print(".");
    wifiTimeout++;
  }

  if (WiFi.status() == WL_CONNECTED)
  {
    Serial.println("\n✅ WiFi terhubung!");
    Serial.print("📡 IP Address: ");
    Serial.println(WiFi.localIP());
    Serial.println("🌐 Akses web interface di: http://" + WiFi.localIP().toString());

    // Setup mDNS (bisa akses via http://agrohygra.local)
    if (MDNS.begin("agrohygra"))
    {
      Serial.println("🌐 mDNS started: http://agrohygra.local");
    }
  }
  else
  {
    Serial.println("\n❌ WiFi gagal terhubung! Memulai Access Point mode...");
    // Setup sebagai Access Point untuk akses langsung
    WiFi.mode(WIFI_AP);
    const char *ap_ssid = "AgroHygra-ESP32";
    const char *ap_password = "agrohygra123";

    bool apStarted = WiFi.softAP(ap_ssid, ap_password);
    if (apStarted)
    {
      IPAddress apIP = WiFi.softAPIP();
      Serial.println("✅ Access Point berhasil dimulai!");
      Serial.println("📡 SSID: " + String(ap_ssid));
      Serial.println("🔐 Password: " + String(ap_password));
      Serial.println("🌐 IP Address: " + apIP.toString());
      Serial.println("🌐 Akses web interface di: http://" + apIP.toString());
    }
    else
    {
      Serial.println("❌ Gagal memulai Access Point!");
    }
  }

  // Setup web server routes
  server.on("/", handleRoot);
  server.on("/pump/on", handlePumpOn);
  server.on("/pump/off", handlePumpOff);
  server.on("/api/data", handleAPI);

  // Start web server
  server.begin();
  Serial.println("🌐 Web server dimulai pada port 80");

  // Setup MQTT (only if WiFi connected)
  if (WiFi.status() == WL_CONNECTED)
  {
    setupMqtt();
    connectToMqtt();
  }

  // Print final access instructions
  Serial.println("=====================================");
  Serial.println("✅ SISTEM SIAP DIGUNAKAN!");
  if (WiFi.status() == WL_CONNECTED)
  {
    Serial.println("📱 Buka browser dan kunjungi:");
    Serial.println("   http://" + WiFi.localIP().toString());
    Serial.println("   atau http://agrohygra.local");
  }
  else
  {
    Serial.println("📱 Hubungkan ke WiFi 'AgroHygra-ESP32'");
    Serial.println("🔐 Password: agrohygra123");
    Serial.println("📱 Lalu buka browser ke: http://192.168.4.1");
  }
  Serial.println("=====================================");

  // Baca sensor pertama kali
  readAllSensors();

  Serial.println("✅ Sistem siap!");
  Serial.printf("🌾 Kelembapan tanah saat ini: %d%%\n", soilMoisture);
  Serial.printf("🌡️  Suhu: %.1f°C, Kelembapan udara: %.1f%%\n", temperature, humidity);
  Serial.printf("🌬️  Kualitas udara: %d%% (ADC: %d, Status: %s, ~%d ppm)\n",
                airQuality, airQualityRaw, airQualityGood ? "Baik" : "Buruk", (int)calculatePPM(airQualityRaw));
  Serial.println("=====================================");

  // Configure ADC for soil sensor and MQ-135 (ESP32)
  // ADC1 channel pins (32-39, but 34 is input-only and suitable for analog)
  analogReadResolution(12); // 12-bit resolution (0-4095)
  // Set attenuation to allow reading full range with sensor values (0-3.3V)
  analogSetPinAttenuation(SOIL_PIN, ADC_11db);
  analogSetPinAttenuation(MQ135_AO_PIN, ADC_11db);
}

// ========== LOOP UTAMA ==========
void loop()
{
  // Handle web server
  server.handleClient();

  // Monitor WiFi connection (ESP32 specific)
  static unsigned long lastWiFiCheck = 0;
  if (millis() - lastWiFiCheck >= 30000)
  { // Check every 30 seconds
    lastWiFiCheck = millis();
    if (WiFi.getMode() == WIFI_STA && WiFi.status() != WL_CONNECTED)
    {
      Serial.println("⚠️  WiFi terputus, mencoba reconnect...");
      WiFi.reconnect();
    }
  }

  // Baca sensor secara berkala
  if (millis() - lastSensorRead >= (SENSOR_READ_INTERVAL * 1000UL))
  {
    lastSensorRead = millis();

    readAllSensors();

    // Tampilkan data ke Serial Monitor
    Serial.printf("📊 Tanah: %d%% | Suhu: %.1f°C | RH: %.1f%% | Udara: %d%% | Pompa: %s\n",
                  soilMoisture, temperature, humidity, airQuality,
                  pumpActive ? "ON" : "OFF");

    // Jalankan logika irigasi otomatis
    autoIrrigation();
  }

  // Handle MQTT connection and publishing
  if (WiFi.status() == WL_CONNECTED)
  {
    // Try to connect to MQTT if not connected
    connectToMqtt();

    // Keep MQTT connection alive
    mqttClient.loop();

    // Publish sensor data periodically
    if (mqttClient.connected() && millis() - lastMqttSensorPublish >= MQTT_SENSOR_INTERVAL)
    {
      lastMqttSensorPublish = millis();
      publishSensorData();
    }
    
    // Debug MQTT connection status every 10 seconds
    static unsigned long lastMqttDebug = 0;
    if (millis() - lastMqttDebug >= 10000) {
      lastMqttDebug = millis();
      Serial.printf("🔧 MQTT Status: Connected=%s, State=%d\n", 
                    mqttClient.connected() ? "YES" : "NO", mqttClient.state());
    }
  }
  else
  {
    Serial.println("⚠️  WiFi not connected - skipping MQTT");
  }

  // Update status LED
  updateStatusLED();

  // Delay kecil untuk stabilitas
  delay(100);
}
#include <Arduino.h>

// Configuration
#include "config/Config.h"

// Sensors
#include "sensors/DHTSensor.h"
#include "sensors/NPKSensor.h"
#include "sensors/MQ135Sensor.h"
#include "sensors/TDSSensor.h"

// Controllers
#include "controllers/PumpController.h"

// Display
#include "display/LCDDisplay.h"

// Network
#include "network/WiFiManager.h"
#include "network/MQTTManager.h"
#include "network/WebServer.h"

// ========== GLOBAL OBJECTS ==========
// Sensors
DHTSensor dhtSensor(DHT_PIN, DHT_TYPE);
HardwareSerial RS485Serial(2);
NPKSensor npkSensor(&RS485Serial, RS485_DE_RE);
MQ135Sensor mq135Sensor(MQ135_AO_PIN, MQ135_DO_PIN);
TDSSensor tdsSensor(TDS_PIN);

// Controllers
PumpController pumpController(RELAY_PIN, RELAY_ACTIVE_LOW);

// Display
LCDDisplay lcdDisplay;

// Network
WiFiManager wifiManager;
MQTTManager mqttManager;
AgroWebServer webServer(80);

// ========== TIMING VARIABLES ==========
unsigned long lastSensorRead = 0;
unsigned long lastNPKRead = 0;

// ========== SENSOR DATA CACHE ==========
int soilMoisture = 0;
float temperature = 0;
float humidity = 0;
int airQuality = 0;
int airQualityRaw = 0;
bool airQualityGood = true;
int tdsValue = 0;
int tdsRaw = 0;

// ========== STATUS LED CONTROL ==========
void updateStatusLED() {
  static unsigned long lastBlink = 0;
  static bool ledState = false;
  
  if (wifiManager.isConnected() && mqttManager.isConnected()) {
    // Slow blink: all OK
    if (millis() - lastBlink > 2000) {
      ledState = !ledState;
      digitalWrite(LED_STATUS_PIN, ledState);
      lastBlink = millis();
    }
  } else if (wifiManager.isConnected()) {
    // Fast blink: WiFi OK, MQTT disconnected
    if (millis() - lastBlink > 500) {
      ledState = !ledState;
      digitalWrite(LED_STATUS_PIN, ledState);
      lastBlink = millis();
    }
  } else {
    // Solid on: WiFi disconnected
    digitalWrite(LED_STATUS_PIN, HIGH);
  }
}

// ========== READ ALL SENSORS ==========
void readAllSensors() {
  // Read DHT sensor
  dhtSensor.read();
  temperature = dhtSensor.getTemperature();
  humidity = dhtSensor.getHumidity();
  
  // Read MQ135 air quality sensor
  mq135Sensor.read();
  airQuality = mq135Sensor.getQualityPercent();
  airQualityRaw = mq135Sensor.getRawValue();
  airQualityGood = mq135Sensor.isGoodQuality();
  
  // Read TDS sensor
  tdsSensor.read();
  tdsValue = tdsSensor.getTDS();
  tdsRaw = tdsSensor.getRawValue();
  
  // Get soil moisture from NPK sensor
  if (npkSensor.isAvailable() && npkSensor.getHumidity() >= 0) {
    soilMoisture = (int)npkSensor.getHumidity();
  } else {
    soilMoisture = 0;
  }
  
  // Update consecutive dry counter for pump controller
  if (soilMoisture <= MOISTURE_THRESHOLD) {
    int count = pumpController.getConsecutiveDryCount() + 1;
    pumpController.setConsecutiveDryCount(count);
  } else {
    pumpController.setConsecutiveDryCount(0);
  }
  
  Serial.printf("📊 Sensors: Soil=%d%% Temp=%.1f°C Hum=%.1f%% Air=%d%% TDS=%dppm\n",
                soilMoisture, temperature, humidity, airQuality, tdsValue);
}

// ========== PUBLISH MQTT DATA ==========
void publishSensorData() {
  if (!mqttManager.isConnected()) return;
  
  JsonDocument doc;
  doc["device"] = MQTT_CLIENT_ID;
  doc["time"] = millis() / 1000;
  doc["soil"] = soilMoisture;
  doc["temp"] = temperature;
  doc["hum"] = humidity;
  doc["air"] = airQuality;
  doc["airRaw"] = airQualityRaw;
  doc["airGood"] = airQualityGood;
  doc["ppm"] = (int)mq135Sensor.getPPM();
  doc["pump"] = pumpController.isPumpActive();
  doc["count"] = pumpController.getWateringCount();
  doc["wtime"] = pumpController.getTotalWateringTime();
  doc["uptime"] = millis() / 1000;
  doc["tdsRaw"] = tdsRaw;
  doc["tds"] = tdsValue;
  
  // NPK Sensor data
  if (npkSensor.isAvailable()) {
    doc["npk"]["n"] = npkSensor.getNitrogen();
    doc["npk"]["p"] = npkSensor.getPhosphorus();
    doc["npk"]["k"] = npkSensor.getPotassium();
    doc["npk"]["ph"] = npkSensor.getPH();
    doc["npk"]["ec"] = npkSensor.getEC();
    doc["npk"]["soilTemp"] = npkSensor.getTemperature();
  }
  
  mqttManager.publishSensorData(doc);
}

// ========== SETUP ==========
void setup() {
  // Initialize Serial
  Serial.begin(115200);
  delay(1000);
  Serial.println("\n\n");
  Serial.println("================================");
  Serial.println("   🌱 AgroHygra System v2.0    ");
  Serial.println("   Modular Architecture        ");
  Serial.println("================================");
  
  // Initialize status LED
  pinMode(LED_STATUS_PIN, OUTPUT);
  digitalWrite(LED_STATUS_PIN, HIGH);
  
  // Initialize sensors
  Serial.println("\n📡 Initializing sensors...");
  dhtSensor.begin();
  npkSensor.begin();
  mq135Sensor.begin();
  tdsSensor.begin();
  
  // Initialize pump controller
  Serial.println("\n💧 Initializing pump controller...");
  pumpController.begin();
  
  // Initialize LCD
  Serial.println("\n📺 Initializing LCD display...");
  if (lcdDisplay.begin(0x27)) {
    lcdDisplay.showMessage("AgroHygra v2.0", "Starting...");
  } else {
    Serial.println("⚠️  LCD initialization failed, trying 0x3F...");
    if (lcdDisplay.begin(0x3F)) {
      lcdDisplay.showMessage("AgroHygra v2.0", "Starting...");
    } else {
      Serial.println("❌ LCD not found!");
    }
  }
  
  // Initialize WiFi
  Serial.println("\n📡 Initializing WiFi...");
  wifiManager.begin();
  wifiManager.loadCredentials();
  
  if (!wifiManager.connect()) {
    Serial.println("⚠️  WiFi connection failed, starting AP mode...");
    wifiManager.startAPMode();
    lcdDisplay.showMessage("AP Mode", "192.168.4.1");
  } else {
    lcdDisplay.showMessage("WiFi Connected", WiFi.localIP().toString());
  }
  
  delay(2000);
  
  // Initialize MQTT
  Serial.println("\n📨 Initializing MQTT...");
  mqttManager.begin();
  mqttManager.setPumpController(&pumpController);
  
  // Initialize Web Server
  Serial.println("\n🌐 Initializing web server...");
  webServer.begin();
  webServer.setWiFiManager(&wifiManager);
  webServer.setPumpController(&pumpController);
  webServer.setSensors(&npkSensor, &mq135Sensor, &tdsSensor, &dhtSensor);
  
  Serial.println("\n================================");
  Serial.println("✅ System initialized!");
  Serial.println("================================");
  Serial.printf("Web Interface: http://%s\n", 
                wifiManager.isAPMode() ? "192.168.4.1" : WiFi.localIP().toString().c_str());
  Serial.println("================================\n");
}

// ========== MAIN LOOP ==========
void loop() {
  // Read NPK sensor every second
  if (millis() - lastNPKRead >= NPK_READ_INTERVAL) {
    lastNPKRead = millis();
    npkSensor.readSensor();
  }
  
  // Read all other sensors every 2 seconds
  if (millis() - lastSensorRead >= (SENSOR_READ_INTERVAL * 1000UL)) {
    lastSensorRead = millis();
    readAllSensors();
    
    // Publish sensor data via MQTT
    publishSensorData();
    
    // Update web server with latest data
    webServer.updateSensorData(soilMoisture, temperature, humidity, 
                              airQuality, airQualityRaw, airQualityGood,
                              tdsValue, tdsRaw);
    
    // Update LCD display
    lcdDisplay.setData(soilMoisture, temperature, humidity, 
                      airQuality, airQualityGood, tdsValue,
                      pumpController.isPumpActive(), 
                      pumpController.getPumpRunTime() / 1000,
                      pumpController.getWateringCount(),
                      mqttManager.isConnected(),
                      wifiManager.isAPMode(),
                      wifiManager.getSSID(),
                      wifiManager.isAPMode() ? "192.168.4.1" : WiFi.localIP().toString());
  }
  
  // Update LCD (handles its own timing)
  lcdDisplay.update();
  
  // Auto irrigation logic
  pumpController.autoIrrigate(soilMoisture);
  
  // Handle network tasks
  mqttManager.loop();
  webServer.loop();
  
  // Update status LED
  updateStatusLED();
  
  // Small delay to prevent watchdog timeout
  delay(10);
}

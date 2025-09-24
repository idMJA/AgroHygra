#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <ESPmDNS.h>
#include <ArduinoJson.h>
#include "DHT.h"

// ========== KONFIGURASI WIFI ==========
const char* ssid = "YOUR_WIFI_SSID";        // Ganti dengan nama WiFi Anda
const char* password = "YOUR_WIFI_PASSWORD"; // Ganti dengan password WiFi Anda

// ========== KONFIGURASI PIN ==========
#define DHT_PIN 14          // Pin sensor suhu/kelembapan udara (DHT22)
#define DHT_TYPE DHT22      // Jenis sensor DHT
#define SOIL_PIN 34         // Pin analog untuk sensor kelembapan tanah
#define RELAY_PIN 27        // Pin relay untuk pompa air
#define LED_STATUS_PIN 2    // Pin LED built-in ESP32 untuk indikator status

// ========== KONFIGURASI SENSOR & SISTEM ==========
const int SOIL_DRY_VALUE = 3000;    // Nilai ADC saat tanah kering (kalibrasi)
const int SOIL_WET_VALUE = 1000;    // Nilai ADC saat tanah basah (kalibrasi)
const int MOISTURE_THRESHOLD = 30;   // Batas kelembapan untuk mulai menyiram (%)
const int MOISTURE_STOP = 70;        // Batas kelembapan untuk berhenti menyiram (%)
const int MAX_PUMP_TIME = 60;        // Maksimal pompa nyala (detik) untuk safety
const int SENSOR_READ_INTERVAL = 10; // Interval pembacaan sensor (detik)

// ========== VARIABEL GLOBAL ==========
DHT dht(DHT_PIN, DHT_TYPE);
WebServer server(80);

unsigned long lastSensorRead = 0;
bool pumpActive = false;
unsigned long pumpStartTime = 0;
int soilMoisture = 0;
float temperature = 0;
float humidity = 0;

// Statistik sistem
unsigned long totalWateringTime = 0;
int wateringCount = 0;

// ========== FUNGSI SENSOR ==========
int readSoilMoisture() {
  int rawValue = analogRead(SOIL_PIN);
  
  // Konversi ke persentase (0-100%)
  int moisture = map(rawValue, SOIL_DRY_VALUE, SOIL_WET_VALUE, 0, 100);
  moisture = constrain(moisture, 0, 100); // Batasi nilai 0-100%
  
  return moisture;
}

void readAllSensors() {
  soilMoisture = readSoilMoisture();
  temperature = dht.readTemperature();
  humidity = dht.readHumidity();
  
  // Cek apakah pembacaan DHT berhasil
  if (isnan(temperature)) temperature = 0;
  if (isnan(humidity)) humidity = 0;
}

// ========== FUNGSI KONTROL POMPA ==========
void startPump() {
  if (!pumpActive) {
    digitalWrite(RELAY_PIN, HIGH);
    pumpActive = true;
    pumpStartTime = millis();
    wateringCount++;
    Serial.println("🚰 POMPA DINYALAKAN - Kelembapan tanah rendah");
  }
}

void stopPump() {
  if (pumpActive) {
    digitalWrite(RELAY_PIN, LOW);
    pumpActive = false;
    
    // Catat waktu penyiraman
    unsigned long wateringDuration = (millis() - pumpStartTime) / 1000;
    totalWateringTime += wateringDuration;
    
    Serial.printf("🛑 POMPA DIMATIKAN - Durasi: %lu detik\n", wateringDuration);
  }
}

// ========== LOGIKA IRIGASI OTOMATIS ==========
void autoIrrigation() {
  // Mulai penyiraman jika kelembapan rendah
  if (!pumpActive && soilMoisture <= MOISTURE_THRESHOLD) {
    startPump();
  }
  
  // Hentikan penyiraman jika kelembapan sudah cukup
  if (pumpActive && soilMoisture >= MOISTURE_STOP) {
    stopPump();
  }
  
  // Safety: hentikan pompa jika sudah terlalu lama nyala
  if (pumpActive && (millis() - pumpStartTime) >= (MAX_PUMP_TIME * 1000UL)) {
    Serial.println("⚠️  SAFETY: Pompa dimatikan karena timeout!");
    stopPump();
  }
}

// ========== WEB SERVER - HALAMAN UTAMA ==========
void handleRoot() {
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
            <div class="sensor-value">)" + String(soilMoisture) + R"(%</div>
        </div>
        
        <div class="sensor-card">
            <div class="sensor-label">Suhu Udara</div>
            <div class="sensor-value">)" + String(temperature, 1) + R"(°C</div>
        </div>
        
        <div class="sensor-card">
            <div class="sensor-label">Kelembapan Udara</div>
            <div class="sensor-value">)" + String(humidity, 1) + R"(%</div>
        </div>
        
        <div class="status )" + String(pumpActive ? "on" : "off") + R"(">
            Status Pompa: )" + String(pumpActive ? "🟢 AKTIF" : "🔴 MATI") + R"(
        </div>
        
        <div class="controls">
            <a href="/pump/on" class="btn btn-primary">🚰 Nyalakan Pompa</a>
            <a href="/pump/off" class="btn btn-secondary">🛑 Matikan Pompa</a>
        </div>
        
        <div class="stats">
            <h3>📊 Statistik Sistem</h3>
            <p><strong>Total Penyiraman:</strong> )" + String(wateringCount) + R"( kali</p>
            <p><strong>Total Waktu Pompa:</strong> )" + String(totalWateringTime) + R"( detik</p>
            <p><strong>Batas Penyiraman:</strong> ≤)" + String(MOISTURE_THRESHOLD) + R"(%</p>
            <p><strong>Batas Berhenti:</strong> ≥)" + String(MOISTURE_STOP) + R"(%</p>
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
void handlePumpOn() {
  startPump();
  server.sendHeader("Location", "/");
  server.send(302, "text/plain", "Pompa dinyalakan");
}

void handlePumpOff() {
  stopPump();
  server.sendHeader("Location", "/");
  server.send(302, "text/plain", "Pompa dimatikan");
}

// ========== API JSON ==========
void handleAPI() {
  DynamicJsonDocument json(512);
  
  json["soilMoisture"] = soilMoisture;
  json["temperature"] = temperature;
  json["humidity"] = humidity;
  json["pumpActive"] = pumpActive;
  json["wateringCount"] = wateringCount;
  json["totalWateringTime"] = totalWateringTime;
  json["moistureThreshold"] = MOISTURE_THRESHOLD;
  json["moistureStop"] = MOISTURE_STOP;
  json["uptime"] = millis() / 1000;
  
  String response;
  serializeJson(json, response);
  
  server.send(200, "application/json", response);
}

// ========== STATUS LED ==========
void updateStatusLED() {
  static unsigned long lastBlink = 0;
  static bool ledState = false;
  
  if (pumpActive) {
    // Kedip cepat saat pompa aktif
    if (millis() - lastBlink >= 250) {
      ledState = !ledState;
      digitalWrite(LED_STATUS_PIN, ledState);
      lastBlink = millis();
    }
  } else {
    // Nyala terus saat standby
    digitalWrite(LED_STATUS_PIN, HIGH);
  }
}

// ========== SETUP ==========
void setup() {
  Serial.begin(115200);
  Serial.println("\n🌱 AgroHygra - Smart Irrigation System");
  Serial.println("=====================================");
  
  // Setup pin
  pinMode(RELAY_PIN, OUTPUT);
  pinMode(LED_STATUS_PIN, OUTPUT);
  digitalWrite(RELAY_PIN, LOW);  // Pastikan pompa mati saat start
  digitalWrite(LED_STATUS_PIN, LOW);
  
  // Inisialisasi sensor DHT
  dht.begin();
  
  // Koneksi WiFi
  Serial.print("🔗 Menghubungkan ke WiFi: ");
  Serial.println(ssid);
  
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  
  int wifiTimeout = 0;
  while (WiFi.status() != WL_CONNECTED && wifiTimeout < 30) {
    delay(1000);
    Serial.print(".");
    wifiTimeout++;
  }
  
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\n✅ WiFi terhubung!");
    Serial.print("📡 IP Address: ");
    Serial.println(WiFi.localIP());
    
    // Setup mDNS (bisa akses via http://agrohygra.local)
    if (MDNS.begin("agrohygra")) {
      Serial.println("🌐 mDNS started: http://agrohygra.local");
    }
  } else {
    Serial.println("\n❌ WiFi gagal terhubung! Sistem tetap berjalan tanpa web interface.");
  }
  
  // Setup web server routes
  server.on("/", handleRoot);
  server.on("/pump/on", handlePumpOn);
  server.on("/pump/off", handlePumpOff);
  server.on("/api/data", handleAPI);
  
  // Start web server
  server.begin();
  Serial.println("🌐 Web server dimulai");
  
  // Baca sensor pertama kali
  readAllSensors();
  
  Serial.println("✅ Sistem siap!");
  Serial.printf("🌾 Kelembapan tanah saat ini: %d%%\n", soilMoisture);
  Serial.printf("🌡️  Suhu: %.1f°C, Kelembapan udara: %.1f%%\n", temperature, humidity);
  Serial.println("=====================================");
}

// ========== LOOP UTAMA ==========
void loop() {
  // Handle web server
  server.handleClient();
  
  // Baca sensor secara berkala
  if (millis() - lastSensorRead >= (SENSOR_READ_INTERVAL * 1000UL)) {
    lastSensorRead = millis();
    
    readAllSensors();
    
    // Tampilkan data ke Serial Monitor
    Serial.printf("📊 Tanah: %d%% | Suhu: %.1f°C | RH: %.1f%% | Pompa: %s\n", 
                  soilMoisture, temperature, humidity, 
                  pumpActive ? "ON" : "OFF");
    
    // Jalankan logika irigasi otomatis
    autoIrrigation();
  }
  
  // Update status LED
  updateStatusLED();
  
  // Delay kecil untuk stabilitas
  delay(100);
}
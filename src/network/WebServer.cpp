#include "WebServer.h"
#include "network/WiFiManager.h"
#include "controllers/PumpController.h"
#include "sensors/NPKSensor.h"
#include "sensors/MQ135Sensor.h"
#include "sensors/TDSSensor.h"
#include "sensors/DHTSensor.h"

AgroWebServer::AgroWebServer(int port) 
  : server(port), wifiManager(nullptr), pumpController(nullptr),
    npkSensor(nullptr), mq135Sensor(nullptr), tdsSensor(nullptr), dhtSensor(nullptr),
    soilMoisture(0), temperature(0), humidity(0), airQuality(0), 
    airQualityRaw(0), airQualityGood(true), tdsValue(0), tdsRaw(0) {
}

void AgroWebServer::begin() {
  // Setup routes
  server.on("/", [this]() { this->handleRoot(); });
  server.on("/wifi", [this]() { this->handleWiFiSetup(); });
  server.on("/wifi/scan", [this]() { this->handleWiFiScan(); });
  server.on("/wifi/save", HTTP_POST, [this]() { this->handleWiFiSave(); });
  server.on("/wifi/clear", HTTP_POST, [this]() { this->handleWiFiClear(); });
  server.on("/pump/on", [this]() { this->handlePumpOn(); });
  server.on("/pump/off", [this]() { this->handlePumpOff(); });
  server.on("/api", [this]() { this->handleAPI(); });
  
  server.begin();
  
  // Start mDNS
  if (MDNS.begin("agrohygra")) {
    Serial.println("✅ mDNS responder started: http://agrohygra.local");
    MDNS.addService("http", "tcp", 80);
  }
  
  Serial.println("✅ Web server started");
}

void AgroWebServer::setWiFiManager(WiFiManager *manager) {
  wifiManager = manager;
}

void AgroWebServer::setPumpController(PumpController *controller) {
  pumpController = controller;
}

void AgroWebServer::setSensors(NPKSensor *npk, MQ135Sensor *mq135, TDSSensor *tds, DHTSensor *dht) {
  npkSensor = npk;
  mq135Sensor = mq135;
  tdsSensor = tds;
  dhtSensor = dht;
}

void AgroWebServer::updateSensorData(int soil, float temp, float hum, int air, 
                                    int airRaw, bool airGood, int tds, int tdsRaw) {
  soilMoisture = soil;
  temperature = temp;
  humidity = hum;
  airQuality = air;
  airQualityRaw = airRaw;
  airQualityGood = airGood;
  tdsValue = tds;
  this->tdsRaw = tdsRaw;
}

void AgroWebServer::loop() {
  server.handleClient();
}

void AgroWebServer::handleRoot() {
  String apBanner = "";
  if (wifiManager && wifiManager->isAPMode()) {
    apBanner = "<div style='background:#ff9800;padding:15px;margin:10px 0;border-radius:5px;'>"
               "<strong>⚠️ AP MODE ACTIVE</strong><br>"
               "Connect to: <strong>AgroHygra-Setup</strong> (password: agrohygra123)<br>"
               "<a href='/wifi' style='color:#fff;text-decoration:underline;'>Configure WiFi</a>"
               "</div>";
  }
  
  String pumpStatus = (pumpController && pumpController->isPumpActive()) ? "ON" : "OFF";
  String pumpColor = (pumpController && pumpController->isPumpActive()) ? "#4caf50" : "#f44336";
  
  String html = "<!DOCTYPE html><html><head>"
                "<meta charset='UTF-8'><meta name='viewport' content='width=device-width,initial-scale=1'>"
                "<title>AgroHygra Dashboard</title>"
                "<style>body{font-family:Arial,sans-serif;margin:0;padding:20px;background:#f0f0f0}"
                ".container{max-width:800px;margin:0 auto;background:#fff;padding:20px;border-radius:10px;box-shadow:0 2px 5px rgba(0,0,0,0.1)}"
                "h1{color:#4caf50;text-align:center}h2{color:#333;border-bottom:2px solid #4caf50;padding-bottom:5px}"
                ".sensor-grid{display:grid;grid-template-columns:repeat(auto-fit,minmax(200px,1fr));gap:15px;margin:20px 0}"
                ".sensor-card{background:#f9f9f9;padding:15px;border-radius:8px;border-left:4px solid #4caf50}"
                ".sensor-value{font-size:24px;font-weight:bold;color:#333;margin:5px 0}"
                ".sensor-label{font-size:14px;color:#666}"
                ".button{display:inline-block;padding:10px 20px;margin:5px;background:#4caf50;color:#fff;text-decoration:none;"
                "border:none;border-radius:5px;cursor:pointer;font-size:16px}"
                ".button:hover{background:#45a049}.button.off{background:#f44336}.button.off:hover{background:#da190b}"
                ".status{padding:10px;border-radius:5px;margin:10px 0;text-align:center;font-weight:bold}"
                "</style>"
                "<script>setInterval(()=>fetch('/api').then(r=>r.json()).then(d=>{"
                "document.getElementById('soil').innerText=d.soil+'%';"
                "document.getElementById('temp').innerText=d.temp+'°C';"
                "document.getElementById('hum').innerText=d.humidity+'%';"
                "document.getElementById('air').innerText=d.airQuality+'%';"
                "document.getElementById('tds').innerText=d.tds+' ppm';"
                "document.getElementById('pumpStatus').innerText=d.pump?'ON':'OFF';"
                "document.getElementById('pumpStatus').style.background=d.pump?'#4caf50':'#f44336';"
                "}),2000);</script>"
                "</head><body><div class='container'>"
                "<h1>🌱 AgroHygra Dashboard</h1>" +
                apBanner +
                "<h2>Sensor Readings</h2>"
                "<div class='sensor-grid'>"
                "<div class='sensor-card'><div class='sensor-label'>Soil Moisture</div>"
                "<div class='sensor-value' id='soil'>" + String(soilMoisture) + "%</div></div>"
                "<div class='sensor-card'><div class='sensor-label'>Temperature</div>"
                "<div class='sensor-value' id='temp'>" + String(temperature, 1) + "°C</div></div>"
                "<div class='sensor-card'><div class='sensor-label'>Humidity</div>"
                "<div class='sensor-value' id='hum'>" + String(humidity, 1) + "%</div></div>"
                "<div class='sensor-card'><div class='sensor-label'>Air Quality</div>"
                "<div class='sensor-value' id='air'>" + String(airQuality) + "%</div></div>"
                "<div class='sensor-card'><div class='sensor-label'>TDS</div>"
                "<div class='sensor-value' id='tds'>" + String(tdsValue) + " ppm</div></div>"
                "</div>";
  
  // NPK Sensor data if available
  if (npkSensor && npkSensor->isAvailable()) {
    html += "<h2>NPK Sensor (7-in-1)</h2><div class='sensor-grid'>"
            "<div class='sensor-card'><div class='sensor-label'>Nitrogen (N)</div>"
            "<div class='sensor-value'>" + String(npkSensor->getNitrogen(), 0) + " mg/kg</div></div>"
            "<div class='sensor-card'><div class='sensor-label'>Phosphorus (P)</div>"
            "<div class='sensor-value'>" + String(npkSensor->getPhosphorus(), 0) + " mg/kg</div></div>"
            "<div class='sensor-card'><div class='sensor-label'>Potassium (K)</div>"
            "<div class='sensor-value'>" + String(npkSensor->getPotassium(), 0) + " mg/kg</div></div>"
            "<div class='sensor-card'><div class='sensor-label'>pH</div>"
            "<div class='sensor-value'>" + String(npkSensor->getPH(), 1) + "</div></div>"
            "<div class='sensor-card'><div class='sensor-label'>EC</div>"
            "<div class='sensor-value'>" + String(npkSensor->getEC(), 2) + " mS/cm</div></div>"
            "</div>";
  }
  
  html += "<h2>Pump Control</h2>"
          "<div class='status' id='pumpStatus' style='background:" + pumpColor + ";color:#fff'>" + pumpStatus + "</div>"
          "<div style='text-align:center'>"
          "<a href='/pump/on' class='button'>Turn Pump ON</a>"
          "<a href='/pump/off' class='button off'>Turn Pump OFF</a>"
          "</div>";
  
  if (pumpController) {
    html += "<p style='text-align:center;color:#666'>"
            "Watering Count: " + String(pumpController->getWateringCount()) + " | "
            "Total Time: " + String(pumpController->getTotalWateringTime()) + "s"
            "</p>";
  }
  
  html += "<h2>Settings</h2>"
          "<div style='text-align:center'>"
          "<a href='/wifi' class='button'>WiFi Settings</a>"
          "<a href='/api' class='button'>API</a>"
          "</div>"
          "</div></body></html>";
  
  server.send(200, "text/html", html);
}

void AgroWebServer::handleWiFiSetup() {
  String networks = wifiManager ? wifiManager->scanNetworks() : "";
  
  String html = "<!DOCTYPE html><html><head>"
                "<meta charset='UTF-8'><meta name='viewport' content='width=device-width,initial-scale=1'>"
                "<title>WiFi Setup - AgroHygra</title>"
                "<style>body{font-family:Arial,sans-serif;margin:0;padding:20px;background:#f0f0f0}"
                ".container{max-width:600px;margin:0 auto;background:#fff;padding:20px;border-radius:10px}"
                "h1{color:#4caf50;text-align:center}label{display:block;margin:10px 0 5px;font-weight:bold}"
                "input,select{width:100%;padding:10px;margin-bottom:15px;border:1px solid #ddd;border-radius:5px;box-sizing:border-box}"
                ".button{width:100%;padding:12px;background:#4caf50;color:#fff;border:none;border-radius:5px;"
                "cursor:pointer;font-size:16px;margin-top:10px}.button:hover{background:#45a049}"
                ".button.danger{background:#f44336}.button.danger:hover{background:#da190b}"
                "</style></head><body><div class='container'>"
                "<h1>🌐 WiFi Configuration</h1>"
                "<form method='POST' action='/wifi/save'>"
                "<label>Select Network:</label>"
                "<select name='ssid' id='ssid'>" + networks + "</select>"
                "<label>Password:</label>"
                "<input type='password' name='password' placeholder='WiFi password'>"
                "<button type='submit' class='button'>Save & Connect</button>"
                "</form>"
                "<form method='POST' action='/wifi/clear'>"
                "<button type='submit' class='button danger'>Clear Credentials</button>"
                "</form>"
                "<p style='text-align:center'><a href='/'>Back to Dashboard</a></p>"
                "</div></body></html>";
  
  server.send(200, "text/html", html);
}

void AgroWebServer::handleWiFiScan() {
  String networks = wifiManager ? wifiManager->scanNetworks() : "Error";
  server.send(200, "text/plain", networks);
}

void AgroWebServer::handleWiFiSave() {
  if (server.hasArg("ssid") && server.hasArg("password") && wifiManager) {
    String ssid = server.arg("ssid");
    String password = server.arg("password");
    
    wifiManager->saveCredentials(ssid, password);
    
    String html = "<!DOCTYPE html><html><head><meta charset='UTF-8'>"
                  "<meta http-equiv='refresh' content='10;url=/'>"
                  "<style>body{font-family:Arial;text-align:center;padding:50px;background:#f0f0f0}"
                  ".message{background:#fff;padding:30px;border-radius:10px;display:inline-block}</style>"
                  "</head><body><div class='message'>"
                  "<h1 style='color:#4caf50'>✅ WiFi Saved!</h1>"
                  "<p>Credentials saved. Device will restart...</p>"
                  "<p>Redirecting in 10 seconds...</p>"
                  "</div></body></html>";
    
    server.send(200, "text/html", html);
    delay(2000);
    ESP.restart();
  } else {
    server.send(400, "text/plain", "Missing parameters");
  }
}

void AgroWebServer::handleWiFiClear() {
  if (wifiManager) {
    wifiManager->clearCredentials();
  }
  
  String html = "<!DOCTYPE html><html><head><meta charset='UTF-8'>"
                "<meta http-equiv='refresh' content='5;url=/wifi'>"
                "<style>body{font-family:Arial;text-align:center;padding:50px;background:#f0f0f0}"
                ".message{background:#fff;padding:30px;border-radius:10px;display:inline-block}</style>"
                "</head><body><div class='message'>"
                "<h1 style='color:#f44336'>🗑️ Credentials Cleared</h1>"
                "<p>WiFi credentials have been deleted.</p>"
                "<p>Redirecting to WiFi setup...</p>"
                "</div></body></html>";
  
  server.send(200, "text/html", html);
}

void AgroWebServer::handlePumpOn() {
  if (pumpController) {
    pumpController->start();
  }
  server.sendHeader("Location", "/");
  server.send(303);
}

void AgroWebServer::handlePumpOff() {
  if (pumpController) {
    pumpController->stop();
  }
  server.sendHeader("Location", "/");
  server.send(303);
}

void AgroWebServer::handleAPI() {
  JsonDocument doc;
  doc["device"] = "AgroHygra-ESP32";
  doc["timestamp"] = millis() / 1000;
  doc["soil"] = soilMoisture;
  doc["temp"] = temperature;
  doc["humidity"] = humidity;
  doc["airQuality"] = airQuality;
  doc["airRaw"] = airQualityRaw;
  doc["airGood"] = airQualityGood;
  doc["tds"] = tdsValue;
  doc["tdsRaw"] = this->tdsRaw;
  doc["pump"] = pumpController ? pumpController->isPumpActive() : false;
  doc["wateringCount"] = pumpController ? pumpController->getWateringCount() : 0;
  doc["totalWateringTime"] = pumpController ? pumpController->getTotalWateringTime() : 0;
  
  // NPK data if available
  if (npkSensor && npkSensor->isAvailable()) {
    doc["npk"]["n"] = npkSensor->getNitrogen();
    doc["npk"]["p"] = npkSensor->getPhosphorus();
    doc["npk"]["k"] = npkSensor->getPotassium();
    doc["npk"]["ph"] = npkSensor->getPH();
    doc["npk"]["ec"] = npkSensor->getEC();
    doc["npk"]["temp"] = npkSensor->getTemperature();
    doc["npk"]["moisture"] = npkSensor->getHumidity();
  }
  
  String json;
  serializeJson(doc, json);
  server.send(200, "application/json", json);
}

#include "LCDDisplay.h"

LCDDisplay::LCDDisplay() : lcd(nullptr), currentPage(0), lastUpdate(0) {
  // Initialize display data
  data.soilMoisture = 0;
  data.temperature = 0;
  data.humidity = 0;
  data.airQuality = 0;
  data.airQualityGood = true;
  data.tdsValue = 0;
  data.pumpActive = false;
  data.pumpRunTime = 0;
  data.wateringCount = 0;
  data.mqttConnected = false;
  data.isAPMode = false;
}

bool LCDDisplay::begin(uint8_t address) {
  // Cast address to pcf8574Address enum type
  lcd = new LiquidCrystal_I2C((pcf8574Address)address);
  lcd->begin(16, 2);
  lcd->backlight();
  lcd->clear();
  
  Serial.printf("✅ LCD initialized at address 0x%02X\n", address);
  return true;
}

void LCDDisplay::setData(int soilMoisture, float temperature, float humidity,
                        int airQuality, bool airQualityGood, int tdsValue,
                        bool pumpActive, int pumpRunTime, int wateringCount,
                        bool mqttConnected, bool isAPMode, String ssid, String ipAddress) {
  data.soilMoisture = soilMoisture;
  data.temperature = temperature;
  data.humidity = humidity;
  data.airQuality = airQuality;
  data.airQualityGood = airQualityGood;
  data.tdsValue = tdsValue;
  data.pumpActive = pumpActive;
  data.pumpRunTime = pumpRunTime;
  data.wateringCount = wateringCount;
  data.mqttConnected = mqttConnected;
  data.isAPMode = isAPMode;
  data.ssid = ssid;
  data.ipAddress = ipAddress;
}

void LCDDisplay::update() {
  if (!lcd) return;
  
  if (millis() - lastUpdate < LCD_UPDATE_INTERVAL) return;
  lastUpdate = millis();
  
  lcd->clear();
  
  switch (currentPage) {
    case 0: // Soil moisture & Temperature
      lcd->setCursor(0, 0);
      lcd->print("Soil:");
      lcd->print(data.soilMoisture);
      lcd->print("%");
      if (data.soilMoisture <= MOISTURE_THRESHOLD) {
        lcd->print(" DRY");
      } else if (data.soilMoisture >= MOISTURE_STOP) {
        lcd->print(" WET");
      }
      
      lcd->setCursor(0, 1);
      lcd->print("Temp:");
      lcd->print(data.temperature, 1);
      lcd->print((char)223); // degree symbol
      lcd->print("C");
      break;
      
    case 1: // Humidity & Air Quality
      lcd->setCursor(0, 0);
      lcd->print("Humidity:");
      lcd->print(data.humidity, 1);
      lcd->print("%");
      
      lcd->setCursor(0, 1);
      lcd->print("Air:");
      lcd->print(data.airQuality);
      lcd->print("% ");
      lcd->print(data.airQualityGood ? "OK" : "BAD");
      break;
      
    case 2: // TDS & Pump Status
      lcd->setCursor(0, 0);
      lcd->print("TDS:");
      lcd->print(data.tdsValue);
      lcd->print(" ppm");
      
      lcd->setCursor(0, 1);
      lcd->print("Pump:");
      if (data.pumpActive) {
        lcd->print("ON ");
        lcd->print(data.pumpRunTime);
        lcd->print("s");
      } else {
        lcd->print("OFF");
      }
      break;
      
    case 3: // WiFi & MQTT Status
      lcd->setCursor(0, 0);
      if (data.isAPMode) {
        lcd->print("WiFi:AP Mode");
      } else if (WiFi.status() == WL_CONNECTED) {
        lcd->print("WiFi:OK ");
        lcd->print(WiFi.RSSI());
        lcd->print("dB");
      } else {
        lcd->print("WiFi:Disc");
      }
      
      lcd->setCursor(0, 1);
      lcd->print("MQTT:");
      lcd->print(data.mqttConnected ? "ON" : "OFF");
      lcd->print(" #");
      lcd->print(data.wateringCount);
      break;
      
    case 4: // SSID & IP Address
      lcd->setCursor(0, 0);
      if (data.isAPMode) {
        lcd->print("AP:AgroHygra");
      } else if (WiFi.status() == WL_CONNECTED) {
        String displaySSID = data.ssid;
        if (displaySSID.length() > 16) {
          displaySSID = displaySSID.substring(0, 16);
        }
        lcd->print(displaySSID);
      } else {
        lcd->print("WiFi: No Conn");
      }
      
      lcd->setCursor(0, 1);
      lcd->print(data.ipAddress);
      break;
  }
  
  // Cycle through pages
  currentPage = (currentPage + 1) % LCD_PAGES;
}

void LCDDisplay::showMessage(String line1, String line2) {
  if (!lcd) return;
  lcd->clear();
  lcd->setCursor(0, 0);
  lcd->print(line1);
  lcd->setCursor(0, 1);
  lcd->print(line2);
}

void LCDDisplay::clear() {
  if (lcd) lcd->clear();
}

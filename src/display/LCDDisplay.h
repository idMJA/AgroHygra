#ifndef LCD_DISPLAY_H
#define LCD_DISPLAY_H

#include <Arduino.h>
#include <LiquidCrystal_I2C.h>
#include <WiFi.h>
#include "config/Config.h"

class LCDDisplay {
private:
  LiquidCrystal_I2C *lcd;
  int currentPage;
  unsigned long lastUpdate;
  
  // Data to display (pointers will be set externally)
  struct DisplayData {
    int soilMoisture;
    float temperature;
    float humidity;
    int airQuality;
    bool airQualityGood;
    int tdsValue;
    bool pumpActive;
    int pumpRunTime;
    int wateringCount;
    bool mqttConnected;
    bool isAPMode;
    String ssid;
    String ipAddress;
  } data;

public:
  LCDDisplay();
  
  bool begin(uint8_t address = 0x27);
  void update();
  void setData(int soilMoisture, float temperature, float humidity, 
               int airQuality, bool airQualityGood, int tdsValue,
               bool pumpActive, int pumpRunTime, int wateringCount,
               bool mqttConnected, bool isAPMode, String ssid, String ipAddress);
  
  void showMessage(String line1, String line2);
  void clear();
};

#endif // LCD_DISPLAY_H

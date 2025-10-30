#ifndef WIFI_MANAGER_H
#define WIFI_MANAGER_H

#include <Arduino.h>
#include <WiFi.h>
#include <Preferences.h>

class WiFiManager {
private:
  Preferences preferences;
  String savedSSID;
  String savedPassword;
  bool apMode;
  
public:
  WiFiManager();
  
  void begin();
  bool connect();
  void startAPMode();
  String scanNetworks();
  
  // Credentials management
  void saveCredentials(String ssid, String password);
  void loadCredentials();
  void clearCredentials();
  
  // Getters
  String getSSID() const { return savedSSID; }
  String getPassword() const { return savedPassword; }
  bool isAPMode() const { return apMode; }
  bool isConnected() const { return WiFi.status() == WL_CONNECTED; }
};

#endif // WIFI_MANAGER_H

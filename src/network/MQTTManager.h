#ifndef MQTT_MANAGER_H
#define MQTT_MANAGER_H

#include <Arduino.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include "config/Config.h"

// Forward declarations
class PumpController;

class MQTTManager {
private:
  WiFiClient *wifiClient;
  PubSubClient mqttClient;
  unsigned long lastReconnect;
  unsigned long lastPublish;
  PumpController *pumpController;
  
  void callback(char *topic, byte *payload, unsigned int length);
  static MQTTManager *instance; // For static callback
  
public:
  MQTTManager();
  
  void begin();
  void setPumpController(PumpController *controller);
  bool connect();
  void loop();
  
  void publishSensorData(JsonDocument &doc);
  void publishPumpStatus(bool active);
  void publishLog(String message);
  
  bool isConnected() { return mqttClient.connected(); }
  
  // Static callback wrapper
  static void staticCallback(char *topic, byte *payload, unsigned int length);
};

#endif // MQTT_MANAGER_H

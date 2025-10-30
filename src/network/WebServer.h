#ifndef WEB_SERVER_H
#define WEB_SERVER_H

#include <Arduino.h>
#include <WebServer.h>
#include <ESPmDNS.h>
#include <ArduinoJson.h>

// Forward declarations
class WiFiManager;
class PumpController;
class NPKSensor;
class MQ135Sensor;
class TDSSensor;
class DHTSensor;

class AgroWebServer {
private:
  WebServer server;
  WiFiManager *wifiManager;
  PumpController *pumpController;
  
  // Sensor pointers
  NPKSensor *npkSensor;
  MQ135Sensor *mq135Sensor;
  TDSSensor *tdsSensor;
  DHTSensor *dhtSensor;
  
  // Current sensor values (updated externally)
  int soilMoisture;
  float temperature;
  float humidity;
  int airQuality;
  int airQualityRaw;
  bool airQualityGood;
  int tdsValue;
  int tdsRaw;
  
  // Route handlers
  void handleRoot();
  void handleWiFiSetup();
  void handleWiFiScan();
  void handleWiFiSave();
  void handleWiFiClear();
  void handlePumpOn();
  void handlePumpOff();
  void handleAPI();
  
public:
  AgroWebServer(int port = 80);
  
  void begin();
  void setWiFiManager(WiFiManager *manager);
  void setPumpController(PumpController *controller);
  void setSensors(NPKSensor *npk, MQ135Sensor *mq135, TDSSensor *tds, DHTSensor *dht);
  void updateSensorData(int soil, float temp, float hum, int air, int airRaw, 
                       bool airGood, int tds, int tdsRaw);
  void loop();
};

#endif // WEB_SERVER_H

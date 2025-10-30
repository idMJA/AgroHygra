#ifndef DHT_SENSOR_H
#define DHT_SENSOR_H

#include <Arduino.h>
#include "DHT.h"
#include "config/Config.h"

class DHTSensor {
private:
  DHT dht;
  float temperature;
  float humidity;

public:
  DHTSensor(uint8_t pin, uint8_t type);
  
  void begin();
  void read();
  
  // Getters
  float getTemperature() const { return temperature; }
  float getHumidity() const { return humidity; }
};

#endif // DHT_SENSOR_H

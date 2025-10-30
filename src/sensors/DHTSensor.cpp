#include "DHTSensor.h"

DHTSensor::DHTSensor(uint8_t pin, uint8_t type) 
  : dht(pin, type), temperature(0), humidity(0) {
}

void DHTSensor::begin() {
  dht.begin();
  Serial.println("✅ DHT Sensor initialized");
}

void DHTSensor::read() {
  temperature = dht.readTemperature();
  humidity = dht.readHumidity();
  
  // Check if readings are valid
  if (isnan(temperature)) temperature = 0;
  if (isnan(humidity)) humidity = 0;
}

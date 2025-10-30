#ifndef NPK_SENSOR_H
#define NPK_SENSOR_H

#include <Arduino.h>
#include "config/Config.h"

class NPKSensor {
private:
  HardwareSerial *serial;
  uint8_t deRePin;
  
  // Sensor data
  float nitrogen;
  float phosphorus;
  float potassium;
  float ph;
  float ec;
  float temperature;
  float humidity;
  bool available;
  
  // Helper functions
  uint16_t calculateCRC16(uint8_t *data, uint8_t length);
  void createRequestFrame(uint8_t *frame, uint8_t deviceAddress, uint8_t functionCode,
                         uint16_t registerAddress, uint16_t registerCount);
  uint16_t readRegister(uint16_t registerAddress);

public:
  NPKSensor(HardwareSerial *serialPort, uint8_t deRePin);
  
  void begin();
  bool readSensor();
  
  // Getters
  float getNitrogen() const { return nitrogen; }
  float getPhosphorus() const { return phosphorus; }
  float getPotassium() const { return potassium; }
  float getPH() const { return ph; }
  float getEC() const { return ec; }
  float getTemperature() const { return temperature; }
  float getHumidity() const { return humidity; }
  bool isAvailable() const { return available; }
};

#endif // NPK_SENSOR_H

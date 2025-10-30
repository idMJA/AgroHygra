#ifndef MQ135_SENSOR_H
#define MQ135_SENSOR_H

#include <Arduino.h>
#include "config/Config.h"

class MQ135Sensor {
private:
  uint8_t aoPin;
  uint8_t doPin;
  
  int rawValue;
  int qualityPercent;
  bool digitalStatus;
  
  int readRaw();

public:
  MQ135Sensor(uint8_t aoPin, uint8_t doPin);
  
  void begin();
  void read();
  
  // Getters
  int getRawValue() const { return rawValue; }
  int getQualityPercent() const { return qualityPercent; }
  bool isGoodQuality() const { return digitalStatus; }
  float getPPM() const;
};

#endif // MQ135_SENSOR_H

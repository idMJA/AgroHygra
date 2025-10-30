#ifndef TDS_SENSOR_H
#define TDS_SENSOR_H

#include <Arduino.h>
#include "config/Config.h"

class TDSSensor {
private:
  uint8_t pin;
  int rawValue;
  int tdsValue;

public:
  TDSSensor(uint8_t pin);
  
  void begin();
  void read();
  
  // Getters
  int getRawValue() const { return rawValue; }
  int getTDS() const { return tdsValue; }
};

#endif // TDS_SENSOR_H

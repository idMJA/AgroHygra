#ifndef PUMP_CONTROLLER_H
#define PUMP_CONTROLLER_H

#include <Arduino.h>
#include "config/Config.h"

class PumpController {
private:
  uint8_t relayPin;
  bool activeLow;
  bool isActive;
  unsigned long startTime;
  unsigned long totalWateringTime;
  int wateringCount;
  int consecutiveDryCount;

public:
  PumpController(uint8_t pin, bool activeLow);
  
  void begin();
  void start();
  void stop();
  void checkSafety();
  void autoIrrigate(int soilMoisture);
  
  // Getters
  bool isPumpActive() const { return isActive; }
  unsigned long getPumpRunTime() const { return isActive ? (millis() - startTime) : 0; }
  unsigned long getTotalWateringTime() const { return totalWateringTime; }
  int getWateringCount() const { return wateringCount; }
  int getConsecutiveDryCount() const { return consecutiveDryCount; }
  
  // Setters for external control
  void setConsecutiveDryCount(int count) { consecutiveDryCount = count; }
};

#endif // PUMP_CONTROLLER_H

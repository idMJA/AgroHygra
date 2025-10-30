#include "MQ135Sensor.h"

MQ135Sensor::MQ135Sensor(uint8_t aoPin, uint8_t doPin) 
  : aoPin(aoPin), doPin(doPin), rawValue(0), qualityPercent(0), digitalStatus(true) {
}

void MQ135Sensor::begin() {
  pinMode(aoPin, INPUT);
  pinMode(doPin, INPUT);
  Serial.println("✅ MQ135 Air Quality Sensor initialized");
}

int MQ135Sensor::readRaw() {
  // Read multiple samples and average for stability
  const int samples = 10;
  long sum = 0;
  for (int i = 0; i < samples; i++) {
    sum += analogRead(aoPin);
    delay(10);
  }
  return sum / samples;
}

void MQ135Sensor::read() {
  rawValue = readRaw();
  
  // Convert to quality percentage (0-100, lower is better)
  qualityPercent = map(rawValue, MQ135_CLEAN_AIR_VALUE, MQ135_POLLUTED_THRESHOLD, 0, 100);
  qualityPercent = constrain(qualityPercent, 0, 100);
  
  // Digital status (LOW = good air quality for most modules)
  digitalStatus = (digitalRead(doPin) == LOW);
}

float MQ135Sensor::getPPM() const {
  if (rawValue <= 0) return 0;

  float voltage = (rawValue / MQ135_ADC_MAX) * MQ135_VOLTAGE_REF;
  float rs = ((MQ135_VOLTAGE_REF * MQ135_RL_VALUE) / voltage) - MQ135_RL_VALUE;
  float ratio = rs / MQ135_RO_CLEAN_AIR;

  // Simplified conversion to CO2 equivalent ppm
  float ppm = 116.6020682 * pow(ratio, -2.769034857);
  return constrain(ppm, 10, 2000);
}

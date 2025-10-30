#include "TDSSensor.h"

TDSSensor::TDSSensor(uint8_t pin) : pin(pin), rawValue(0), tdsValue(0) {
}

void TDSSensor::begin() {
  pinMode(pin, INPUT);
  Serial.println("✅ TDS Sensor initialized");
}

void TDSSensor::read() {
  // Read multiple samples and average
  const int samples = 10;
  long sum = 0;
  for (int i = 0; i < samples; i++) {
    sum += analogRead(pin);
    delay(10);
  }
  rawValue = sum / samples;
  
  // Convert raw ADC to voltage (12-bit ADC, 3.3V reference)
  float voltage = (rawValue / 4095.0) * 3.3;
  
  // Convert to TDS (ppm) - requires calibration
  tdsValue = (int)(voltage * TDS_K);
  
  Serial.printf("📡 TDS: %d ppm (raw: %d)\n", tdsValue, rawValue);
}

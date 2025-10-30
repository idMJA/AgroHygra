#include "PumpController.h"

PumpController::PumpController(uint8_t pin, bool activeLow) 
  : relayPin(pin), activeLow(activeLow), isActive(false), 
    startTime(0), totalWateringTime(0), wateringCount(0), 
    consecutiveDryCount(0) {
}

void PumpController::begin() {
  pinMode(relayPin, OUTPUT);
  // Set relay to OFF state initially
  digitalWrite(relayPin, activeLow ? HIGH : LOW);
  Serial.println("✅ Pump Controller initialized");
}

void PumpController::start() {
  if (!isActive) {
    digitalWrite(relayPin, activeLow ? LOW : HIGH);
    isActive = true;
    startTime = millis();
    wateringCount++;
    consecutiveDryCount = 0;
    Serial.printf("💧 Pump STARTED (count: %d)\n", wateringCount);
  }
}

void PumpController::stop() {
  if (isActive) {
    digitalWrite(relayPin, activeLow ? HIGH : LOW);
    unsigned long runtime = millis() - startTime;
    totalWateringTime += runtime / 1000;
    isActive = false;
    Serial.printf("💧 Pump STOPPED (ran for %lu seconds, total: %lu seconds)\n", 
                  runtime / 1000, totalWateringTime);
  }
}

void PumpController::checkSafety() {
  // Safety: stop pump if running too long
  if (isActive && (millis() - startTime) >= (MAX_PUMP_TIME * 1000UL)) {
    stop();
    Serial.println("⚠️  Pump auto-stopped (max time reached)");
  }
}

void PumpController::autoIrrigate(int soilMoisture) {
  // Safety: do not auto-start immediately after boot
  if (millis() < BOOT_SAFE_DELAY) {
    if (soilMoisture <= MOISTURE_THRESHOLD) {
      Serial.println("⏳ Boot delay active, auto-irrigation pending...");
    }
    return;
  }

  // Start watering if moisture is low
  if (!isActive && soilMoisture <= MOISTURE_THRESHOLD) {
    consecutiveDryCount++;
    if (consecutiveDryCount >= REQUIRED_CONSECUTIVE_DRY) {
      start();
    } else {
      Serial.printf("⏳ Waiting for %d more dry readings before starting pump\n", 
                    REQUIRED_CONSECUTIVE_DRY - consecutiveDryCount);
    }
  }

  // Stop watering if moisture is sufficient
  if (isActive && soilMoisture >= MOISTURE_STOP) {
    stop();
  }

  // Check safety limits
  checkSafety();
}

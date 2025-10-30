#include "NPKSensor.h"

NPKSensor::NPKSensor(HardwareSerial *serialPort, uint8_t deRePin) 
  : serial(serialPort), deRePin(deRePin), 
    nitrogen(0), phosphorus(0), potassium(0), 
    ph(0), ec(0), temperature(0), humidity(0), 
    available(false) {
}

void NPKSensor::begin() {
  pinMode(deRePin, OUTPUT);
  digitalWrite(deRePin, LOW); // Start in receive mode
  serial->begin(NPK_BAUD_RATE, SERIAL_8N1, RS485_RX, RS485_TX);
  delay(100);
  Serial.println("✅ NPK Sensor initialized");
}

uint16_t NPKSensor::calculateCRC16(uint8_t *data, uint8_t length) {
  uint16_t crc = 0xFFFF;
  for (uint8_t pos = 0; pos < length; pos++) {
    crc ^= (uint16_t)data[pos];
    for (uint8_t i = 8; i != 0; i--) {
      if ((crc & 0x0001) != 0) {
        crc >>= 1;
        crc ^= 0xA001;
      } else {
        crc >>= 1;
      }
    }
  }
  return crc;
}

void NPKSensor::createRequestFrame(uint8_t *frame, uint8_t deviceAddress, 
                                   uint8_t functionCode, uint16_t registerAddress, 
                                   uint16_t registerCount) {
  frame[0] = deviceAddress;
  frame[1] = functionCode;
  frame[2] = (registerAddress >> 8) & 0xFF;
  frame[3] = registerAddress & 0xFF;
  frame[4] = (registerCount >> 8) & 0xFF;
  frame[5] = registerCount & 0xFF;

  uint16_t crc = calculateCRC16(frame, 6);
  frame[6] = crc & 0xFF;
  frame[7] = (crc >> 8) & 0xFF;
}

uint16_t NPKSensor::readRegister(uint16_t registerAddress) {
  uint8_t requestFrame[8];
  uint8_t responseFrame[9];
  uint16_t value = 0;

  // Clear serial buffer
  while (serial->available()) {
    serial->read();
  }

  // Create request frame
  createRequestFrame(requestFrame, NPK_SENSOR_ADDRESS, MODBUS_READ_HOLDING_REGISTERS,
                    registerAddress, 1);

  // Set to transmit mode
  digitalWrite(deRePin, HIGH);
  delay(10);

  // Send request
  serial->write(requestFrame, sizeof(requestFrame));
  serial->flush();

  // Set to receive mode
  digitalWrite(deRePin, LOW);
  delay(10);

  // Wait for response (timeout 1000ms)
  unsigned long startTime = millis();
  uint8_t index = 0;

  while (millis() - startTime < 1000 && index < sizeof(responseFrame)) {
    if (serial->available()) {
      responseFrame[index++] = serial->read();
    }
  }

  // Check response
  if (index >= 7) {
    uint16_t receivedCRC = (responseFrame[index - 1] << 8) | responseFrame[index - 2];
    uint16_t calculatedCRC = calculateCRC16(responseFrame, index - 2);

    if (receivedCRC == calculatedCRC && responseFrame[1] == MODBUS_READ_HOLDING_REGISTERS) {
      value = (responseFrame[3] << 8) | responseFrame[4];
      return value;
    } else {
      Serial.printf("❌ NPK Reg 0x%04X: CRC error\n", registerAddress);
      return 0xFFFF;
    }
  } else {
    Serial.printf("❌ NPK Reg 0x%04X: Timeout (%d bytes)\n", registerAddress, index);
    return 0xFFFF;
  }
}

bool NPKSensor::readSensor() {
  // Read all 7 registers one by one
  uint16_t moisture_raw = readRegister(MOISTURE_REGISTER);
  delay(50);

  uint16_t temperature_raw = readRegister(TEMPERATURE_REGISTER);
  delay(50);

  uint16_t conductivity_raw = readRegister(CONDUCTIVITY_REGISTER);
  delay(50);

  uint16_t ph_raw = readRegister(PH_REGISTER);
  delay(50);

  uint16_t nitrogen_raw = readRegister(NITROGEN_REGISTER);
  delay(50);

  uint16_t phosphorus_raw = readRegister(PHOSPHORUS_REGISTER);
  delay(50);

  uint16_t potassium_raw = readRegister(POTASSIUM_REGISTER);
  delay(50);

  // Count valid readings
  int validReadings = 0;
  if (moisture_raw != 0xFFFF) validReadings++;
  if (temperature_raw != 0xFFFF) validReadings++;
  if (conductivity_raw != 0xFFFF) validReadings++;
  if (ph_raw != 0xFFFF) validReadings++;
  if (nitrogen_raw != 0xFFFF) validReadings++;
  if (phosphorus_raw != 0xFFFF) validReadings++;
  if (potassium_raw != 0xFFFF) validReadings++;

  if (validReadings >= 4) {
    // Convert raw values to actual measurements
    humidity = (moisture_raw != 0xFFFF) ? moisture_raw / 10.0 : -1.0;
    temperature = (temperature_raw != 0xFFFF) ? temperature_raw / 10.0 : -100.0;
    ec = (conductivity_raw != 0xFFFF) ? conductivity_raw / 1000.0 : -1.0;
    ph = (ph_raw != 0xFFFF) ? ph_raw / 10.0 : -1.0;
    nitrogen = (nitrogen_raw != 0xFFFF) ? nitrogen_raw : 0;
    phosphorus = (phosphorus_raw != 0xFFFF) ? phosphorus_raw : 0;
    potassium = (potassium_raw != 0xFFFF) ? potassium_raw : 0;

    available = true;

    Serial.println("=== 7-in-1 NPK Sensor Readings ===");
    Serial.printf("Moisture: %.1f%%\n", humidity);
    Serial.printf("Temperature: %.1f°C\n", temperature);
    Serial.printf("Conductivity: %.3f mS/cm\n", ec);
    Serial.printf("pH: %.1f\n", ph);
    Serial.printf("Nitrogen (N): %.0f mg/kg\n", nitrogen);
    Serial.printf("Phosphorus (P): %.0f mg/kg\n", phosphorus);
    Serial.printf("Potassium (K): %.0f mg/kg\n", potassium);
    Serial.printf("Valid readings: %d/7\n", validReadings);
    Serial.println("==================================");

    return true;
  } else {
    available = false;
    Serial.printf("❌ NPK sensor failed (only %d/7 valid readings)\n", validReadings);
    return false;
  }
}

#include "NPKSensor.h"

NPKSensor::NPKSensor(HardwareSerial *serialPort, uint8_t deRePin)
    : serial(serialPort), deRePin(deRePin),
      nitrogen(0), phosphorus(0), potassium(0),
      ph(0), ec(0), temperature(0), humidity(0),
      available(false)
{
}

void NPKSensor::begin()
{
  pinMode(deRePin, OUTPUT);
  digitalWrite(deRePin, LOW); // Start in receive mode
  serial->begin(NPK_BAUD_RATE, SERIAL_8N1, RS485_RX, RS485_TX);
  delay(100);
  Serial.println("✅ NPK Sensor initialized");
}

uint16_t NPKSensor::calculateCRC16(uint8_t *data, uint8_t length)
{
  uint16_t crc = 0xFFFF;
  for (uint8_t pos = 0; pos < length; pos++)
  {
    crc ^= (uint16_t)data[pos];
    for (uint8_t i = 8; i != 0; i--)
    {
      if ((crc & 0x0001) != 0)
      {
        crc >>= 1;
        crc ^= 0xA001;
      }
      else
      {
        crc >>= 1;
      }
    }
  }
  return crc;
}

void NPKSensor::createRequestFrame(uint8_t *frame, uint8_t deviceAddress,
                                   uint8_t functionCode, uint16_t registerAddress,
                                   uint16_t registerCount)
{
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

uint16_t NPKSensor::readRegister(uint16_t registerAddress)
{
  uint8_t requestFrame[8];
  uint8_t responseFrame[9];
  uint16_t value = 0;

  // Clear serial buffer
  while (serial->available())
  {
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

  // Debug: Show what we sent
  Serial.printf("📤 NPK Reg 0x%04X: Sent [", registerAddress);
  for (int i = 0; i < 8; i++)
  {
    Serial.printf("%02X ", requestFrame[i]);
  }
  Serial.println("]");

  // Set to receive mode
  digitalWrite(deRePin, LOW);
  delay(100); // Increased delay to allow sensor to respond

  // Wait for response (timeout 500ms - increased responsiveness)
  unsigned long startTime = millis();
  uint8_t index = 0;

  while (millis() - startTime < 500 && index < sizeof(responseFrame))
  {
    if (serial->available())
    {
      responseFrame[index++] = serial->read();
    }
  }

  // Debug: Show what we received
  Serial.printf("📥 NPK Reg 0x%04X: Recv [", registerAddress);
  for (int i = 0; i < index; i++)
  {
    Serial.printf("%02X ", responseFrame[i]);
  }
  Serial.printf("] (%d bytes)\n", index);

  // Modbus response format for reading holding registers:
  // [Address:1][Function:1][ByteCount:1][Data_H:1][Data_L:1][CRC_L:1][CRC_H:1]
  // Minimum 7 bytes for a single register read
  if (index >= 7)
  {
    // Verify frame structure
    if (responseFrame[0] != NPK_SENSOR_ADDRESS || responseFrame[1] != MODBUS_READ_HOLDING_REGISTERS)
    {
      Serial.printf("❌ NPK Reg 0x%04X: Invalid response header (addr=%02X, func=%02X)\n",
                    registerAddress, responseFrame[0], responseFrame[1]);
      return 0xFFFF;
    }

    uint8_t byteCount = responseFrame[2];
    if (byteCount != 2)
    {
      Serial.printf("❌ NPK Reg 0x%04X: Invalid byte count %d\n", registerAddress, byteCount);
      return 0xFFFF;
    }

    uint16_t receivedCRC = (responseFrame[index - 1] << 8) | responseFrame[index - 2];
    uint16_t calculatedCRC = calculateCRC16(responseFrame, index - 2);

    if (receivedCRC == calculatedCRC)
    {
      // Data is in responseFrame[3] (high byte) and responseFrame[4] (low byte)
      value = (responseFrame[3] << 8) | responseFrame[4];
      Serial.printf("✓ NPK Reg 0x%04X: 0x%04X (%u)\n", registerAddress, value, value);
      return value;
    }
    else
    {
      Serial.printf("❌ NPK Reg 0x%04X: CRC error (got 0x%04X, calc 0x%04X)\n",
                    registerAddress, receivedCRC, calculatedCRC);
      return 0xFFFF;
    }
  }
  else
  {
    Serial.printf("❌ NPK Reg 0x%04X: Timeout - got %d bytes, need ≥7\n", registerAddress, index);
    return 0xFFFF;
  }
}

bool NPKSensor::readSensor()
{
  // Read all 7 registers one by one
  uint16_t moisture_raw = readRegister(MOISTURE_REGISTER);
  delay(100);

  uint16_t temperature_raw = readRegister(TEMPERATURE_REGISTER);
  delay(100);

  uint16_t conductivity_raw = readRegister(CONDUCTIVITY_REGISTER);
  delay(100);

  uint16_t ph_raw = readRegister(PH_REGISTER);
  delay(100);

  uint16_t nitrogen_raw = readRegister(NITROGEN_REGISTER);
  delay(100);

  uint16_t phosphorus_raw = readRegister(PHOSPHORUS_REGISTER);
  delay(100);

  uint16_t potassium_raw = readRegister(POTASSIUM_REGISTER);
  delay(100);

  // Count valid readings
  int validReadings = 0;
  if (moisture_raw != 0xFFFF)
    validReadings++;
  if (temperature_raw != 0xFFFF)
    validReadings++;
  if (conductivity_raw != 0xFFFF)
    validReadings++;
  if (ph_raw != 0xFFFF)
    validReadings++;
  if (nitrogen_raw != 0xFFFF)
    validReadings++;
  if (phosphorus_raw != 0xFFFF)
    validReadings++;
  if (potassium_raw != 0xFFFF)
    validReadings++;

  if (validReadings >= 4)
  {
    // Convert raw values to actual measurements
    // Reference: NPK_SENSOR_GUIDE.md register map
    // Register 0x0000: Humidity/Moisture (0-100%, resolution 0.1%)
    // Register 0x0001: Temperature (-40 to 80°C, resolution 0.1°C)
    // Register 0x0002: EC (0-20000 µS/cm, resolution 1 µS/cm) - convert to mS/cm by dividing by 1000
    // Register 0x0003: pH (0-14, resolution 0.1)
    // Register 0x0004: Nitrogen (0-1999 mg/kg, resolution 1 mg/kg)
    // Register 0x0005: Phosphorus (0-1999 mg/kg, resolution 1 mg/kg)
    // Register 0x0006: Potassium (0-1999 mg/kg, resolution 1 mg/kg)

    humidity = (moisture_raw != 0xFFFF) ? moisture_raw / 10.0 : -1.0;
    temperature = (temperature_raw != 0xFFFF) ? temperature_raw / 10.0 : -100.0;
    ec = (conductivity_raw != 0xFFFF) ? conductivity_raw / 1000.0 : -1.0; // µS/cm to mS/cm
    ph = (ph_raw != 0xFFFF) ? ph_raw / 10.0 : -1.0;
    nitrogen = (nitrogen_raw != 0xFFFF) ? nitrogen_raw : 0;       // Already in mg/kg
    phosphorus = (phosphorus_raw != 0xFFFF) ? phosphorus_raw : 0; // Already in mg/kg
    potassium = (potassium_raw != 0xFFFF) ? potassium_raw : 0;    // Already in mg/kg

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
  }
  else
  {
    available = false;
    Serial.printf("❌ NPK sensor failed (only %d/7 valid readings)\n", validReadings);
    return false;
  }
}

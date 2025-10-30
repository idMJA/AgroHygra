#ifndef CONFIG_H
#define CONFIG_H

#include <Arduino.h>

// ========== PIN CONFIGURATION ==========
#define DHT_PIN 4           // Temperature/humidity sensor pin (DHT11)
#define DHT_TYPE DHT11      // DHT sensor type
#define I2C_SDA 21          // I2C SDA pin for LCD (default ESP32)
#define I2C_SCL 22          // I2C SCL pin for LCD (default ESP32)

// RS485 NPK Sensor 7-in-1 pins
#define RS485_RX 16         // RO (Receiver Output) from sensor to ESP32 RX2
#define RS485_TX 17         // DI (Driver Input) from ESP32 TX2 to sensor
#define RS485_DE_RE 23      // DE (Driver Enable) and RE (Receiver Enable) tied together

// Analog sensor pins (ADC1 to avoid WiFi conflicts)
#define MQ135_AO_PIN 35     // Analog pin for MQ-135 AO (air quality analog output)
#define MQ135_DO_PIN 32     // Digital pin for MQ-135 DO (digital threshold output)
#define TDS_PIN 33          // Analog pin for TDS meter

// Control pins
#define RELAY_PIN 27        // Relay pin for water pump (IN pin)
#define RELAY_ACTIVE_LOW true // Set to true if relay module is active-low
#define LED_STATUS_PIN 2    // Built-in ESP32 LED pin for status indicator

// ========== MQTT CONFIGURATION ==========
extern const char *MQTT_HOST;
extern const int MQTT_PORT;
extern const char *MQTT_USERNAME;
extern const char *MQTT_PASSWORD;
extern const char *MQTT_CLIENT_ID;

// MQTT Topics
extern const char *TOPIC_SENSORS;
extern const char *TOPIC_PUMP_COMMAND;
extern const char *TOPIC_PUMP_STATUS;
extern const char *TOPIC_SYSTEM_STATUS;
extern const char *TOPIC_LOGS;

// ========== SENSOR CONFIGURATION ==========
// Irrigation thresholds
extern const int MOISTURE_THRESHOLD;
extern const int MOISTURE_STOP;
extern const int MAX_PUMP_TIME;
extern const int SENSOR_READ_INTERVAL;

// RS485 NPK Sensor Configuration
extern const byte NPK_SENSOR_ADDRESS;
extern const long NPK_BAUD_RATE;
extern const byte MODBUS_READ_HOLDING_REGISTERS;

// NPK Sensor Register Addresses
extern const uint16_t MOISTURE_REGISTER;
extern const uint16_t TEMPERATURE_REGISTER;
extern const uint16_t CONDUCTIVITY_REGISTER;
extern const uint16_t PH_REGISTER;
extern const uint16_t NITROGEN_REGISTER;
extern const uint16_t PHOSPHORUS_REGISTER;
extern const uint16_t POTASSIUM_REGISTER;

// Safety configuration
extern const unsigned long BOOT_SAFE_DELAY;
extern const int REQUIRED_CONSECUTIVE_DRY;

// ========== MQ-135 AIR QUALITY CONFIGURATION ==========
extern const int MQ135_CLEAN_AIR_VALUE;
extern const int MQ135_POLLUTED_THRESHOLD;
extern const float MQ135_VOLTAGE_REF;
extern const float MQ135_ADC_MAX;
extern const float MQ135_RL_VALUE;
extern const float MQ135_RO_CLEAN_AIR;

// ========== TDS CONFIGURATION ==========
extern const float TDS_K;

// ========== TIMING CONFIGURATION ==========
extern const unsigned long MQTT_SENSOR_INTERVAL;
extern const unsigned long MQTT_RECONNECT_INTERVAL;
extern const unsigned long LCD_UPDATE_INTERVAL;
extern const unsigned long NPK_READ_INTERVAL;
extern const int LCD_PAGES;

#endif // CONFIG_H

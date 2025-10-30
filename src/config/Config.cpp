#include "Config.h"

// ========== MQTT CONFIGURATION ==========
const char *MQTT_HOST = "broker.hivemq.com";
const int MQTT_PORT = 1883;
const char *MQTT_USERNAME = "";
const char *MQTT_PASSWORD = "";
const char *MQTT_CLIENT_ID = "AgroHygra-ESP32";

// MQTT Topics
const char *TOPIC_SENSORS = "agrohygra/sensors";
const char *TOPIC_PUMP_COMMAND = "agrohygra/pump/command";
const char *TOPIC_PUMP_STATUS = "agrohygra/pump/status";
const char *TOPIC_SYSTEM_STATUS = "agrohygra/system/status";
const char *TOPIC_LOGS = "agrohygra/logs";

// ========== SENSOR CONFIGURATION ==========
const int MOISTURE_THRESHOLD = 30;
const int MOISTURE_STOP = 70;
const int MAX_PUMP_TIME = 60;
const int SENSOR_READ_INTERVAL = 2;

// RS485 NPK Sensor Configuration
const byte NPK_SENSOR_ADDRESS = 0x01;
const long NPK_BAUD_RATE = 4800;
const byte MODBUS_READ_HOLDING_REGISTERS = 0x03;

// NPK Sensor Register Addresses
const uint16_t MOISTURE_REGISTER = 0x0000;
const uint16_t TEMPERATURE_REGISTER = 0x0001;
const uint16_t CONDUCTIVITY_REGISTER = 0x0002;
const uint16_t PH_REGISTER = 0x0003;
const uint16_t NITROGEN_REGISTER = 0x0004;
const uint16_t PHOSPHORUS_REGISTER = 0x0005;
const uint16_t POTASSIUM_REGISTER = 0x0006;

// Safety configuration
const unsigned long BOOT_SAFE_DELAY = 15000;
const int REQUIRED_CONSECUTIVE_DRY = 2;

// ========== MQ-135 AIR QUALITY CONFIGURATION ==========
const int MQ135_CLEAN_AIR_VALUE = 500;
const int MQ135_POLLUTED_THRESHOLD = 1500;
const float MQ135_VOLTAGE_REF = 3.3;
const float MQ135_ADC_MAX = 4095.0;
const float MQ135_RL_VALUE = 20.0;
const float MQ135_RO_CLEAN_AIR = 3.6;

// ========== TDS CONFIGURATION ==========
const float TDS_K = 500.0;

// ========== TIMING CONFIGURATION ==========
const unsigned long MQTT_SENSOR_INTERVAL = 2000;
const unsigned long MQTT_RECONNECT_INTERVAL = 5000;
const unsigned long LCD_UPDATE_INTERVAL = 2000;
const unsigned long NPK_READ_INTERVAL = 1000;
const int LCD_PAGES = 5;

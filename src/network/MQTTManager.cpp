#include "MQTTManager.h"
#include "controllers/PumpController.h"

// Static member initialization
MQTTManager *MQTTManager::instance = nullptr;

MQTTManager::MQTTManager() 
  : wifiClient(new WiFiClient()), mqttClient(*wifiClient), 
    lastReconnect(0), lastPublish(0), pumpController(nullptr) {
  instance = this;
}

void MQTTManager::begin() {
  mqttClient.setBufferSize(512);
  mqttClient.setServer(MQTT_HOST, MQTT_PORT);
  mqttClient.setCallback(staticCallback);
  Serial.printf("🔧 MQTT configured for %s:%d (buffer size: 512)\n", MQTT_HOST, MQTT_PORT);
}

void MQTTManager::setPumpController(PumpController *controller) {
  pumpController = controller;
}

void MQTTManager::staticCallback(char *topic, byte *payload, unsigned int length) {
  if (instance) {
    instance->callback(topic, payload, length);
  }
}

void MQTTManager::callback(char *topic, byte *payload, unsigned int length) {
  String message;
  for (unsigned int i = 0; i < length; i++) {
    message += (char)payload[i];
  }

  Serial.printf("📨 MQTT message received on %s: %s\n", topic, message.c_str());

  // Handle pump commands
  if (strcmp(topic, TOPIC_PUMP_COMMAND) == 0 && pumpController) {
    if (message == "ON" || message == "on" || message == "1" || message == "true") {
      pumpController->start();
      publishPumpStatus(true);
      publishLog("Pump started via MQTT");
    } else if (message == "OFF" || message == "off" || message == "0" || message == "false") {
      pumpController->stop();
      publishPumpStatus(false);
      publishLog("Pump stopped via MQTT");
    }
  }
}

bool MQTTManager::connect() {
  if (WiFi.status() != WL_CONNECTED) {
    if (WiFi.status() != WL_IDLE_STATUS) { // Only log if not already logging
      Serial.printf("⏳ WiFi not ready (status: %d), skipping MQTT connect\n", WiFi.status());
    }
    return false;
  }

  if (mqttClient.connected()) {
    return true;
  }

  if (millis() - lastReconnect < MQTT_RECONNECT_INTERVAL) {
    return false;
  }
  lastReconnect = millis();

  Serial.println("🔗 Connecting to MQTT broker...");
  Serial.printf("🔧 Broker: %s:%d\n", MQTT_HOST, MQTT_PORT);
  Serial.printf("🔗 Local IP: %s\n", WiFi.localIP().toString().c_str());

  if (mqttClient.connect(MQTT_CLIENT_ID)) {
    Serial.println("✅ MQTT connected!");
    
    // Subscribe to topics
    mqttClient.subscribe(TOPIC_PUMP_COMMAND);
    Serial.printf("📥 Subscribed to: %s\n", TOPIC_PUMP_COMMAND);
    
    publishLog("AgroHygra system connected");
    return true;
  } else {
    int state = mqttClient.state();
    Serial.printf("❌ MQTT connection failed (state: %d)\n", state);
    
    // Provide helpful error messages
    switch(state) {
      case -4: Serial.println("   → MQTT_CONNECTION_TIMEOUT"); break;
      case -3: Serial.println("   → MQTT_CONNECTION_LOST"); break;
      case -2: Serial.println("   → MQTT_CONNECT_FAILED (check broker address/port)"); break;
      case -1: Serial.println("   → MQTT_DISCONNECTED"); break;
      case 1: Serial.println("   → MQTT_CONNECT_BAD_PROTOCOL"); break;
      case 2: Serial.println("   → MQTT_CONNECT_BAD_CLIENT_ID"); break;
      case 3: Serial.println("   → MQTT_CONNECT_UNAVAILABLE"); break;
      case 4: Serial.println("   → MQTT_CONNECT_BAD_CREDENTIALS"); break;
      case 5: Serial.println("   → MQTT_CONNECT_UNAUTHORIZED"); break;
    }
    return false;
  }
}

void MQTTManager::loop() {
  if (mqttClient.connected()) {
    mqttClient.loop();
  } else {
    connect();
  }
}

void MQTTManager::publishSensorData(JsonDocument &doc) {
  if (!mqttClient.connected()) return;
  
  if (millis() - lastPublish < MQTT_SENSOR_INTERVAL) return;
  lastPublish = millis();

  String payload;
  serializeJson(doc, payload);

  bool success = mqttClient.publish(TOPIC_SENSORS, payload.c_str());
  Serial.printf("📤 Published sensor data: %s (success: %s)\n", 
                payload.c_str(), success ? "YES" : "NO");
}

void MQTTManager::publishPumpStatus(bool active) {
  if (!mqttClient.connected()) return;
  mqttClient.publish(TOPIC_PUMP_STATUS, active ? "ON" : "OFF");
}

void MQTTManager::publishLog(String message) {
  if (!mqttClient.connected()) return;
  mqttClient.publish(TOPIC_LOGS, message.c_str());
}

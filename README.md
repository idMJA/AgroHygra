# 🌱 AgroHygra - Smart Irrigation System

An intelligent ESP32-based irrigation system for automatic monitoring and control of plant watering based on soil moisture levels.

## 📋 Main Features

- **Soil Moisture Monitoring** - Resistive/capacitive sensor to measure soil water content
- **Environmental Sensors** - Air temperature and humidity using DHT22
- **Automatic Irrigation** - Water pump automatically controlled based on moisture threshold
- **Web Interface** - Monitoring and control via browser (responsive design)
- **REST API** - JSON data for integration with other applications
- **Safety Features** - Pump timeout, LED status indicator, watering statistics
- **WiFi Connectivity** - Remote access via WiFi network
- **mDNS Support** - Easy access via http://agrohygra.local

## 🔧 Hardware Components

### Microcontroller
- **ESP32 Development Board** (ESP32-DevKitC or compatible)

### Sensors
- **DHT22** - Air temperature and humidity sensor
- **Soil Moisture Sensor** - Soil moisture sensor (resistive or capacitive)

### Actuators & Outputs
- **Relay Module 5V** - For water pump control
- **Water Pump** - DC water pump 3-12V
- **Built-in ESP32 LED** - System status indicator

### Power Supply
- **Power Supply 5V/2A** - For ESP32 and relay
- **12V Adapter** (optional) - For larger water pumps

## 📐 Wiring Diagram

### Basic Pinout
```
ESP32 Pin    →    Component
=========================================
GPIO 4       →    DHT22 Data Pin
GPIO 34      →    Soil Sensor Analog Out
GPIO 27      →    Relay IN (Pump)
GPIO 2       →    LED Status (built-in)
3.3V         →    DHT22 VCC, Soil Sensor VCC
GND          →    DHT22 GND, Soil Sensor GND
5V           →    Relay VCC

Relay        →    Water Pump
=========================================
Relay COM    →    Power Supply (+) Pump
Relay NO     →    Pump (+)
GND          →    Pump (-)
```

### Detailed Wiring Instructions

#### 1. DHT22 Temperature & Humidity Sensor
```
DHT22 Pin    →    ESP32 Pin / Component
================================================
VCC (1)      →    3.3V (Power Supply)
DATA (2)     →    GPIO 4
NC (3)       →    (Not Connected)
GND (4)      →    GND (Ground)

OPTIONAL: Add 4.7kΩ pull-up resistor between DATA (GPIO 4) and 3.3V
```

**Wiring Notes:**
- Use 3.3V power supply (NOT 5V)
- Add 10μF capacitor between VCC and GND for stability
- Keep data line short to avoid noise
- Add 4.7kΩ pull-up resistor on data line if sensor is far from ESP32

#### 2. Capacitive Soil Moisture Sensor
```
Soil Sensor Pin  →    ESP32 Pin / Component
================================================
VCC              →    3.3V (Power Supply)
GND              →    GND (Ground)
AOUT             →    GPIO 34 (Analog Input)
DOUT (optional)  →    GPIO 32 (Digital Output - optional)

Note: Connect AOUT to ADC1 pin (32-39) to avoid WiFi conflicts
```

**Wiring Notes:**
- Use 3.3V power supply (some modules can handle 5V)
- AOUT provides analog reading (0-4095)
- DOUT provides digital threshold output (HIGH/LOW)
- Avoid ADC2 pins (12, 13, 14, 15, 25, 26, 27) - used by WiFi
- Add ceramic capacitor (0.1μF) near sensor for noise filtering

#### 3. MQ-135 Air Quality Sensor
```
MQ-135 Pin   →    ESP32 Pin / Component
================================================
VCC          →    3.3V or 5V (check module)
GND          →    GND (Ground)
AO (Analog)  →    GPIO 35 (Analog Input ADC1)
DO (Digital) →    GPIO 32 (Digital Input - optional)

Note: Power requirements vary by module - check datasheet
```

**Wiring Notes:**
- Requires warm-up time of 24 hours for stabilization
- AO pin provides continuous analog output (0-4095)
- DO pin provides digital threshold (set by onboard potentiometer)
- Add 100μF electrolytic capacitor near power for stability
- Calibrate in clean air for accurate readings

#### 4. TDS (Total Dissolved Solids) Sensor
```
TDS Sensor Pin   →    ESP32 Pin / Component
================================================
VCC (Red)        →    5V (Power Supply)
GND (Black)      →    GND (Ground)
AOUT (White)     →    GPIO 33 (Analog Input ADC1)

OPTIONAL: Add step-down voltage divider if output is 0-5V
  - If TDS outputs 0-5V, use voltage divider:
    - VCC (5V) → Resistor R1 (10kΩ) → GPIO 33
    - GPIO 33 → Resistor R2 (10kΩ) → GND
    - This scales 0-5V down to 0-3.3V
```

**Wiring Notes:**
- Most TDS modules operate at 5V
- Output voltage is proportional to conductivity
- Requires calibration with reference solution
- Add ceramic capacitor (0.1μF) near sensor for noise filtering
- NOTE: If module outputs 0-5V, MUST use voltage divider to protect ESP32 ADC

#### 5. I2C LCD Display (16x2)
```
LCD Pin  →    ESP32 Pin / Component
================================================
VCC      →    5V (Power Supply)
GND      →    GND (Ground)
SDA      →    GPIO 21 (I2C Data)
SCL      →    GPIO 22 (I2C Clock)

I2C Address: 0x27 or 0x3F (check your module)
```

**Wiring Notes:**
- LCD modules require 5V power supply
- I2C lines (SDA/SCL) are 3.3V from ESP32 (usually compatible)
- Optional: Add 4.7kΩ pull-up resistors on SDA and SCL to 3.3V
- Most modules have onboard potentiometer for contrast adjustment
- Backlight can be controlled via potentiometer or GPIO pin

#### 6. 5V Relay Module (for Pump Control)
```
Relay Module Pin  →    ESP32 Pin / Component
================================================
VCC               →    5V (Power Supply)
GND               →    GND (Ground)
IN (Signal)       →    GPIO 27 (Digital Output)
JD-VCC (optional) →    5V (if separate from VCC)

Relay Output (for pump):
COM               →    Power Supply (+) to Pump
NO (Normally Open)→    Pump Power (+) Input
NC (Normally Closed) → (Not used for pump control)
```

**Wiring Notes:**
- Most 5V relay modules are active-LOW (LOW = relay ON)
- When GPIO 27 = LOW → relay activates → pump runs
- When GPIO 27 = HIGH → relay deactivates → pump stops
- Add 1N4007 diode across relay coil for back-EMF protection
- Some modules have separate JD-VCC pin for isolation (optional)
- Add 100μF capacitor across relay power pins for stability

#### 7. Water Pump Power Connection
```
Power Supply    →    Relay / Pump
================================================
Power (+)       →    Relay COM (Common)
Pump (+)        →    Relay NO (Normally Open)
Pump (-)        →    Power Supply GND (Ground)
Power GND       →    GND (Common Ground with ESP32)

Safety:
- Use separate 12V/5V power supply for pump (not from ESP32)
- Ensure common ground between ESP32 and pump power supply
- Add 100-470μF capacitor across pump power pins
- Use proper gauge wires for pump current (typically 1-2A)
```

**Important Safety Notes:**
- NEVER power pump directly from ESP32 GPIO pins
- Use external power supply for pump (5-12V depending on pump)
- Always establish common ground between ESP32 and external power
- Add protection diodes for inductive loads (pump)
- Use proper fuses/circuit breakers for pump power circuit

#### 8. LED Status Indicator
```
LED (Built-in)  →    ESP32 Pin / Component
================================================
Anode (+)       →    GPIO 2 (ESP32 built-in LED)
Cathode (-)     →    GND

OR External LED:
Anode (+)       →    GPIO 2 (via 330Ω resistor)
Cathode (-)     →    GND
```

**Wiring Notes:**
- ESP32 has built-in LED on GPIO 2
- External LED requires 330Ω current-limiting resistor
- LED blinks fast (250ms) when pump is active
- LED steady on during normal operation

### Complete System Wiring Schematic

```
                          ┌─────────────────────────┐
                          │   ESP32 DevKit          │
                          │                         │
                   ┌──────┤ GPIO 4 (DHT Data)      │
                   │      │                         │
                   │      │ GPIO 34 (Soil ADC)    ├─────┐
                   │      │                         │     │
                   │      │ GPIO 35 (MQ135 ADC)   ├──┐  │
                   │      │                         │  │  │
                   │      │ GPIO 32 (MQ135 Digital)│  │  │
                   │      │                         │  │  │
                   │      │ GPIO 33 (TDS ADC)     ├─┐│  │
                   │      │                         │ ││  │
                   │      │ GPIO 27 (Relay)       ├─┐││  │
                   │      │                         │ │││  │
                   │      │ GPIO 2 (LED)          │ │││  │
                   │      │                         │ │││  │
                   │      │ GPIO 21 (I2C SDA)    ├─┐││││  │
                   │      │                         │ ││││  │
                   │      │ GPIO 22 (I2C SCL)    ├─┐│││││  │
                   │      │                         │ │││││  │
                   │      │ 3.3V                  │ │││││  │
                   │      │ 5V                    ├─┐│││││  │
                   │      │ GND                   │ │││││  │
                   │      └─────────────────────────┘ │││││  │
                   │                                  │││││  │
         ┌─────────┴─────┐                           │││││  │
         │               │                           │││││  │
      ┌──┴──┐        ┌───┴────┐                     │││││  │
      │DHT22│        │ Soil   │                     │││││  │
      └──┬──┘        │Sensor  │                     │││││  │
         │           └────────┘                     │││││  │
         │                                          │││││  │
      ┌──┴────┐     ┌──────────┐                   │││││  │
      │MQ-135 │     │   TDS    │                   │││││  │
      └────────┘     └──────────┘                   │││││  │
                                                    │││││  │
                     ┌─────────────────┐            │││││  │
                     │  I2C LCD 16x2  │            │││││  │
                     │   (0x27/0x3F)   │◄───────────┘││││  │
                     └─────────────────┘             ││││  │
                                                    ││││  │
                    ┌──────────────────┐            ││││  │
                    │ 5V Relay Module  │◄──────────┬┘│││  │
                    │                  │            │ │││  │
                    │  COM─────────────┼────────────┴─┼┼┼──┘
                    │  NO──────────┐   │              │││
                    └──────────────┼───┘              │││
                                   │                 │││
                    ┌──────────────┴────┐            │││
                    │   Water Pump      │            │││
                    │  (5-12V DC)       │            │││
                    └───────────────────┘          ┌─┘││
                                                   │  ││
                                           ┌───────┴──┼┼─ LED
                                           │         ││
                                      ┌────┴────┐    ││
                                      │ Power   │    ││
                                      │ Supply  │    ││
                                      └─────────┘    ││
                                                   ┌─┘│
                                                   │  │
                                                GND GND
```

### Pin Configuration Summary

| Pin | Function | Type | Voltage | Notes |
|-----|----------|------|---------|-------|
| GPIO 4 | DHT22 Data | Digital I/O | 3.3V | Temperature/Humidity sensor |
| GPIO 21 | I2C SDA | Digital I/O | 3.3V | LCD communication |
| GPIO 22 | I2C SCL | Digital I/O | 3.3V | LCD communication |
| GPIO 34 | Soil Moisture ADC | Analog In | 0-3.3V | Capacitive soil sensor |
| GPIO 35 | MQ135 Air Quality ADC | Analog In | 0-3.3V | Air quality sensor |
| GPIO 32 | MQ135 Digital Out | Digital In | 3.3V | Optional digital threshold |
| GPIO 33 | TDS Meter ADC | Analog In | 0-3.3V | Water quality sensor |
| GPIO 27 | Relay Control | Digital Out | 3.3V | Pump relay control (active-low) |
| GPIO 2 | LED Status | Digital Out | 3.3V | Status indicator LED |
| 3.3V | Power | Output | 3.3V | For sensors (max 500mA) |
| 5V | Power | Output | 5V | For relay and LCD |
| GND | Ground | - | 0V | Common ground |

### Important Wiring Tips

1. **Power Supply**
   - Use separate 5V/2A power supply for ESP32 and relay
   - Consider additional 12V supply for larger pumps
   - Always establish common ground between all components

2. **Analog Inputs**
   - Use only ADC1 pins (32-39) for sensors to avoid WiFi conflicts
   - Add 0.1μF capacitors near analog sensors for noise filtering
   - Keep analog cables short and away from power lines

3. **I2C Communication**
   - Optional: Add 4.7kΩ pull-up resistors on SDA/SCL to 3.3V
   - Keep I2C cable length under 1 meter
   - Use twisted pair cable for I2C signals

4. **Relay Control**
   - Always add freewheeling diode (1N4007) across relay coil
   - Use relay with active-low logic (LOW = ON) for safety
   - Add capacitor (100μF) across relay power pins

5. **Moisture & Durability**
   - Encapsulate sensor connections with heat shrink tubing
   - Use waterproof connectors where appropriate
   - Consider IP65/IP67 rated connectors for outdoor use
   - Protect electronics from direct water contact

6. **Cable Management**
   - Use proper gauge wires (AWG 22 for signals, AWG 18+ for pump power)
   - Bundle sensor cables together away from power cables
   - Label all connections clearly
   - Use strain relief connectors for durability

## ⚙️ Configuration

### 1. WiFi Setup (Access Point Mode)
The system starts in Access Point mode on first boot:
1. Connect to WiFi network: **AgroHygra-Setup** (Open Network)
2. Open browser to: `http://192.168.4.1`
3. Select your WiFi network and enter password
4. System will save credentials and restart

### 2. Soil Moisture Sensor Calibration
```cpp
const int SOIL_DRY_VALUE = 3000;    // ADC value when soil is dry
const int SOIL_WET_VALUE = 1000;    // ADC value when soil is wet
```

**Calibration Steps:**
1. Leave sensor in open air (dry) → note ADC value
2. Submerge sensor in water → note ADC value
3. Update constants `SOIL_DRY_VALUE` and `SOIL_WET_VALUE`

### 3. Irrigation Thresholds
```cpp
const int MOISTURE_THRESHOLD = 30;   // Start irrigation at ≤30%
const int MOISTURE_STOP = 70;        // Stop irrigation at ≥70%
const int MAX_PUMP_TIME = 60;        // Maximum 60 seconds (safety)
const unsigned long BOOT_SAFE_DELAY = 15000; // ms to wait after boot before auto irrigation
const int REQUIRED_CONSECUTIVE_DRY = 2;      // consecutive dry readings required before starting pump
```

### 4. MQ-135 Air Quality Sensor Configuration
```cpp
const int MQ135_CLEAN_AIR_VALUE = 500;     // Typical ADC value in clean air (calibrate in fresh air)
const int MQ135_POLLUTED_THRESHOLD = 1500; // ADC value indicating poor air quality
const float MQ135_VOLTAGE_REF = 3.3;       // Reference voltage for ADC (3.3V for ESP32)
const float MQ135_ADC_MAX = 4095.0;        // 12-bit ADC max value
const float MQ135_RL_VALUE = 20.0;         // Load resistance on sensor board (kOhm)
const float MQ135_RO_CLEAN_AIR = 3.6;      // Sensor resistance in clean air (kOhm)
```

### 5. MQTT Configuration
```cpp
const char *MQTT_HOST = "broker.hivemq.com";    // Free public broker
const int MQTT_PORT = 1883;                     // Non-TLS port for testing
const char *MQTT_CLIENT_ID = "AgroHygra-ESP32"; // Unique client ID
const unsigned long MQTT_SENSOR_INTERVAL = 2000; // Publish sensor data every 2 seconds
const unsigned long MQTT_RECONNECT_INTERVAL = 5000; // Try reconnect every 5 seconds
```

**MQTT Topics Published:**
- `agrohygra/sensors` - Sensor readings (soil moisture, temperature, humidity, air quality, TDS)
- `agrohygra/pump/status` - Pump status (ON/OFF)
- `agrohygra/system/status` - System status and device information
- `agrohygra/logs` - System logs and events

**MQTT Topics Subscribed:**
- `agrohygra/pump/command` - Receive commands to control pump (ON/OFF/JSON)

**Alternative Free MQTT Brokers:**
- `broker.hivemq.com:1883` (public, no auth, no TLS)
- `test.mosquitto.org:1883` (public, no auth, no TLS)
- For production: Use HiveMQ Cloud, CloudMQTT, or AWS IoT with TLS + authentication

### 6. TDS (Total Dissolved Solids) Sensor Configuration
```cpp
const float TDS_K = 500.0;  // Calibration multiplier (requires module-specific calibration)
// TDS voltage to ppm conversion: ppm ≈ voltage × TDS_K
// Note: Module-specific and needs proper calibration for accurate readings
```

### 7. Sensor Reading Intervals
```cpp
const int SENSOR_READ_INTERVAL = 2; // Read sensors every 2 seconds
const unsigned long LCD_UPDATE_INTERVAL = 2000; // Update LCD display every 2 seconds
```

### 8. Safety Features
- **Boot Safe Delay**: System waits 15 seconds after boot before enabling auto-irrigation
- **Consecutive Dry Readings**: Requires 2 consecutive dry readings before starting pump
- **Maximum Pump Time**: Pump automatically stops after 60 seconds (safety timeout)
- **LED Status Indicator**: Fast blink when pump is active, steady on during standby

## 🚀 Installation & Setup

### 1. Install PlatformIO
```bash
# Via VS Code Extension
# Install "PlatformIO IDE" extension

# Or via command line
pip install platformio
```

### 2. Clone & Build Project
```bash
git clone <repository-url>
cd AgroHygra
pio run  # Compile project
```

### 3. Upload to ESP32
```bash
# Via USB
pio run --target upload

# Via Serial Monitor
pio device monitor
```

### 4. Access Web Interface
After ESP32 connects to WiFi, open browser:
- **IP Address**: `http://192.168.x.x` (see Serial Monitor)
- **mDNS**: `http://agrohygra.local`

### 5. First Boot
On first boot without saved WiFi credentials:
1. ESP32 starts as Access Point: **AgroHygra-Setup**
2. Connect to this network (open, no password required)
3. Open browser to `http://192.168.4.1`
4. Select your home WiFi network and enter password
5. Device will save credentials and restart
6. Access the web interface via IP or mDNS after restart

## 🌐 API Endpoints

### Web Interface
- `GET /` - Main page (dashboard)
- `GET /wifi/setup` - WiFi configuration page
- `GET /wifi/scan` - Scan available WiFi networks
- `GET /pump/on` - Manually turn on pump
- `GET /pump/off` - Manually turn off pump

### REST API
- `GET /api/data` - Sensor data in JSON format

**Example Response:**
```json
{
  "soilMoisture": 45,
  "temperature": 28.5,
  "humidity": 65.2,
  "airQuality": 35,
  "airQualityRaw": 1200,
  "airQualityGood": true,
  "airQualityPPM": 850,
  "tdsRaw": 1500,
  "tdsValuePPM": 750,
  "pumpActive": false,
  "wateringCount": 12,
  "totalWateringTime": 3600,
  "moistureThreshold": 30,
  "moistureStop": 70,
  "uptime": 86400,
  "mqttConnected": true,
  "wifiConnected": true,
  "ipAddress": "192.168.1.100",
  "mqttTopics": {
    "sensors": "agrohygra/sensors",
    "pumpCommand": "agrohygra/pump/command",
    "pumpStatus": "agrohygra/pump/status",
    "systemStatus": "agrohygra/system/status"
  }
}
```

### MQTT Control Commands

**Turn On Pump via MQTT:**
```
Topic: agrohygra/pump/command
Payload: ON
```

**Turn Off Pump via MQTT:**
```
Topic: agrohygra/pump/command
Payload: OFF
```

**JSON Command Format (Alternative):**
```json
{
  "pump": true  // or false
}
```

### Example MQTT Sensor Data Publication:
```json
{
  "device": "AgroHygra-ESP32",
  "time": 3600,
  "soil": 45,
  "temp": 28.5,
  "hum": 65.2,
  "air": 35,
  "airRaw": 1200,
  "airGood": true,
  "ppm": 850,
  "pump": false,
  "count": 12,
  "wtime": 3600,
  "uptime": 3600,
  "tdsRaw": 1500,
  "tds": 750
}
```

## 📊 Monitoring & Troubleshooting

### Serial Monitor Output
```
🌱 AgroHygra - Smart Irrigation System
=====================================
🔗 Connecting to WiFi: MyWiFi
✅ WiFi connected!
📡 IP Address: 192.168.1.100
🌐 mDNS started: http://agrohygra.local
🌐 Web server started on port 80
✅ System ready!
🌾 Current soil moisture: 45%
🌡️  Temperature: 28.5°C, Air humidity: 65.2%
🌬️  Air quality: 35% (ADC: 1200, Status: Good, ~850 ppm)
=====================================
📊 Soil: 45% | Temp: 28.5°C | RH: 65.2% | Air: 35% | Pump: OFF
🚰 PUMP ACTIVATED - Soil moisture low
📊 Soil: 32% | Temp: 28.8°C | RH: 64.8% | Air: 38% | Pump: ON
🛑 PUMP DEACTIVATED - Duration: 45 seconds
📤 Published sensor data to agrohygra/sensors: {...} (success: YES)
✅ Connected to MQTT broker
```

### Web Dashboard Features
- **Real-time sensor readings** - Updated every 2 seconds
- **Live pump status** - Shows if pump is currently running
- **System statistics** - Total watering count, total pump time, system uptime
- **WiFi & MQTT status** - Connection status display
- **Manual pump control** - Turn on/off pump manually from web interface
- **Responsive design** - Works on desktop, tablet, and mobile

### LCD Display Modes (Auto-cycling every 2 seconds)
The 16x2 LCD cycles through 5 pages:
- **Page 0:** Soil moisture % & Temperature
- **Page 1:** Humidity % & Air Quality status
- **Page 2:** TDS value (ppm) & Pump status with runtime
- **Page 3:** WiFi status & MQTT connection + watering count
- **Page 4:** SSID or AP name & IP address

### LED Status Indicator
- **Steady On** - System normal, pump OFF, running in stable state
- **Fast Blink (250ms)** - Pump is actively running (watering in progress)
- **Off** - System booting or error state

### Common Issues & Solutions

**1. WiFi Not Connected**
- Check SSID and password are correct
- Ensure WiFi signal is strong enough (at least -70 dBm)
- Try restarting ESP32 (press RESET button)
- Check if WiFi supports 2.4GHz (ESP32 doesn't support 5GHz)

**2. Sensor Not Accurate**
- **Soil Moisture:** Recalibrate `SOIL_DRY_VALUE` and `SOIL_WET_VALUE`
  - Check sensor cable connections for loose wires
  - Clean sensor probe from soil deposits and corrosion
  - Ensure sensor is at correct depth in soil
- **DHT22 Temperature/Humidity:** Check for proper wiring
  - Verify 3.3V power supply to sensor
  - Check data line has proper pull-up resistor
- **MQ-135 Air Quality:** Requires warm-up time
  - Allow 24 hours for sensor stabilization after power-on
  - Calibrate in fresh air for best results
- **TDS Meter:** Needs proper calibration
  - Check voltage output from sensor
  - Adjust `TDS_K` calibration constant based on reference fluid

**3. Pump Not Activating**
- Check relay wiring (relay IN pin, VCC, GND)
- Verify pump power supply voltage and polarity
- Test relay manually with multimeter (should hear click when activated)
- Ensure pump is not mechanically stuck
- Check for water leaks or blockages in pump/tubing

**4. Web Interface Not Accessible**
- Check IP address shown in Serial Monitor
- Ensure ESP32 and your device are on same WiFi network
- Try accessing via IP directly: `http://192.168.x.x`
- If mDNS not working, try IP instead of `agrohygra.local`
- Disable any VPN or proxy that might block local network access

**5. MQTT Not Connecting**
- Check MQTT broker address and port
- Verify WiFi is connected before MQTT connection attempt
- Check Serial Monitor for MQTT state codes:
  - `-4` = timeout
  - `-3` = connection lost
  - `-2` = connect failed
  - `-1` = disconnected
  - `0` = connected
- Try alternative free broker (test.mosquitto.org)

**6. Pump Running Continuously or Not Stopping**
- Check moisture thresholds (`MOISTURE_THRESHOLD` and `MOISTURE_STOP`)
- Verify soil sensor is reading correctly
- Safety timeout should trigger after 60 seconds (check `MAX_PUMP_TIME`)
- Manual override: Use web interface to manually turn off pump

**7. LCD Display Not Showing**
- Check I2C wiring (SDA=GPIO21, SCL=GPIO22)
- Verify LCD power supply (5V)
- Check I2C address (default 0x27 or 0x3F)
- Serial Monitor will show "I2C LCD not found" if address is wrong
- Try I2C scanner to find actual LCD address

## 🔨 Customization

### Adding New Sensors
Add to `readAllSensors()` function in `main.cpp`:
```cpp
void readAllSensors() {
  soilMoisture = readSoilMoisture();
  temperature = dht.readTemperature();
  humidity = dht.readHumidity();
  
  // New sensor - Light level
  int lightLevel = analogRead(LIGHT_SENSOR_PIN);
  
  // New sensor - pH level
  float phLevel = readPHSensor();
  
  // Publish via MQTT or add to JSON response
  if (mqttClient.connected()) {
    mqttClient.publish("agrohygra/light", String(lightLevel).c_str());
    mqttClient.publish("agrohygra/ph", String(phLevel).c_str());
  }
}
```

### Changing MQTT Broker
Edit in `main.cpp`:
```cpp
const char *MQTT_HOST = "your-broker.com";  // Your broker address
const int MQTT_PORT = 1883;                 // Or 8883 for TLS
const char *MQTT_USERNAME = "username";     // If required
const char *MQTT_PASSWORD = "password";     // If required
```

### Adjusting Sensor Intervals
```cpp
const int SENSOR_READ_INTERVAL = 2;         // Read sensors every 2 seconds
const unsigned long MQTT_SENSOR_INTERVAL = 2000;     // Publish every 2 seconds
const unsigned long LCD_UPDATE_INTERVAL = 2000;      // Update LCD every 2 seconds
```

### Custom Irrigation Logic
Modify `autoIrrigation()` function to implement advanced logic:
```cpp
void autoIrrigation() {
  // Example: Different thresholds based on time of day
  int threshold = (hour() < 12) ? 25 : 35;  // Lower in morning, higher in afternoon
  
  if (!pumpActive && soilMoisture <= threshold) {
    startPump();
  }
  
  if (pumpActive && soilMoisture >= (threshold + 30)) {
    stopPump();
  }
}
```

### Home Assistant Integration
Use API endpoint for Home Assistant integration:
```yaml
# configuration.yaml
sensor:
  - platform: rest
    resource: http://agrohygra.local/api/data
    name: "AgroHygra Soil Moisture"
    value_template: "{{ value_json.soilMoisture }}"
    unit_of_measurement: "%"
    scan_interval: 30

  - platform: rest
    resource: http://agrohygra.local/api/data
    name: "AgroHygra Temperature"
    value_template: "{{ value_json.temperature }}"
    unit_of_measurement: "°C"
    scan_interval: 30

switch:
  - platform: rest
    name: "AgroHygra Pump"
    resource: http://agrohygra.local/pump/on
    body_on: 'ON'
    is_on_template: "{{ value_json.pumpActive }}"
```

### MQTT to Home Assistant (via MQTT)
```yaml
# configuration.yaml
mqtt:
  broker: broker.hivemq.com

sensor:
  - platform: mqtt
    name: "AgroHygra Soil Moisture"
    state_topic: "agrohygra/sensors"
    value_template: "{{ value_json.soil }}"
    unit_of_measurement: "%"

  - platform: mqtt
    name: "AgroHygra Temperature"
    state_topic: "agrohygra/sensors"
    value_template: "{{ value_json.temp }}"
    unit_of_measurement: "°C"
```

### Push Notifications via Telegram
Add to `main.cpp`:
```cpp
void sendTelegramNotification(String message) {
  HTTPClient http;
  String url = "https://api.telegram.org/bot<YOUR_BOT_TOKEN>/sendMessage";
  String payload = "chat_id=<YOUR_CHAT_ID>&text=" + message;
  
  http.begin(url);
  http.addHeader("Content-Type", "application/x-www-form-urlencoded");
  int httpCode = http.POST(payload);
  
  if (httpCode == 200) {
    Serial.println("Telegram notification sent");
  }
  http.end();
}

// Call when pump starts/stops
void startPump() {
  // ... existing code ...
  sendTelegramNotification("🚰 Pump started - Soil moisture: " + String(soilMoisture) + "%");
}
```

### Data Logging to InfluxDB
```cpp
#include <InfluxDbClient.h>

// Configure InfluxDB
InfluxDBClient client("http://192.168.1.x:8086", "agrohygra");

void logToInfluxDB() {
  Point sensorData("agrohygra");
  sensorData.addTag("device", "ESP32");
  sensorData.addField("soil_moisture", soilMoisture);
  sensorData.addField("temperature", temperature);
  sensorData.addField("humidity", humidity);
  sensorData.addField("air_quality", airQuality);
  sensorData.addField("tds", tdsValue);
  sensorData.addField("pump_active", pumpActive ? 1 : 0);
  
  if (!client.writePoint(sensorData)) {
    Serial.print("InfluxDB write failed: ");
    Serial.println(client.getLastErrorMessage());
  }
}
```

## 📈 Future Development Roadmap

### Phase 2 - Advanced Features
- [ ] **Multiple Irrigation Zones** - Control several plant areas independently
- [ ] **Time-based Scheduling** - Schedule irrigation at specific times
- [ ] **Moisture History Graph** - Display sensor trends over time
- [ ] **Plant Profiles** - Save different plant type configurations
- [ ] **Weather Integration** - Fetch local weather data to adjust irrigation
- [ ] **Water Level Sensor** - Prevent pump dry-running with reservoir monitoring

### Phase 3 - Data & Intelligence
- [ ] **Database Logging** - Save all sensor data to InfluxDB/TimescaleDB/MySQL
- [ ] **Grafana Dashboards** - Professional data visualization
- [ ] **Machine Learning** - Predict water needs with AI/neural networks
- [ ] **Data Analytics** - Pattern recognition for optimal watering schedules
- [ ] **Predictive Maintenance** - Alert for sensor/pump issues

### Phase 4 - Expansion & Integration
- [ ] **Mobile App** - Native Android/iOS companion application
- [ ] **Cloud Storage** - Sync data to cloud platforms (AWS, Google Cloud, Azure)
- [ ] **Voice Control** - Alexa/Google Assistant integration
- [ ] **Multiple Devices** - Support multi-device management and sync
- [ ] **Home Automation** - Full integration with Home Assistant, OpenHAB

### Phase 5 - Hardware Upgrades
- [ ] **Solar Power** - Solar panel + battery for outdoor deployment
- [ ] **LoRaWAN/NB-IoT** - Long-range communication for remote farms
- [ ] **Wireless Sensor Nodes** - Multiple independent sensor units
- [ ] **Advanced Sensors** - EC/NPK sensors, leaf wetness, soil temperature
- [ ] **Dual Pump Control** - Separate fertilizer/nutrient injection system

### Phase 6 - Commercial Ready
- [ ] **Industrial Certification** - IP67 rated enclosure, professional design
- [ ] **Scalability** - Support for greenhouse management systems
- [ ] **Security Hardening** - TLS/SSL, user authentication, encryption
- [ ] **API Documentation** - Complete OpenAPI/Swagger specification
- [ ] **Mobile Responsiveness** - Progressive Web App (PWA) support

## 📄 License

This project is licensed under the MIT License. See `LICENSE` file for full details.

## 🤝 Contributing

Contributions are welcome! Please:
1. Fork the repository
2. Create a feature branch (`git checkout -b feature/AmazingFeature`)
3. Commit your changes (`git commit -m 'Add some AmazingFeature'`)
4. Push to the branch (`git push origin feature/AmazingFeature`)
5. Create a Pull Request

Please ensure your code:
- Follows consistent formatting and naming conventions
- Includes comments for complex logic
- Is tested on ESP32 hardware
- Updates documentation as needed

## 📞 Support & Community

If you have any questions, issues, or suggestions:
- **GitHub Issues**: Create an issue on the repository
- **Documentation**: Check project wiki and code comments
- **Discussions**: Use GitHub Discussions for feature requests
- **Community**: Join AgroHygra Telegram group for community support

## 🛠️ Project Structure

```
AgroHygra/
├── src/
│   └── main.cpp              # Main firmware code
├── include/                  # Header files (if any)
├── lib/                      # External libraries
├── test/                     # Unit tests
├── platformio.ini            # PlatformIO configuration
├── README.md                 # This file
├── SETUP.md                  # Detailed setup instructions
├── WIRING.md                 # Detailed wiring diagram
└── LICENSE                   # MIT License
```

## 📚 Related Documentation

- **SETUP.md** - Step-by-step hardware and software setup guide
- **WIRING.md** - Detailed wiring diagrams and pin configurations
- **API.md** - Comprehensive API documentation (if available)

## 🌍 Version & Changelog

**Current Version:** 1.0.0

**Key Features in v1.0:**
- ✅ Soil moisture monitoring with calibration
- ✅ Temperature & humidity sensing (DHT22)
- ✅ Air quality monitoring (MQ-135)
- ✅ TDS/EC water quality sensor support
- ✅ Automatic irrigation control with safety features
- ✅ Web dashboard with real-time updates
- ✅ REST API for integration
- ✅ MQTT support for remote monitoring
- ✅ 16x2 LCD display with multi-page cycling
- ✅ WiFi setup via Access Point mode
- ✅ Persistent credential storage
- ✅ LED status indicator
- ✅ Safety timeouts and boot delays

For detailed changelog, see GitHub releases.

## 🎯 Project Goals

AgroHygra aims to:
1. **Simplify** plant watering automation with easy setup
2. **Reduce** water waste through intelligent irrigation
3. **Monitor** environmental conditions in real-time
4. **Enable** remote monitoring via web and MQTT
5. **Support** integration with home automation systems
6. **Provide** an open-source platform for agricultural IoT

## ⚡ Quick Start Checklist

- [ ] Gather all hardware components
- [ ] Wire components according to wiring diagram
- [ ] Install PlatformIO extension in VS Code
- [ ] Clone the repository
- [ ] Connect ESP32 via USB
- [ ] Run `pio run --target upload`
- [ ] Monitor serial output for IP address
- [ ] Connect to WiFi via web interface
- [ ] Access dashboard at IP address or agrohygra.local
- [ ] Calibrate soil moisture sensor
- [ ] Set irrigation thresholds
- [ ] Test pump control manually
- [ ] Monitor automatic irrigation in action

---

**Made with ❤️ for Smart Agriculture**

*Last Updated: October 2025*
*Contributors: Kiyomi Fujiwara & Moehammad Joesoef Aldri*
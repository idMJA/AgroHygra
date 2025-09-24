# 📐 Wiring Diagram - AgroHygra Smart Irrigation System

## 🔌 Koneksi Pin ESP32

```
╔══════════════════════════════════════════════════════════════╗
║                        ESP32 DevKit                         ║
║                                                              ║
║  GPIO 14 ──────────── DHT22 Data Pin                        ║
║  GPIO 34 ──────────── Soil Sensor Analog Output             ║
║  GPIO 27 ──────────── Relay Module IN                       ║
║  GPIO 2  ──────────── LED Status (Built-in)                 ║
║                                                              ║
║  3.3V ─────────────── DHT22 VCC                             ║
║  3.3V ─────────────── Soil Sensor VCC                       ║
║  5V   ─────────────── Relay Module VCC                      ║
║                                                              ║
║  GND  ─────────────── DHT22 GND                             ║
║  GND  ─────────────── Soil Sensor GND                       ║  
║  GND  ─────────────── Relay Module GND                      ║
╚══════════════════════════════════════════════════════════════╝
```

## 🔧 Detail Koneksi Per Komponen

### 1. Sensor DHT22 (Suhu & Kelembapan Udara)
```
DHT22 Pin    →    ESP32 Pin
=============================
VCC (+)      →    3.3V
GND (-)      →    GND
DATA         →    GPIO 14
```

### 2. Soil Moisture Sensor (Kelembapan Tanah)
```
Soil Sensor  →    ESP32 Pin
=============================
VCC (+)      →    3.3V
GND (-)      →    GND
AO (Analog)  →    GPIO 34 (ADC1_CH6)
DO (Digital) →    Not used
```

### 3. Relay Module (Kontrol Pompa)
```
Relay Pin    →    ESP32 Pin    →    Load (Pompa)
================================================
VCC          →    5V           
GND          →    GND          
IN           →    GPIO 27      
COM          →                 →    Power Supply (+)
NO           →                 →    Pompa (+)
NC           →                 →    Not used
                              →    Pompa (-) to GND
```

## ⚡ Power Supply Diagram

```
┌─────────────────┐    ┌───────────────┐    ┌─────────────────┐
│  Power Adapter  │    │     ESP32     │    │   Relay Module  │
│     5V/2A       │    │   DevKit      │    │      5V         │
│                 │    │               │    │                 │
│  (+) ───────────┼────┤ VIN      5V   ├────┤ VCC             │
│  (-) ───────────┼────┤ GND      GND  ├────┤ GND             │
└─────────────────┘    └───────────────┘    └─────────────────┘
                              │
                              │ 3.3V
                       ┌──────┴──────┐
                       │   Sensors   │
                       │ DHT22 + Soil│
                       │   3.3V      │
                       └─────────────┘

┌─────────────────┐    ┌─────────────────┐
│  Pump Power     │    │   Water Pump    │
│  Supply 12V     │    │    DC 12V       │
│                 │    │                 │
│  (+) ───────────┼────┤ (+) via Relay   │
│  (-) ───────────┼────┤ (-)             │
└─────────────────┘    └─────────────────┘
```

## 🛠️ Assembly Steps

### Step 1: Prepare Components
- [ ] ESP32 Development Board
- [ ] DHT22 Temperature/Humidity Sensor
- [ ] Soil Moisture Sensor (capacitive recommended)
- [ ] 5V Relay Module (1 channel)
- [ ] Water Pump DC 12V (or 5V for smaller setup)
- [ ] Jumper wires (Male-Female, Male-Male)
- [ ] Breadboard (optional for prototyping)
- [ ] Power supplies (5V for ESP32, 12V for pump)

### Step 2: Sensor Connections
1. **Connect DHT22:**
   - Red wire (VCC) → ESP32 3.3V
   - Black wire (GND) → ESP32 GND  
   - Yellow/Blue wire (DATA) → ESP32 GPIO 14

2. **Connect Soil Sensor:**
   - VCC → ESP32 3.3V
   - GND → ESP32 GND
   - AO → ESP32 GPIO 34 (Analog pin)

### Step 3: Relay Module Connection
1. **Control signals:**
   - Relay VCC → ESP32 5V
   - Relay GND → ESP32 GND
   - Relay IN → ESP32 GPIO 27

2. **Load connections:**
   - Connect pump (+) wire to relay NO (Normally Open)
   - Connect power supply (+) to relay COM (Common)
   - Connect pump (-) directly to power supply (-)

### Step 4: Power Distribution
```
Primary Power (5V/2A Adapter):
├── ESP32 VIN (5V input)
└── Relay Module VCC

Secondary Power (12V Adapter):
├── Water Pump via Relay
└── (Optional) Extra sensors

ESP32 Generated:
├── 3.3V → Sensors (DHT22, Soil)
└── GND → All components
```

## ⚠️ Safety Notes

### Electrical Safety
- **Double-check polarity** before powering on
- **Use proper wire gauge** for pump current
- **Add fuses** on power supply lines
- **Insulate all connections** properly
- **Keep water away** from electronics

### Component Protection
- **Use pull-up resistors** on DHT22 data line (10kΩ)
- **Add flyback diodes** for inductive loads
- **Use optocoupler relay** for better isolation
- **Add capacitors** for power supply filtering

### Water System Safety
- **Use food-grade tubing** for drinking water
- **Install overflow protection** in water tank
- **Add water level sensors** to prevent dry running
- **Use submersible pumps** rated for continuous operation

## 🔍 Testing Procedure

### 1. Component Testing (Before Full Assembly)
```
Test DHT22:
- Connect to ESP32
- Upload test sketch
- Verify temperature/humidity readings

Test Soil Sensor:
- Check dry/wet values in Serial Monitor  
- Calibrate SOIL_DRY_VALUE and SOIL_WET_VALUE

Test Relay:
- Manually trigger from code
- Listen for relay click sound
- Measure continuity with multimeter
```

### 2. System Integration Test
```
1. Upload full AgroHygra code
2. Check Serial Monitor for startup messages
3. Verify WiFi connection (IP address displayed)
4. Test web interface access
5. Manually trigger pump via web buttons
6. Verify automatic watering thresholds
```

### 3. Calibration Process
```
Soil Sensor Calibration:
1. Place sensor in completely dry soil/air
2. Record ADC value → Set as SOIL_DRY_VALUE
3. Place sensor in wet soil/water
4. Record ADC value → Set as SOIL_WET_VALUE
5. Test percentage calculation accuracy
```

## 📏 PCB Layout (Optional Advanced)

For permanent installation, consider custom PCB with:
- ESP32-WROOM-32 module
- Onboard voltage regulators (3.3V, 5V)
- Screw terminals for external connections
- Status LEDs for each function
- Protection circuits (fuses, TVS diodes)
- Mounting holes for enclosure

```
PCB Size: 100mm x 80mm (approximate)

Components Layout:
┌─────────────────────────────────────┐
│ [PWR LED]  [WiFi LED]  [PUMP LED]   │
│                                     │
│     ┌─────────────┐                 │
│     │   ESP32     │  [Relay]        │
│     │   Module    │                 │  
│     └─────────────┘                 │
│                                     │
│ [DHT Conn] [Soil Conn] [Pump Conn]  │
│                                     │
│ [5V IN]    [12V IN]    [GND]        │
└─────────────────────────────────────┘
```

## 🏠 Enclosure Recommendations

- **IP54 rated plastic box** for outdoor use
- **Ventilation holes** for DHT22 sensor
- **Cable glands** for wire entry
- **Clear window** for status LEDs
- **Mounting brackets** for wall/pole installation

**Suggested Dimensions:** 200mm x 150mm x 80mm

---

*⚠️ Important: Always double-check connections before powering on. If unsure about electrical connections, consult with an experienced electronics technician.*
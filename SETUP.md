# 🚀 Setup Instructions - AgroHygra

## ⚠️ Prerequisites

Sebelum menjalankan project ini, Anda perlu menginstall tools berikut:

### Option 1: PlatformIO (Recommended)

1. **Install Python 3.7+**
   - Download dari: https://python.org/downloads/
   - Centang "Add Python to PATH" saat instalasi
   - Verify: `python --version`

2. **Install PlatformIO**
   ```bash
   pip install platformio
   ```

3. **Install VS Code Extension** (Optional tapi direkomendasikan)
   - Buka VS Code
   - Install extension: "PlatformIO IDE"

### Option 2: Arduino IDE

1. **Install Arduino IDE 2.x**
   - Download: https://arduino.cc/en/software

2. **Install ESP32 Board Package**
   - File → Preferences → Additional Boards Manager URLs
   - Tambahkan: `https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json`
   - Tools → Board → Boards Manager → Cari "ESP32" → Install "esp32 by Espressif"

3. **Install Libraries**
   - Sketch → Include Library → Manage Libraries
   - Install libraries berikut:
     - "DHT sensor library" by Adafruit
     - "ArduinoJson" by Benoit Blanchon

4. **Convert Project Structure**
   - Rename `src/main.cpp` → `AgroHygra.ino`
   - Move ke folder `AgroHygra/AgroHygra.ino`

## 🔧 Build & Upload

### Via PlatformIO
```bash
cd AgroHygra
pio run                    # Compile
pio run --target upload    # Upload ke ESP32
pio device monitor         # Serial monitor
```

### Via Arduino IDE
1. Open `AgroHygra.ino`
2. Select Board: Tools → Board → ESP32 Arduino → ESP32 Dev Module
3. Select Port: Tools → Port → (pilih port COM ESP32)
4. Click Upload (→)

## 📝 Configuration

1. **Edit WiFi Settings** in code:
   ```cpp
   const char* ssid = "YOUR_WIFI_NAME";
   const char* password = "YOUR_WIFI_PASSWORD";
   ```

2. **Calibrate Soil Sensor**:
   - Run program
   - Check Serial Monitor untuk nilai ADC
   - Update `SOIL_DRY_VALUE` dan `SOIL_WET_VALUE`

3. **Adjust Thresholds**:
   ```cpp
   const int MOISTURE_THRESHOLD = 30;  // Start watering at ≤30%
   const int MOISTURE_STOP = 70;       // Stop watering at ≥70%
   ```

## 🔍 Troubleshooting

### "ESP32 not found" Error
- Install ESP32 drivers: https://www.silabs.com/developers/usb-to-uart-bridge-vcp-drivers
- Check cable (harus data cable, bukan charging only)
- Press BOOT button pada ESP32 saat upload

### Library Errors
- Make sure all libraries installed with correct versions
- Clear build cache: `pio run --target clean` (PlatformIO)

### WiFi Connection Issues
- Check SSID/password spelling
- Ensure 2.4GHz WiFi (ESP32 tidak support 5GHz)
- Move closer to router for better signal

## 📋 Quick Start Checklist

- [ ] Python 3.7+ installed
- [ ] PlatformIO installed (`pip install platformio`)
- [ ] ESP32 drivers installed  
- [ ] ESP32 board connected via USB
- [ ] WiFi credentials configured in code
- [ ] Hardware wired according to diagram
- [ ] Libraries installed (DHT, ArduinoJson)
- [ ] Project compiled successfully
- [ ] Code uploaded to ESP32
- [ ] Serial monitor shows successful WiFi connection
- [ ] Web interface accessible via browser

## 🌐 Access Points

After successful upload and WiFi connection:

- **Serial Monitor**: Check IP address printed
- **Web Interface**: http://[IP_ADDRESS] (e.g., http://192.168.1.100)
- **mDNS**: http://agrohygra.local (if supported)
- **API**: http://[IP_ADDRESS]/api/data

## 📞 Need Help?

1. Check Serial Monitor output for error messages
2. Verify all wiring connections
3. Test individual components separately
4. Check this project's Issues on GitHub
5. Join our Telegram group for community support

---

**Ready to grow? Let's make your plants happy! 🌱**
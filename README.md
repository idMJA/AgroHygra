# 🌱 AgroHygra - Smart Irrigation System

Sistem irigasi pintar berbasis ESP32 untuk monitoring dan kontrol otomatis penyiraman tanaman berdasarkan kelembapan tanah.

## 📋 Fitur Utama

- **Monitor Kelembapan Tanah** - Sensor resistif/kapasitif untuk mengukur kadar air tanah
- **Sensor Lingkungan** - Suhu dan kelembapan udara menggunakan DHT22
- **Irigasi Otomatis** - Pompa air otomatis berdasarkan threshold kelembapan
- **Web Interface** - Monitoring dan kontrol via browser (responsive design)
- **API REST** - Data JSON untuk integrasi dengan aplikasi lain
- **Safety Features** - Timeout pompa, LED status, statistik penyiraman
- **WiFi Connectivity** - Akses remote via jaringan WiFi
- **mDNS Support** - Akses mudah via http://agrohygra.local

## 🔧 Komponen Hardware

### Mikrokontroler
- **ESP32 Development Board** (ESP32-DevKitC atau compatible)

### Sensor
- **DHT22** - Sensor suhu dan kelembapan udara
- **Soil Moisture Sensor** - Sensor kelembapan tanah (resistif atau kapasitif)

### Aktuator & Output
- **Relay Module 5V** - Untuk kontrol pompa air
- **Water Pump** - Pompa air DC 3-12V
- **LED Built-in ESP32** - Indikator status sistem

### Catu Daya
- **Power Supply 5V/2A** - Untuk ESP32 dan relay
- **Adaptor 12V** (opsional) - Untuk pompa air yang lebih besar

## 📐 Wiring Diagram

```
ESP32 Pin    →    Komponen
=========================================
GPIO 14      →    DHT22 Data Pin
GPIO 34      →    Soil Sensor Analog Out
GPIO 27      →    Relay IN (Pompa)
GPIO 2       →    LED Status (built-in)
3.3V         →    DHT22 VCC, Soil Sensor VCC
GND          →    DHT22 GND, Soil Sensor GND
5V           →    Relay VCC

Relay        →    Pompa Air
=========================================
Relay COM    →    Power Supply (+) Pompa
Relay NO     →    Pompa (+)
GND          →    Pompa (-)
```

## ⚙️ Konfigurasi

### 1. WiFi Settings
Edit di file `src/main.cpp`:
```cpp
const char* ssid = "YOUR_WIFI_SSID";        // Nama WiFi Anda
const char* password = "YOUR_WIFI_PASSWORD"; // Password WiFi Anda
```

### 2. Kalibrasi Sensor Kelembapan Tanah
```cpp
const int SOIL_DRY_VALUE = 3000;    // Nilai ADC saat tanah kering
const int SOIL_WET_VALUE = 1000;    // Nilai ADC saat tanah basah
```

**Cara Kalibrasi:**
1. Biarkan sensor di udara terbuka (kering) → catat nilai ADC
2. Celupkan sensor ke air → catat nilai ADC
3. Update konstanta `SOIL_DRY_VALUE` dan `SOIL_WET_VALUE`

### 3. Threshold Penyiraman
```cpp
const int MOISTURE_THRESHOLD = 30;   // Mulai penyiraman pada ≤30%
const int MOISTURE_STOP = 70;        // Berhenti penyiraman pada ≥70%
const int MAX_PUMP_TIME = 60;        // Maksimal 60 detik (safety)
```

## 🚀 Instalasi & Setup

### 1. Install PlatformIO
```bash
# Via VS Code Extension
# Install "PlatformIO IDE" extension

# Atau via command line
pip install platformio
```

### 2. Clone & Build Project
```bash
git clone <repository-url>
cd AgroHygra
pio run  # Compile project
```

### 3. Upload ke ESP32
```bash
# Via USB
pio run --target upload

# Via Serial Monitor
pio device monitor
```

### 4. Akses Web Interface
Setelah ESP32 terhubung WiFi, buka browser:
- **IP Address**: `http://192.168.x.x` (lihat Serial Monitor)
- **mDNS**: `http://agrohygra.local`

## 🌐 API Endpoints

### Web Interface
- `GET /` - Halaman utama (dashboard)
- `GET /pump/on` - Nyalakan pompa manual
- `GET /pump/off` - Matikan pompa manual

### REST API
- `GET /api/data` - Data sensor dalam format JSON

**Contoh Response:**
```json
{
  "soilMoisture": 45,
  "temperature": 28.5,
  "humidity": 65.2,
  "pumpActive": false,
  "wateringCount": 12,
  "totalWateringTime": 3600,
  "moistureThreshold": 30,
  "moistureStop": 70,
  "uptime": 86400
}
```

## 📊 Monitoring & Troubleshooting

### Serial Monitor Output
```
🌱 AgroHygra - Smart Irrigation System
=====================================
🔗 Menghubungkan ke WiFi: MyWiFi
✅ WiFi terhubung!
📡 IP Address: 192.168.1.100
🌐 mDNS started: http://agrohygra.local
🌐 Web server dimulai
✅ Sistem siap!
🌾 Kelembapan tanah saat ini: 45%
🌡️  Suhu: 28.5°C, Kelembapan udara: 65.2%
=====================================
📊 Tanah: 45% | Suhu: 28.5°C | RH: 65.2% | Pompa: OFF
🚰 POMPA DINYALAKAN - Kelembapan tanah rendah
📊 Tanah: 32% | Suhu: 28.8°C | RH: 64.8% | Pompa: ON
🛑 POMPA DIMATIKAN - Durasi: 45 detik
```

### LED Status Indikator
- **Nyala Terus** - Sistem normal, pompa OFF
- **Kedip Cepat** - Pompa aktif
- **Mati** - Sistem error atau booting

### Common Issues

**1. WiFi Tidak Terhubung**
- Periksa SSID dan password
- Pastikan sinyal WiFi cukup kuat
- Coba restart ESP32

**2. Sensor Tidak Akurat**
- Kalibrasi ulang `SOIL_DRY_VALUE` dan `SOIL_WET_VALUE`
- Periksa koneksi kabel sensor
- Bersihkan probe sensor dari korosi

**3. Pompa Tidak Nyala**
- Periksa wiring relay dan pompa
- Test relay dengan multimeter
- Pastikan catu daya pompa mencukupi

**4. Web Interface Tidak Bisa Diakses**
- Periksa IP address di Serial Monitor
- Pastikan ESP32 dan perangkat di jaringan yang sama
- Coba akses via IP langsung, bukan mDNS

## 🔨 Customization

### Menambah Sensor Baru
Tambahkan di fungsi `readAllSensors()`:
```cpp
void readAllSensors() {
  soilMoisture = readSoilMoisture();
  temperature = dht.readTemperature();
  humidity = dht.readHumidity();
  
  // Sensor baru
  lightLevel = analogRead(LIGHT_SENSOR_PIN);
  phLevel = readPHSensor();
}
```

### Integrasi dengan Home Assistant
Gunakan API endpoint untuk integrasi:
```yaml
# configuration.yaml
sensor:
  - platform: rest
    resource: http://agrohygra.local/api/data
    name: "AgroHygra Data"
    value_template: "{{ value_json.soilMoisture }}"
    unit_of_measurement: "%"
```

### Notifikasi Push
Tambahkan integrasi dengan Telegram/Discord/Email:
```cpp
void sendNotification(String message) {
  // Implementasi notifikasi
  HTTPClient http;
  http.begin("https://api.telegram.org/bot<TOKEN>/sendMessage");
  // ... kode notifikasi
}
```

## 📈 Pengembangan Lanjutan

- [ ] **Database Logging** - Simpan data ke InfluxDB/MySQL
- [ ] **Machine Learning** - Prediksi kebutuhan air dengan AI
- [ ] **Mobile App** - Aplikasi Android/iOS native
- [ ] **Multiple Zones** - Kontrol beberapa area tanaman
- [ ] **Weather API** - Integrasi data cuaca untuk optimisasi
- [ ] **Solar Power** - Sistem tenaga surya untuk outdoor
- [ ] **LoRaWAN** - Komunikasi jarak jauh untuk area remote

## 📄 Lisensi

Project ini menggunakan lisensi MIT. Lihat file `LICENSE` untuk detail lengkap.

## 🤝 Kontribusi

Kontribusi sangat diterima! Silakan:
1. Fork repository
2. Buat branch feature (`git checkout -b feature/AmazingFeature`)
3. Commit changes (`git commit -m 'Add some AmazingFeature'`)
4. Push ke branch (`git push origin feature/AmazingFeature`)
5. Buat Pull Request

## 📞 Support

Jika ada pertanyaan atau masalah:
- **Issues**: Buat issue di GitHub repository
- **Dokumentasi**: Lihat wiki project
- **Community**: Join Telegram group AgroHygra

---

**Made with ❤️ for Smart Agriculture**
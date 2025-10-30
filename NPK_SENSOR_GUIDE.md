# 🌾 RS485 NPK Sensor 7-in-1 Integration Guide

## 📋 Overview

Sensor RS485 NPK 7-in-1 adalah sensor tanah profesional yang mengukur 7 parameter penting:
- **Nitrogen (N)** - Kandungan nitrogen dalam tanah (mg/kg)
- **Phosphorus (P)** - Kandungan fosfor dalam tanah (mg/kg)
- **Potassium (K)** - Kandungan kalium dalam tanah (mg/kg)
- **pH** - Tingkat keasaman tanah (0-14)
- **EC (Electrical Conductivity)** - Konduktivitas listrik tanah (mS/cm)
- **Temperature** - Suhu tanah (°C)
- **Humidity/Moisture** - Kelembaban tanah (%)

## 🔌 Wiring / Koneksi Hardware

### Pinout Sensor

**Sisi Kiri (Power Supply):**
```
VCC  →  5V (ESP32)
B    →  RS485 B line
A    →  RS485 A line
GND  →  GND (ESP32)
```

**Sisi Kanan (UART Communication):**
```
DI (Driver Input)      →  TX2 (GPIO 17) ESP32
DE (Driver Enable)     →  GPIO 23 ESP32
RE (Receiver Enable)   →  GPIO 23 ESP32  (digabung dengan DE)
RO (Receiver Output)   →  RX2 (GPIO 16) ESP32
```

### Diagram Koneksi Lengkap

```
ESP32                           NPK Sensor 7-in-1
====================================================
5V        ──────────────────►  VCC (Power)
GND       ──────────────────►  GND (Ground)
                               B (RS485 B line)
                               A (RS485 A line)
GPIO 17   ──────────────────►  DI (Driver Input)
GPIO 16   ◄──────────────────  RO (Receiver Output)
GPIO 23   ──────┬───────────►  DE (Driver Enable)
                └───────────►  RE (Receiver Enable)
```

### Catatan Penting Wiring

1. **Power Supply:**
   - Sensor membutuhkan 5V DC (bukan 3.3V)
   - Arus konsumsi: ~50-100mA
   - Gunakan power supply yang stabil

2. **DE dan RE:**
   - Pin DE (Driver Enable) dan RE (Receiver Enable) HARUS digabung
   - Keduanya terhubung ke GPIO 23
   - HIGH = transmit mode (ESP32 mengirim data)
   - LOW = receive mode (ESP32 menerima data)

3. **RS485 A dan B:**
   - Tidak terhubung langsung ke ESP32
   - Sensor sudah memiliki modul RS485 built-in
   - Pin A dan B untuk koneksi ke perangkat RS485 lain (daisy chain)

4. **UART Pins:**
   - Menggunakan UART2 pada ESP32
   - RX2 = GPIO 16, TX2 = GPIO 17
   - Baud rate default: 9600 (cek datasheet sensor Anda)

## ⚙️ Konfigurasi Software

### Cara Kerja Kode

Kode ini membaca sensor NPK dengan cara **membaca satu register per satu** (sequential reading), bukan batch reading. Metode ini lebih reliable untuk sensor 7-in-1 NPK.

**Alur kerja:**
1. Set DE/RE pin ke HIGH → masuk transmit mode
2. Kirim request frame untuk 1 register via UART2
3. Flush serial buffer
4. Set DE/RE pin ke LOW → masuk receive mode  
5. Tunggu response dengan timeout 1000ms
6. Verifikasi CRC response
7. Extract data jika CRC valid
8. Delay 50ms sebelum request berikutnya
9. Ulangi untuk 7 register

**Update Interval:**
- NPK sensor dibaca setiap **1 detik** (1000ms)
- Total waktu per cycle: ~700ms (7 register × 100ms per register)
- Pembacaan dilakukan secara non-blocking di main loop

**Validasi:**
- Sistem menganggap sensor OK jika minimal 4 dari 7 readings berhasil
- Jika kurang dari 4, sensor dianggap error
- Nilai 0xFFFF menandakan register error (tidak terbaca)

### Pin Configuration (sudah ada di main.cpp)

```cpp
#define RS485_RX 16        // RO (Receiver Output) sensor ke ESP32
#define RS485_TX 17        // DI (Driver Input) ESP32 ke sensor  
#define RS485_DE_RE 23     // DE dan RE digabung
```

### Modbus Configuration

```cpp
const byte NPK_SENSOR_ADDRESS = 0x01;  // Address Modbus sensor (default 0x01)
const long NPK_BAUD_RATE = 4800;       // Baud rate 4800 (standard for this sensor)
```

**⚠️ PENTING:** 
- **Baud rate untuk sensor ini adalah 4800** (bukan 9600!)
- Modbus address default adalah 0x01
- Cek manual sensor Anda untuk memastikan

## 📊 Modbus Register Map

Register map standar untuk NPK sensor 7-in-1 (cek manual sensor Anda):

| Register | Parameter | Unit | Resolution | Range |
|----------|-----------|------|------------|-------|
| 0x0000 | Soil Humidity/Moisture | % | 0.1% | 0-100% |
| 0x0001 | Soil Temperature | °C | 0.1°C | -40 to 80°C |
| 0x0002 | Soil EC | µS/cm | 1 µS/cm | 0-20000 |
| 0x0003 | Soil pH | - | 0.1 | 0-14 |
| 0x0004 | Nitrogen (N) | mg/kg | 1 mg/kg | 0-1999 |
| 0x0005 | Phosphorus (P) | mg/kg | 1 mg/kg | 0-1999 |
| 0x0006 | Potassium (K) | mg/kg | 1 mg/kg | 0-1999 |

### Modbus Read Command

Function Code: **0x03** (Read Holding Registers)

**Contoh Request Frame:**
```
Address: 0x01
Function: 0x03
Start Register: 0x0000 (High: 0x00, Low: 0x00)
Number of Registers: 0x0007 (High: 0x00, Low: 0x07)
CRC: [calculated]

Complete frame: 01 03 00 00 00 07 [CRC_L] [CRC_H]
```

**Contoh Response Frame:**
```
Address: 0x01
Function: 0x03
Byte Count: 0x0E (14 bytes = 7 registers × 2 bytes)
Data: [14 bytes of sensor data]
CRC: [calculated]

Complete frame: 01 03 0E [14 data bytes] [CRC_L] [CRC_H]
```

## 🔧 Troubleshooting

### Sensor Tidak Terdeteksi

**Symptom:** Serial Monitor menampilkan "❌ NPK sensor failed (only X/7 valid readings)"

**Penyebab Umum:**
- Wiring tidak benar (RX/TX terbalik)
- Baud rate salah (harus 4800, bukan 9600)
- Power supply tidak stabil atau kurang dari 5V
- DE/RE pin tidak terhubung atau logic level salah

**Solusi:**
1. **Cek Wiring:**
   - Pastikan VCC terhubung ke 5V (bukan 3.3V)
   - Pastikan GND terhubung dengan benar
   - Cek koneksi RX/TX tidak terbalik (RO→GPIO16, DI→GPIO17)
   - Pastikan DE dan RE terhubung ke GPIO 23

2. **Cek Baud Rate:**
   ```cpp
   // Sensor 7-in-1 NPK menggunakan baud rate 4800
   const long NPK_BAUD_RATE = 4800;  // JANGAN ganti ke 9600!
   ```

3. **Cek Modbus Address:**
   ```cpp
   // Sensor mungkin memiliki address berbeda
   const byte NPK_SENSOR_ADDRESS = 0x01; // coba 0x02, 0x03, dst
   ```

4. **Test dengan USB-RS485 Converter:**
   - Gunakan USB-RS485 converter untuk test sensor
   - Gunakan software Modbus Master (ModScan, QModMaster)
   - Verifikasi address dan register map sensor

### CRC Error

**Symptom:** "❌ NPK Sensor: CRC mismatch"

**Solusi:**
1. **Cek Kualitas Koneksi:**
   - Pastikan kabel tidak terlalu panjang (max 100m untuk RS485)
   - Hindari interferensi dari kabel power AC
   - Gunakan twisted pair cable untuk A dan B

2. **Tambahkan Termination Resistor:**
   - Untuk kabel panjang, tambahkan resistor 120Ω antara A dan B
   - Pasang di ujung kabel terjauh dari master

3. **Power Supply Noise:**
   - Tambahkan kapasitor 100µF di dekat sensor (VCC ke GND)
   - Gunakan power supply yang stabil dan filtered

### Pembacaan Data Tidak Stabil

**Solusi:**
1. **Tambahkan Delay:**
   ```cpp
   // Dalam loop(), beri jeda lebih lama antara pembacaan
   const int SENSOR_READ_INTERVAL = 5; // 5 detik
   ```

2. **Kalibrasi Sensor:**
   - Sensor NPK perlu dikalibrasi dengan buffer solution
   - Ikuti prosedur kalibrasi dari manual sensor

3. **Warming Up:**
   - Sensor perlu waktu stabilisasi setelah power on
   - Tunggu 30 detik setelah boot sebelum membaca data

## 📡 Data Output

### Serial Monitor

Ketika sensor berhasil dibaca, Serial Monitor akan menampilkan:

```
=== 7-in-1 NPK Sensor Readings ===
Moisture: 45.2%
Temperature: 25.3°C
Conductivity: 1234 uS/cm (1.234 mS/cm)
pH: 6.8
Nitrogen (N): 82 mg/kg
Phosphorus (P): 45 mg/kg
Potassium (K): 156 mg/kg
Valid readings: 7/7
==================================
```

### Web Dashboard

Data NPK akan muncul dalam kartu sensor baru di dashboard web:

```
🌾 NPK Sensor 7-in-1 (RS485)
┌──────────────┬──────────────┐
│ Nitrogen (N) │ Phosphorus(P)│
│   82 mg/kg   │   45 mg/kg   │
├──────────────┼──────────────┤
│ Potassium(K) │  pH Level    │
│  156 mg/kg   │     6.8      │
├──────────────┼──────────────┤
│     EC       │  Soil Temp   │
│ 1.234 mS/cm  │   25.3°C     │
├──────────────┴──────────────┤
│      Soil Moisture (NPK)    │
│           45.2%             │
└─────────────────────────────┘
```

### MQTT Output

Data dipublikasikan ke topic `agrohygra/sensors` dalam format JSON:

```json
{
  "device": "AgroHygra-ESP32",
  "soil": 45,
  "temp": 28.5,
  "hum": 65.2,
  "npk": {
    "n": 82,
    "p": 45,
    "k": 156,
    "ph": 6.8,
    "ec": 1.234,
    "soilTemp": 25.3,
    "soilMoist": 45.2
  }
}
```

### REST API

Endpoint `/api/data` mengembalikan JSON lengkap:

```json
{
  "soilMoisture": 45,
  "temperature": 28.5,
  "humidity": 65.2,
  "npkSensor": {
    "available": true,
    "nitrogen": 82,
    "phosphorus": 45,
    "potassium": 156,
    "ph": 6.8,
    "ec": 1.234,
    "soilTemperature": 25.3,
    "soilMoisture": 45.2
  }
}
```

## 🌱 Interpretasi Data NPK

### Nitrogen (N)

| Level | Range (mg/kg) | Status | Action |
|-------|---------------|--------|--------|
| Sangat Rendah | < 30 | 🔴 Critical | Tambah pupuk nitrogen tinggi |
| Rendah | 30-60 | 🟡 Warning | Perlu pemupukan |
| Optimal | 60-150 | 🟢 Good | Pertahankan |
| Tinggi | 150-200 | 🟡 Caution | Kurangi pupuk |
| Sangat Tinggi | > 200 | 🔴 Excessive | Risiko kerusakan tanaman |

### Phosphorus (P)

| Level | Range (mg/kg) | Status | Action |
|-------|---------------|--------|--------|
| Sangat Rendah | < 10 | 🔴 Critical | Tambah pupuk fosfor |
| Rendah | 10-25 | 🟡 Warning | Perlu pemupukan |
| Optimal | 25-75 | 🟢 Good | Pertahankan |
| Tinggi | 75-100 | 🟡 Caution | Monitor |
| Sangat Tinggi | > 100 | 🔴 Excessive | Kurangi pupuk |

### Potassium (K)

| Level | Range (mg/kg) | Status | Action |
|-------|---------------|--------|--------|
| Sangat Rendah | < 80 | 🔴 Critical | Tambah pupuk kalium |
| Rendah | 80-120 | 🟡 Warning | Perlu pemupukan |
| Optimal | 120-200 | 🟢 Good | Pertahankan |
| Tinggi | 200-280 | 🟡 Caution | Monitor |
| Sangat Tinggi | > 280 | 🔴 Excessive | Kurangi pupuk |

### pH Level

| Level | Range | Status | Suitable Plants |
|-------|-------|--------|-----------------|
| Sangat Asam | < 5.0 | 🔴 | Blueberry, azalea |
| Asam | 5.0-6.0 | 🟡 | Kentang, strawberry |
| Sedikit Asam | 6.0-6.5 | 🟢 | Kebanyakan sayuran |
| Netral | 6.5-7.5 | 🟢 | Optimal untuk kebanyakan tanaman |
| Sedikit Basa | 7.5-8.0 | 🟡 | Asparagus, bawang |
| Basa | > 8.0 | 🔴 | Perlu koreksi |

### EC (Electrical Conductivity)

| Level | Range (mS/cm) | Status | Meaning |
|-------|---------------|--------|---------|
| Sangat Rendah | < 0.5 | 🟡 | Kurang nutrisi |
| Rendah | 0.5-1.0 | 🟢 | Baik untuk seedling |
| Optimal | 1.0-2.5 | 🟢 | Optimal untuk pertumbuhan |
| Tinggi | 2.5-4.0 | 🟡 | Risiko salt stress |
| Sangat Tinggi | > 4.0 | 🔴 | Berbahaya, flush dengan air |

## 🔬 Kalibrasi Sensor

### Kalibrasi pH

1. Siapkan buffer solution pH 4.0, 7.0, dan 10.0
2. Bersihkan probe dengan aquades
3. Celupkan probe ke buffer pH 7.0
4. Tunggu 30 detik hingga stabil
5. Catat pembacaan dan adjust dengan software kalibrasi
6. Ulangi untuk pH 4.0 dan 10.0
7. Bilas probe dengan aquades setelah kalibrasi

### Kalibrasi EC

1. Siapkan standard solution EC 1.413 mS/cm (atau sesuai manual)
2. Celupkan probe ke solution
3. Tunggu stabilisasi
4. Adjust reading dengan software kalibrasi
5. Bilas probe dengan aquades

### Kalibrasi NPK

Kalibrasi NPK memerlukan:
- Standard soil sample dengan konsentrasi N-P-K yang diketahui
- Laboratorium untuk analisis referensi
- Software kalibrasi dari manufacturer

**Note:** Untuk hasil akurat, lakukan kalibrasi NPK di laboratorium atau gunakan nilai kalibrasi dari pabrik.

## 📝 Tips Penggunaan

1. **Instalasi Probe:**
   - Masukkan probe 10-15 cm ke dalam tanah
   - Pastikan kontak penuh dengan tanah
   - Hindari udara trapped di sekitar probe

2. **Maintenance:**
   - Bersihkan probe setiap minggu dengan aquades
   - Hindari probe kering terlalu lama
   - Simpan probe dalam larutan KCl 3M saat tidak digunakan

3. **Pembacaan Optimal:**
   - Tunggu 5-10 detik setelah probe inserted
   - Ambil multiple readings dan rata-ratakan
   - Hindari pembacaan saat irigasi aktif

4. **Battery Life:**
   - Sensor konsumsi daya rendah (~50mA)
   - Bisa powered terus menerus dari ESP32
   - Untuk battery operation, baca setiap 15-30 menit

## 🔗 Resources

### Datasheet & Manual
- [RS485 Modbus Protocol Guide](https://www.modbustools.com/modbus.html)
- [Soil NPK Testing Standards](https://www.fao.org/soils-portal)

### Software Tools
- **QModMaster** - GUI Modbus master (testing sensor)
- **ModScan** - Windows Modbus scanner
- **pymodbus** - Python library untuk Modbus

### Sensor Alternatives
- Jika sensor ini tidak tersedia, alternatif:
  - **Gravity NPK Sensor** (DFRobot)
  - **SEN0244** - Analog NPK Sensor
  - **RS-ECTH-N01** - RS485 EC/pH sensor

## 📞 Support

Jika mengalami masalah:
1. Cek wiring diagram dengan multimeter
2. Test sensor dengan USB-RS485 converter + QModMaster
3. Verifikasi Modbus address dan register map dari manual sensor
4. Periksa baud rate dan communication settings
5. Hubungi manufacturer sensor untuk support teknis

---

**Version:** 1.0  
**Last Updated:** October 2025  
**Compatibility:** ESP32 + Arduino Framework + PlatformIO

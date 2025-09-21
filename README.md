# 🔥 Smart Stove Kit - Setup Guide

คู่มือการติดตั้งและตั้งค่าระบบควบคุมอุณหภูมิเตาอัจฉริยะ

## 📦 อุปกรณ์ที่ต้องใช้

### Hardware
- ESP32 Development Board
- MAX6675 Thermocouple Amplifier
- K-Type Thermocouple Sensor
- LCD I2C 16x2
- L298N Motor Driver Module
- DC Fan/Blower
- Jumper Wires
- Breadboard หรือ PCB

### Software
- Arduino IDE
- Blynk Mobile App

## 🔌 การต่อสาย (Wiring)

### ESP32 Connections

| อุปกรณ์ | Pin ESP32 | หมายเหตุ |
|---------|-----------|----------|
| **MAX6675** |  |  |
| SO | GPIO 19 | Data Out |
| CS | GPIO 5 | Chip Select |
| SCK | GPIO 18 | Clock |
| VCC | 3.3V |  |
| GND | GND |  |
| **LCD I2C** |  |  |
| SDA | GPIO 21 | I2C Data |
| SCL | GPIO 22 | I2C Clock |
| VCC | 5V |  |
| GND | GND |  |
| **L298N Motor Driver** |  |  |
| IN1 | GPIO 25 | Direction Control |
| EN1 | GPIO 26 | Speed Control (PWM) |
| VCC | 5V |  |
| GND | GND |  |
| **DC Fan** |  |  |
| Control | GPIO 2 | Fan On/Off |

## ⚙️ การติดตั้ง Software

### 1. ติดตั้ง Arduino IDE Libraries
```
Tools > Manage Libraries... แล้วติดตั้ง:
- MAX6675 library
- LiquidCrystal I2C
- Blynk (ESP32)
```

### 2. ตั้งค่า Board ใน Arduino IDE
```
File > Preferences > Additional Board Manager URLs:
https://dl.espressif.com/dl/package_esp32_index.json

Tools > Board > ESP32 Arduino > ESP32 Dev Module
```

## 📱 การตั้งค่า Blynk

### 1. สร้าง Template ใหม่
- เข้า [Blynk Console](https://blynk.cloud/)
- สร้าง Template ใหม่ชื่อ "Smart Stove Kit"
- เลือก Hardware: ESP32
- เลือก Connection: WiFi

### 2. ตั้งค่า Datastreams
| Virtual Pin | ชื่อ | Data Type | Min | Max | Default |
|-------------|------|-----------|-----|-----|---------|
| V2 | Current Temperature | Double | 0 | 150 | 0 |
| V3 | Target Temperature | Double | 20 | 100 | 35 |

### 3. สร้าง Mobile Dashboard
1. **Value Display** (V2)
   - แสดงอุณหภูมิปัจจุบัน
   - หน่วย: °C
   
2. **Slider** (V3)
   - ตั้งค่าอุณหภูมิเป้าหมาย
   - ช่วง: 20-100°C
   - ค่าเริ่มต้น: 35°C

### 4. รับ Auth Token
- ไปที่ Device > Device Info
- คัดลอก **BLYNK_AUTH_TOKEN**

## 🔧 การกำหนดค่าในโค้ด

### 1. ข้อมูล WiFi และ Blynk
```cpp
// ===== Wi-Fi =====
char ssid[] = "YOUR_WIFI_NAME";      // ชื่อ WiFi
char pass[] = "YOUR_WIFI_PASSWORD";  // รหัสผ่าน WiFi

// ===== Blynk Settings =====
#define BLYNK_AUTH_TOKEN "YOUR_AUTH_TOKEN_HERE"
```

### 2. การปรับแต่งระบบ

#### อุณหภูมิ
```cpp
const float MIN_TEMP = 20.0;   // อุณหภูมิต่ำสุดที่ตั้งได้
const float MAX_TEMP = 100.0;  // อุณหภูมิสูงสุดที่ตั้งได้
const float TEMP_BUFFER = 2.0; // ช่วงบัฟเฟอร์
float targetTemp = 35.0;       // อุณหภูมิเป้าหมายเริ่มต้น
```

#### ความแรงพัดลม
```cpp
const int FAN_SPEED_MAX = 255;    // ความแรงสูงสุด (100%)
const int FAN_SPEED_HIGH = 220;   // ความแรงสูง (86%)
const int FAN_SPEED_MED = 200;    // ความแรงปานกลาง (78%)
const int FAN_SPEED_LOW = 180;    // ความแรงต่ำ (71%)
const int FAN_SPEED_KEEP = 180;   // ความแรงรักษาระดับ (71%)
```

#### ช่วงความต่างอุณหภูมิ
```cpp
const float TEMP_DIFF_HIGH = 10.0;  // ≥10°C → ความแรงสูงสุด
const float TEMP_DIFF_MED = 5.0;    // ≥5°C  → ความแรงสูง
const float TEMP_DIFF_LOW = 2.0;    // ≥2°C  → ความแรงปานกลาง
```

### 3. ตรวจสอบ I2C Address ของ LCD
```cpp
LiquidCrystal_I2C lcd(0x27, 16, 2);  // ลองเปลี่ยนเป็น 0x3F ถ้าไม่ขึ้น
```

## 🚀 การใช้งาน

### 1. Upload โค้ด
- เชื่อมต่อ ESP32 กับคอมพิวเตอร์
- เลือก Port ที่ถูกต้อง
- กด Upload

### 2. ตรวจสอบการทำงาน
- เปิด Serial Monitor (9600 baud)
- ตรวจสอบการเชื่อมต่อ WiFi
- ตรวจสอบการอ่านค่าอุณหภูมิ

### 3. ใช้งานผ่าน Blynk App
- ดูอุณหภูมิปัจจุบันใน Value Display
- ปรับอุณหภูมิเป้าหมายด้วย Slider
- ระบบจะควบคุมพัดลมอัตโนมัติ

## 📊 การทำงานของระบบ

### Logic การควบคุม
1. **อุณหภูมิต่ำกว่าเป้าหมาย** → เปิดพัดลมเป่าไฟแรงขึ้น
2. **อุณหภูมิใกล้เป้าหมาย** → รักษาระดับด้วยลมเบา
3. **อุณหภูมิสูงเกิน** → ปิดพัดลมให้อุณหภูมิลดลง

### ระดับความแรงพัดลม
- **ความต่าง ≥10°C** → ความแรง 100% (255)
- **ความต่าง ≥5°C** → ความแรง 86% (220)
- **ความต่าง ≥2°C** → ความแรง 78% (200)
- **ความต่าง <2°C** → ความแรง 71% (180)
- **รักษาระดับ** → ความแรง 71% (180)

## 🔍 การแก้ไขปัญหา

### LCD ไม่แสดงผล
- ตรวจสอบการต่อสาย SDA, SCL
- ลองเปลี่ยน address เป็น 0x3F
- ใช้ I2C Scanner หา address ที่ถูกต้อง

### เซ็นเซอร์อุณหภูมิอ่านค่าผิด
- ตรวจสอบการต่อสาย MAX6675
- ตรวจสอบ Thermocouple ไม่ขาด
- ตรวจสอบแหล่งจ่ายไฟ 3.3V

### พัดลมไม่หมุน
- ตรวจสอบการต่อสาย L298N
- ตรวจสอบแหล่งจ่ายไฟ 5V
- ตรวจสอบ GPIO pins

### เชื่อมต่อ WiFi ไม่ได้
- ตรวจสอบชื่อและรหัสผ่าน WiFi
- ตรวจสอบสัญญาณ WiFi
- ลอง Reset ESP32

## 📝 หมายเหตุ

- ตรวจสอบการต่อสายให้ถูกต้องก่อนเปิดไฟ
- ใช้แหล่งจ่ายไฟที่เหมาะสมสำหรับ DC Fan
- ระวังความร้อนจาก Thermocouple
- สำรองข้อมูลโค้ดไว้เสมอ

## 🆘 การสนับสนุน

หากพบปัญหาในการใช้งาน:
1. ตรวจสอบ Serial Monitor เพื่อดู error messages
2. ตรวจสอบการต่อสายอีกครั้ง
3. ลองใช้ค่าเริ่มต้นในโค้ด
4. อ่านเอกสารของ Library ที่ใช้

---
**⚠️ คำเตือน**: ใช้งานด้วยความระมัดระวัง เนื่องจากเกี่ยวข้องกับอุณหภูมิสูงและไฟฟ้า
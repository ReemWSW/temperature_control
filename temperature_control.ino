// ===== Blynk Settings =====
#define BLYNK_TEMPLATE_ID "TMPL6a4Grdiv2"
#define BLYNK_TEMPLATE_NAME "Smart Stove Kit"
#define BLYNK_AUTH_TOKEN    "CQ418r4ls_nughsmz6iNloT-g45pKRB9"
 
#define IN1 25
#define EN1 26

#include <max6675.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <WiFi.h>
#include <BlynkSimpleEsp32.h>




LiquidCrystal_I2C lcd(0x27, 16, 2);  
int thermoSO = 19;                   
int thermoCS = 5;                    
int thermoSCK = 18;                  
MAX6675 thermocouple(thermoSCK, thermoCS, thermoSO);

// ===== Wi-Fi =====
char ssid[] = "WPYM-COM 2.4G";
char pass[] = "0817632661";

int dcfan = 2;
float tempC = 0;
float targetTemp = 35.0;  // อุณหภูมิเป้าหมาย เริ่มต้น 35°C
int fanSpeed = 0;         // ความแรงของพัดลม (0-255)
bool fanStatus = false;

void setup() {
  pinMode(dcfan, OUTPUT);
  Serial.begin(9600);
  
  lcd.init();       
  lcd.backlight();  
  lcd.setCursor(0, 0);
  lcd.print("Smart Stove Kit");
  lcd.setCursor(0, 1);
  lcd.print("Starting...");
  
  pinMode(IN1, OUTPUT);
  ledcAttach(EN1, 1000, 8);
  digitalWrite(IN1, HIGH);
  
  // เชื่อมต่อ Blynk
  Blynk.begin(BLYNK_AUTH_TOKEN, ssid, pass);
  
  delay(2000);
  Serial.println("System Ready!");
  Serial.print("Target Temperature: ");
  Serial.print(targetTemp);
  Serial.println("°C");
}

void loop() {
  tempC = thermocouple.readCelsius();
  
  // ตรวจสอบข้อผิดพลาดของเซ็นเซอร์
  if (isnan(tempC) || tempC <= 0) {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Sensor Error!");
    Serial.println("Temperature sensor error!");
    delay(1000);
    return;
  }
  
  // ควบคุมพัดลมตามอุณหภูมิเป้าหมาย
  controlFanByTemp();
  
  // แสดงผลบน LCD
  updateDisplay();
  
  // แสดงใน Serial Monitor
  Serial.print("Current: ");
  Serial.print(tempC);
  Serial.print("°C, Target: ");
  Serial.print(targetTemp);
  Serial.print("°C, Fan Speed: ");
  Serial.print(fanSpeed);
  Serial.print(", Status: ");
  Serial.println(fanStatus ? "ON" : "OFF");
  
  delay(500);
  Blynk.run();
}

void controlFanByTemp() {
  float tempDiff = targetTemp - tempC;  // ความต่างของอุณหภูมิ
  
  if (tempC < targetTemp) {
    // อุณหภูมิต่ำกว่าเป้าหมาย → เปิดพัดลมเป่าไฟให้แรงขึ้น
    fanStatus = true;
    
    // คำนวณความแรงของพัดลมตามความต่างของอุณหภูมิ
    if (tempDiff >= 10) {
      fanSpeed = 255;  // ความแรงสูงสุด
    } else if (tempDiff >= 5) {
      fanSpeed = 200;  // ความแรงปานกลาง-สูง
    } else if (tempDiff >= 2) {
      fanSpeed = 150;  // ความแรงปานกลาง
    } else {
      fanSpeed = 100;  // ความแรงต่ำ
    }
    
    digitalWrite(dcfan, HIGH);
    ledcWrite(EN1, fanSpeed);
    
  } else if (tempC > targetTemp + 2) {
    // อุณหภูมิสูงเกินเป้าหมาย (มี buffer 2°C) → ปิดพัดลม
    fanStatus = false;
    fanSpeed = 0;
    digitalWrite(dcfan, LOW);
    ledcWrite(EN1, 0);
    
  } else {
    // อุณหภูมิใกล้เป้าหมาย → ลดความแรงลง
    fanStatus = true;
    fanSpeed = 80;  // ความแรงต่ำมาก เพื่อรักษาอุณหภูมิ
    digitalWrite(dcfan, HIGH);
    ledcWrite(EN1, fanSpeed);
  }
}

void updateDisplay() {
  lcd.clear();
  
  // บรรทัดที่ 1: อุณหภูมิปัจจุบัน และ เป้าหมาย
  lcd.setCursor(0, 0);
  lcd.print("T:");
  lcd.print(tempC, 1);
  lcd.print(" G:");
  lcd.print(targetTemp, 0);
  lcd.print("C");
  
  // บรรทัดที่ 2: สถานะพัดลม และ ความแรง
  lcd.setCursor(0, 1);
  lcd.print("Fan:");
  if (fanStatus) {
    lcd.print("ON ");
    lcd.print(map(fanSpeed, 0, 255, 0, 100));  // แสดงเปอร์เซ็นต์
    lcd.print("%");
  } else {
    lcd.print("OFF   ");
  }
}

// ===== Blynk Functions =====
BLYNK_READ(V2) {
  // ส่งค่าอุณหภูมิปัจจุบันไปแอพ
  Blynk.virtualWrite(V2, tempC);
}

BLYNK_WRITE(V3) {
  // รับค่าอุณหภูมิเป้าหมายจากแอพ
  float newTarget = param.asFloat();
  
  // ตรวจสอบช่วงอุณหภูมิที่สมเหตุสมผล
  if (newTarget >= 35 && newTarget <= ) {
    targetTemp = newTarget;
    Serial.print("New target temperature set: ");
    Serial.print(targetTemp);
    Serial.println("°C");
  } else {
    Serial.println("Invalid temperature range! (20-100°C)");
  }
}

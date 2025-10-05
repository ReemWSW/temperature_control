#include <max6675.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <WiFi.h>

// ===== ตั้งค่า Blynk =====
#define BLYNK_TEMPLATE_ID "TMPL6a4Grdiv2"
#define BLYNK_TEMPLATE_NAME "Smart Stove Kit"
#define BLYNK_AUTH_TOKEN    "CQ418r4ls_nughsmz6iNloT-g45pKRB9"

#include <BlynkSimpleEsp32.h>

#define IN1 25
#define EN1 26

LiquidCrystal_I2C lcd(0x27, 16, 2);
int thermoSO = 19;
int thermoCS = 5;
int thermoSCK = 18;
MAX6675 thermocouple(thermoSCK, thermoCS, thermoSO);

// ===== ปุ่มสวิตช์ =====
const int BUTTON_UP = 32;           // ปุ่มเพิ่มอุณหภูมิ
const int BUTTON_DOWN = 33;         // ปุ่มลดอุณหภูมิ

// ===== ตั้งค่า Wi-Fi =====
char ssid[] = "WPYM-COM 2.4G";
char pass[] = "0817632661";

int dcfan = 2;
float tempC = 0;
float targetTemp = 35.0;  // อุณหภูมิเป้าหมาย เริ่มต้น 35°C
int fanSpeed = 0;         // ความแรงของพัดลม (0-255)
bool fanStatus = false;

// ===== การตั้งค่าอุณหภูมิ =====
const float MIN_TEMP = 35.0;   // อุณหภูมิต่ำสุดที่ตั้งได้
const float MAX_TEMP = 200.0;  // อุณหภูมิสูงสุดที่ตั้งได้
const float TEMP_BUFFER = 2.0; // ช่วงบัฟเฟอร์เพื่อป้องกันการเปิดปิดบ่อย
const float TEMP_STEP = 5.0;   // ปรับทีละ 5°C

// ===== ระดับความแรงพัดลม =====
const int FAN_SPEED_MAX = 255;    // ความแรงสูงสุด - เมื่ออุณหภูมิต่ำกว่าเป้าหมายมาก
const int FAN_SPEED_HIGH = 200;   // ความแรงสูง
const int FAN_SPEED_MED = 150;    // ความแรงปานกลาง
const int FAN_SPEED_LOW = 100;    // ความแรงต่ำ - เมื่อใกล้ถึงเป้าหมาย
const int FAN_SPEED_KEEP = 80;    // ความแรงรักษาระดับ - เมื่ออยู่ในช่วงเป้าหมาย

// ===== ช่วงความต่างอุณหภูมิ =====
const float TEMP_DIFF_HIGH = 10.0;  // ความต่าง >= 10°C
const float TEMP_DIFF_MED = 5.0;    // ความต่าง >= 5°C
const float TEMP_DIFF_LOW = 2.0;    // ความต่าง >= 2°C

// Anti-bounce
unsigned long lastButtonPress = 0;
const unsigned long DEBOUNCE_DELAY = 200; // 200ms

void setup() {
  Serial.begin(115200);
  delay(500);
  Serial.println("\n\n=== System Starting ===");

  pinMode(dcfan, OUTPUT);
  pinMode(IN1, OUTPUT);
  digitalWrite(dcfan, LOW);
  digitalWrite(IN1, HIGH);

  ledcAttach(EN1, 25000, 8);
  ledcWrite(EN1, 0);

  // ตั้งค่าปุ่ม
  pinMode(BUTTON_UP, INPUT_PULLUP);
  pinMode(BUTTON_DOWN, INPUT_PULLUP);

  lcd.init();
  lcd.backlight();
  lcd.setCursor(0, 0);
  lcd.print("Smart Temperatur");
  lcd.setCursor(0, 1);
  lcd.print("Init...");

  // เชื่อมต่อ Wi-Fi ก่อน
  Serial.println("Connecting WiFi...");
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, pass);

  int timeout = 0;
  while (WiFi.status() != WL_CONNECTED && timeout < 15) {
    delay(500);
    Serial.print(".");
    timeout++;
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\nWiFi OK!");
    Serial.print("IP: ");
    Serial.println(WiFi.localIP());

    Blynk.config(BLYNK_AUTH_TOKEN);
    Blynk.connect(3000);  // timeout 3 วินาที
  } else {
    Serial.println("\nWiFi FAIL - Continue without Blynk");
  }

  Serial.println("=== System Ready ===");
  Serial.print("Target Temperature: ");
  Serial.print(targetTemp);
  Serial.print("°C (Range: ");
  Serial.print(MIN_TEMP);
  Serial.print("-");
  Serial.print(MAX_TEMP);
  Serial.println("°C)");
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

  // ส่งไป Blynk ทันทีทุกครั้ง (Real-time)
  if (Blynk.connected()) {
    Blynk.virtualWrite(V2, tempC);
  }

  // ตรวจสอบการกดปุ่ม
  checkButtons();

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

void checkButtons() {
  unsigned long currentTime = millis();

  // ป้องกันกดซ้ำเร็วเกินไป
  if (currentTime - lastButtonPress < DEBOUNCE_DELAY) {
    return;
  }

  // ปุ่มเพิ่ม (กด = LOW เพราะใช้ INPUT_PULLUP)
  if (digitalRead(BUTTON_UP) == LOW) {
    targetTemp += TEMP_STEP;
    if (targetTemp > MAX_TEMP) targetTemp = MAX_TEMP;

    lastButtonPress = currentTime;
    Serial.print("Button UP - New target: ");
    Serial.print(targetTemp);
    Serial.println("°C");

    // ส่งไป Blynk
    if (Blynk.connected()) {
      Blynk.virtualWrite(V3, targetTemp);
    }
  }

  // ปุ่มลด
  if (digitalRead(BUTTON_DOWN) == LOW) {
    targetTemp -= TEMP_STEP;
    if (targetTemp < MIN_TEMP) targetTemp = MIN_TEMP;

    lastButtonPress = currentTime;
    Serial.print("Button DOWN - New target: ");
    Serial.print(targetTemp);
    Serial.println("°C");

    // ส่งไป Blynk
    if (Blynk.connected()) {
      Blynk.virtualWrite(V3, targetTemp);
    }
  }
}

void controlFanByTemp() {
  float tempDiff = targetTemp - tempC;  // ความต่างของอุณหภูมิ

  if (tempC < targetTemp) {
    // อุณหภูมิต่ำกว่าเป้าหมาย → เปิดพัดลมเป่าไฟให้แรงขึ้น
    fanStatus = true;

    // คำนวณความแรงของพัดลมตามความต่างของอุณหภูมิ
    if (tempDiff >= TEMP_DIFF_HIGH) {
      fanSpeed = FAN_SPEED_MAX;  // ความแรงสูงสุด
    } else if (tempDiff >= TEMP_DIFF_MED) {
      fanSpeed = FAN_SPEED_HIGH;  // ความแรงสูง
    } else if (tempDiff >= TEMP_DIFF_LOW) {
      fanSpeed = FAN_SPEED_MED;  // ความแรงปานกลาง
    } else {
      fanSpeed = FAN_SPEED_LOW;  // ความแรงต่ำ
    }

    digitalWrite(dcfan, HIGH);
    ledcWrite(EN1, fanSpeed);

  } else if (tempC > targetTemp + TEMP_BUFFER) {
    // อุณหภูมิสูงเกินเป้าหมาย (มี buffer) → ปิดพัดลม
    fanStatus = false;
    fanSpeed = 0;
    digitalWrite(dcfan, LOW);
    ledcWrite(EN1, 0);

  } else {
    // อุณหภูมิใกล้เป้าหมาย → ลดความแรงลง
    fanStatus = true;
    fanSpeed = FAN_SPEED_KEEP;  // ความแรงรักษาระดับ
    digitalWrite(dcfan, HIGH);
    ledcWrite(EN1, fanSpeed);
  }
}

void updateDisplay() {
  lcd.clear();

  // อุณหภูมิปัจจุบัน
  lcd.setCursor(0, 0);
  lcd.print("Temp: ");
  lcd.print(tempC, 1);
  lcd.print(" C");

  // อุณหภูมิที่ตั้ง
  lcd.setCursor(0, 1);
  lcd.print("Set : ");
  lcd.print(targetTemp, 1);
  lcd.print(" C");
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
  if (newTarget >= MIN_TEMP && newTarget <= MAX_TEMP) {
    targetTemp = newTarget;
    Serial.print("New target temperature set: ");
    Serial.print(targetTemp);
    Serial.println("°C");
  } else {
    Serial.print("Invalid temperature range! (");
    Serial.print(MIN_TEMP);
    Serial.print("-");
    Serial.print(MAX_TEMP);
    Serial.println("°C)");
  }
}

#include <Arduino.h>
#include <EEPROM.h>
#include <LiquidCrystal_I2C.h>

// #include <PZEM004Tv30.h>
#include "../lib/PHSensor/PHSensor.h"
#include "../lib/TDSSensor/TDSSensor.h"
#include "../lib/TempSensor/TempSensor.h"
// #include "../lib/FlowSensor/FlowSensor.h"
#include "../lib/TimerInterrupt/TimerInterrupt.h"
#include "../lib/WaterLevel/WaterLevel.h"

// LCD (tạm thời không sử dụng)
LiquidCrystal_I2C lcd(0x27, 16, 2);

// Relay control: đèn PB0, sủi PB14, bơm PB8
const uint8_t relayPins[] = {PB0, PB14, PB8};
const uint8_t RELAY_COUNT = sizeof(relayPins) / sizeof(relayPins[0]);
// HardwareSerial pzemSerial(PA3, PA2);
// PZEM004Tv30 pzem(pzemSerial);

// Custom Sensor Objects
PHSensor phSensor(PA5, 25.0);        // pH sensor
TDSSensor tdsSensor(PB1, 3.3, 4096); // TDS sensor
TempSensor tempSensor(PA12, PA11, PA7, PA6, 430.0,
                      100.0); // PT100 sensor via MAX31865
// FlowSensor flowSensor(PA1, 5.5);                           // Flow sensor
WaterLevel waterLevelSensor(PA4); // Khởi tạo cảm biến Y26 ở chân PA3

// Sensor Data Variables
float temperature = 25.0;
float phValue = 7.0;
float tdsValue = 0.0;
// float flowRate = 0.0;
bool hasWater = false; // Biến lưu trạng thái mực nước
// float voltage = 0.0;
// float current = 0.0;
// float power = 0.0;
bool relayStates[RELAY_COUNT] = {false, false, false};

// Timing & Status
bool sensorReady = false;

// Calibration mode
bool phCalMode = false;
bool waitingForCalValue = false;

// Timer Objects - Sử dụng TIM2, TIM3, TIM4
TimerInterrupt sensorTimer(TIM2); // Timer cho tất cả sensors
// TimerInterrupt pzemTimer(TIM4);   // Timer cho PZEM
TimerInterrupt lcdTimer(TIM3); // Timer cho LCD update

// Interrupt Flags
volatile bool readSensorsFlag = false;
// volatile bool readPzemFlag = false;
volatile bool updateLCDFlag = false;

// Timer Callbacks - Chỉ set flag, không xử lý logic
void sensorTimerCallback() { readSensorsFlag = true; }
void lcdTimerCallback() { updateLCDFlag = true; }

// Serial Buffer
String serialBuffer = "";

void setRelayState(uint8_t relayIndex, bool enabled) {
  if (relayIndex >= RELAY_COUNT) {
    return;
  }

  relayStates[relayIndex] = enabled;
  digitalWrite(relayPins[relayIndex], enabled ? LOW : HIGH);
}

void setAllRelaysState(bool enabled) {
  for (uint8_t i = 0; i < RELAY_COUNT; ++i) {
    setRelayState(i, enabled);
  }
}
//
// void readPzemData()
// {
//   voltage = pzem.voltage();
//   current = pzem.current();
//   power = pzem.power();
//
//   if (isnan(voltage))
//     voltage = 0.0;
//   if (isnan(current))
//     current = 0.0;
//   if (isnan(power))
//     power = 0.0;
// }

void sendTelemetryToEsp32() {
  // Chỉ đóng gói các biến cảm biến, không đính kèm relayStates vào nữa
  String jsonData = "{\"temp\":" + String(temperature, 1) +
                    ",\"pH\":" + String(phValue, 2) +
                    ",\"tds\":" + String(tdsValue, 0) +
                    // ",\"flow\":" + String(flowRate, 1) +
                    ",\"water\":" + String(hasWater ? 1 : 0) +
                    // ",\"voltage\":" + String(voltage, 1) +
                    // ",\"current\":" + String(current, 2) +
                    // ",\"power\":" + String(power, 1) +
                    "}";

  Serial3.println(jsonData);
}

/*
void processRelayCommand(String cmd)
{
  cmd.trim();

  int firstSeparator = cmd.indexOf(':');
  int secondSeparator = cmd.indexOf(':', firstSeparator + 1);
  if (firstSeparator < 0 || secondSeparator < 0)
  {
    Serial3.println(">>> Invalid relay command");
    return;
  }

  String target = cmd.substring(firstSeparator + 1, secondSeparator);
  String action = cmd.substring(secondSeparator + 1);
  target.trim();
  action.trim();
  target.toUpperCase();
  action.toUpperCase();

  if (target == "ALL")
  {
    if (action == "TURN_ON" || action == "ON" || action == "1")
    {
      setAllRelaysState(true);
    }
    else if (action == "TURN_OFF" || action == "OFF" || action == "0")
    {
      setAllRelaysState(false);
    }
    else if (action == "TOGGLE")
    {
      bool anyOn = false;
      for (uint8_t i = 0; i < RELAY_COUNT; ++i)
      {
        anyOn = anyOn || relayStates[i];
      }
      setAllRelaysState(!anyOn);
    }

    Serial3.print(">>> Relay ALL = ");
    Serial3.println(action);
    return;
  }

  uint8_t relayNumber = static_cast<uint8_t>(target.toInt());
  if (relayNumber == 0)
  {
    if (action == "TURN_ON" || action == "ON" || action == "1")
    {
      setAllRelaysState(true);
    }
    else if (action == "TURN_OFF" || action == "OFF" || action == "0")
    {
      setAllRelaysState(false);
    }
    else if (action == "TOGGLE")
    {
      bool anyOn = false;
      for (uint8_t i = 0; i < RELAY_COUNT; ++i)
      {
        anyOn = anyOn || relayStates[i];
      }
      setAllRelaysState(!anyOn);
    }

    Serial3.print(">>> Relay ALL = ");
    Serial3.println(action);
    return;
  }

  if (relayNumber > RELAY_COUNT)
  {
    Serial3.println(">>> Relay index out of range");
    return;
  }

  uint8_t relayIndex = relayNumber - 1;
  if (action == "TURN_ON" || action == "ON" || action == "1")
  {
    setRelayState(relayIndex, true);
  }
  else if (action == "TURN_OFF" || action == "OFF" || action == "0")
  {
    setRelayState(relayIndex, false);
  }
  else if (action == "TOGGLE")
  {
    setRelayState(relayIndex, !relayStates[relayIndex]);
  }

  Serial3.print(">>> Relay ");
  Serial3.print(relayNumber);
  Serial3.print(" = ");
  Serial3.println(action);
}
*/

uint8_t relayIndexFromTarget(String target) {
  target.trim();
  target.toUpperCase();

  if (target == "1" || target == "DEN" || target == "LIGHT") {
    return 0;
  }
  if (target == "2" || target == "SUI" || target == "AERATOR") {
    return 1;
  }
  if (target == "3" || target == "BOM" || target == "PUMP") {
    return 2;
  }

  return 255;
}

void processRelayCommand(String cmd) {
  cmd.trim();

  int firstSeparator = cmd.indexOf(':');
  int secondSeparator = cmd.indexOf(':', firstSeparator + 1);
  if (firstSeparator < 0 || secondSeparator < 0) {
    Serial3.println(">>> Invalid relay command");
    return;
  }

  String target = cmd.substring(firstSeparator + 1, secondSeparator);
  String action = cmd.substring(secondSeparator + 1);
  target.trim();
  action.trim();
  target.toUpperCase();
  action.toUpperCase();

  if (target == "ALL" || target == "0") {
    if (action == "TURN_ON" || action == "ON" || action == "1") {
      setAllRelaysState(true);
    } else if (action == "TURN_OFF" || action == "OFF" || action == "0") {
      setAllRelaysState(false);
    } else if (action == "TOGGLE") {
      bool anyOn = false;
      for (uint8_t i = 0; i < RELAY_COUNT; ++i) {
        anyOn = anyOn || relayStates[i];
      }
      setAllRelaysState(!anyOn);
    }

    Serial3.print(">>> Relay ALL = ");
    Serial3.println(action);
    return;
  }

  uint8_t relayIndex = relayIndexFromTarget(target);
  if (relayIndex == 255) {
    Serial3.println(">>> Relay index out of range");
    return;
  }

  if (action == "TURN_ON" || action == "ON" || action == "1") {
    setRelayState(relayIndex, true);
  } else if (action == "TURN_OFF" || action == "OFF" || action == "0") {
    setRelayState(relayIndex, false);
  } else if (action == "TOGGLE") {
    setRelayState(relayIndex, !relayStates[relayIndex]);
  }

  Serial3.print(">>> Relay ");
  Serial3.print(relayIndex + 1);
  Serial3.print(" = ");
  Serial3.println(action);
}

// Xử lý lệnh hiệu chuẩn từ ESP32
void processIncomingCommand(String cmd) {
  cmd.trim();

  if (cmd.startsWith("CMD:") || cmd.startsWith("cmd:")) {
    processRelayCommand(cmd);
    return;
  }

  // DEBUG
  Serial.print(">>> Received: [");
  Serial.print(cmd);
  Serial.println("]");
  Serial3.print(">>> Received: [");
  Serial3.print(cmd);
  Serial3.println("]");

  cmd.toLowerCase();

  if (cmd == "enterph") {
    phCalMode = true;
    Serial3.println(">>> Enter pH calibration mode");
    Serial3.println(">>> Put probe in buffer solution");
    Serial3.println(">>> Send: 4.0, 7.0, or 10.0");
    // lcd.clear();
    // lcd.setCursor(0, 0);
    // lcd.print("pH CAL MODE");
  } else if (cmd == "exitph") {
    phCalMode = false;
    Serial.println("exitph");
    delay(100);
    Serial3.println(">>> Exit pH calibration - Saved!");
    // lcd.clear();
    // lcd.setCursor(0, 0);
    // lcd.print("CAL SAVED");
    delay(1000);
  } else if (cmd == "4.0" || cmd == "7.0" || cmd == "10.0") {
    if (phCalMode) {
      Serial3.print(">>> Calibrating at pH ");
      Serial3.println(cmd);

      // Gửi lệnh DFRobot_PH
      Serial.println("enterph");
      delay(200);
      Serial.println("calph");
      delay(200);
      Serial.println(cmd);
      delay(500);

      Serial3.print(">>> Calibrated at pH ");
      Serial3.println(cmd);
      Serial3.println(">>> Send 'exitph' to save or another value");

      // lcd.clear();
      // lcd.setCursor(0, 0);
      // lcd.print("CAL pH ");
      // lcd.print(cmd);
      // lcd.setCursor(0, 1);
      // lcd.print("SUCCESS!");
      delay(1500);
    } else {
      Serial3.println(">>> Error: Send 'enterph' first!");
    }
  } else {
    Serial3.print(">>> Unknown: [");
    Serial3.print(cmd);
    Serial3.println("]");
  }
}

void setup() {
  Serial.begin(115200);  // USB hoặc Debug
  Serial3.begin(115200); // UART to ESP32 (PB10=TX3, PB11=RX3)
  // pzemSerial.begin(9600);

  pinMode(PC13, OUTPUT);
  analogReadResolution(12);

  Serial.println("=== System Starting ===");
  Serial3.println("=== STM32 System Ready ===");

  // LCD (tạm thời không sử dụng)
  lcd.init();
  lcd.backlight();
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("System Init...");
  delay(1000);

  // Khởi tạo các Custom Sensors
  Serial.println("Init Sensors...");

  // PT100 Temperature Sensor
  if (tempSensor.begin()) {
    Serial.println("PT100: OK");
    Serial3.println("PT100: OK");
  } else {
    Serial.print("PT100: FAILED (Fault: 0x");
    Serial.print(tempSensor.lastFault, HEX);
    Serial.println(")");

    Serial3.print("PT100: FAILED (Fault: 0x");
    Serial3.print(tempSensor.lastFault, HEX);
    Serial3.println(")");
  }

  // pH Sensor
  phSensor.begin();
  Serial.println("pH: OK");
  Serial3.println("pH: OK");

  // TDS Sensor
  tdsSensor.begin();
  Serial.println("TDS: OK");
  Serial3.println("TDS: OK");

  // Flow Sensor
  // flowSensor.begin();
  // Serial.println("Flow: OK");
  // Serial3.println("Flow: OK");

  // Khởi tạo cảm biến mực nước Y26
  waterLevelSensor.begin();
  Serial.println("Water Level: OK");
  Serial3.println("Water Level: OK");

  // Khởi tạo relay active-low
  for (uint8_t i = 0; i < RELAY_COUNT; ++i) {
    pinMode(relayPins[i], OUTPUT);
    digitalWrite(relayPins[i], HIGH);
  }
  setAllRelaysState(false);
  Serial.println("Relay: OK");
  Serial3.println("Relay: OK");

  // Khởi tạo PZEM
  // Serial.println("PZEM: OK");
  // Serial3.println("PZEM: OK");
  // setAllRelaysState(false);

  sensorReady = true;

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Ready!");
  delay(1000);
  lcd.clear();

  // Khởi tạo Timer Interrupts
  Serial.println("Init Timers...");
  // Bỏ Hardware Timer vì bộ đếm 16-bit của STM32F103 chỉ chịu được tối đa 59.6
  // giây! sensorTimer.begin(60000, sensorTimerCallback); pzemTimer.begin(2000,
  // pzemTimerCallback);     // Đọc PZEM mỗi 2000ms
  lcdTimer.begin(500, lcdTimerCallback); // Update LCD mỗi 500ms
  Serial.println("Timers: OK");
  Serial3.println("Timers: OK");
  Serial3.println("Using TIM2 (sensors)");

  Serial.println("=== System Ready ===");
  Serial3.println("=== Ready to receive commands ===");
}

void loop() {
  unsigned long currentMillis = millis();

  // ==== NHÁY LED CHỈ THỊ HOẠT ĐỘNG (Mỗi 500ms) ====
  static unsigned long lastBlinkTime = 0;
  if (currentMillis - lastBlinkTime >= 500) {
    lastBlinkTime = currentMillis;
    digitalWrite(PC13, !digitalRead(PC13));
  }

  // ==== HẸN GIỜ ĐỌC CẢM BIẾN (Mỗi 60s) BẰNG MILLIS ====
  static unsigned long lastSensorTime = 0;
  if (currentMillis - lastSensorTime >= 60000) {
    lastSensorTime = currentMillis;
    readSensorsFlag = true;
  }

  // ==== NHẬN LỆNH TỪ ESP32 qua Serial3 ====
  while (Serial3.available()) {
    char c = Serial3.read();

    if (c == '\n' || c == '\r') {
      if (serialBuffer.length() > 0) {
        processIncomingCommand(serialBuffer);
        serialBuffer = "";
      }
    } else {
      serialBuffer += c;
    }
  }

  // ==== CẬP NHẬT FLOW: Liên tục (handled by FlowSensor) ====
  // flowSensor.update();
  // flowRate = flowSensor.getFlowRate();

  // ==== ĐỌC PZEM: Khi có interrupt từ TIM4 ====
  // if (readPzemFlag)
  // {
  //   readPzemFlag = false;
  //   readPzemData();
  //   sendTelemetryToEsp32();
  // }

  // ==== ĐỌC SENSORS: Khi có interrupt từ TIM2 ====
  if (readSensorsFlag) {
    readSensorsFlag = false;

    // Đọc nhiệt độ PT100
    if (tempSensor.isReady()) {
      float temp = tempSensor.readTemperature();
      if (temp > -50 && temp < 200) {
        temperature = temp;
      }
    }

    // Đọc TDS với temperature compensation
    tdsSensor.setTemperature(temperature);
    tdsValue = tdsSensor.readTDS();

    // Đọc pH với temperature compensation
    phSensor.setTemperature(temperature);
    phValue = phSensor.readPH();

    // Đọc trạng thái mực nước
    hasWater = waterLevelSensor.isWaterPresent();

    // Gửi data lên ESP32 dạng JSON
    sendTelemetryToEsp32();
  }

  // ==== CẬP NHẬT LCD: Khi có interrupt từ TIM3 ====
  if (updateLCDFlag) {
    updateLCDFlag = false;

    if (!phCalMode) {
      lcd.clear();

      lcd.setCursor(0, 0);
      lcd.print("T:");
      lcd.print(temperature, 1);
      lcd.print(" P:");
      lcd.print(phValue, 1);

      lcd.setCursor(0, 1);
      lcd.print("TDS:");
      lcd.print((int)tdsValue);
      lcd.print(" W:");
      lcd.print(hasWater ? "1" : "0");
    }
  }

  // ==== CALIBRATION ====
  phSensor.calibration(temperature);
}
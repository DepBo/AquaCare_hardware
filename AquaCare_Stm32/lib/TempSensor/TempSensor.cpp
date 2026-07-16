#include "TempSensor.h"

TempSensor::TempSensor(uint8_t cs, uint8_t di, uint8_t dout, uint8_t clk,
                       float refResistor, float nominalResistor) {
  csPin = cs;
  diPin = di;
  doPin = dout;
  clkPin = clk;
  rref = refResistor;
  rnominal = nominalResistor;
  isOk = false;
  lastFault = 0;

  rtd = new Adafruit_MAX31865(csPin, diPin, doPin, clkPin);
}

TempSensor::~TempSensor() { delete rtd; }

bool TempSensor::begin() {
  // Thêm timeout để tránh treo
  unsigned long startTime = millis();

  if (rtd->begin(MAX31865_3WIRE)) {
    isOk = true;
    delay(100);

    lastFault = rtd->readFault();
    if (lastFault) {
      rtd->clearFault();
      isOk = false;
    }
  } else {
    lastFault = 0xFF; // 0xFF means begin() failed completely
    isOk = false;
  }

  // Timeout sau 2 giây
  if (millis() - startTime > 2000) {
    isOk = false;
  }

  return isOk;
}

float TempSensor::readTemperature() {
  if (!isOk) {
    return -999.0;
  }

  uint16_t rtdVal = rtd->readRTD();
  uint8_t fault = rtd->readFault();

  if (fault == 0 && rtdVal > 0) {
    float temp = rtd->temperature(rnominal, rref);

    // Kiểm tra nhiệt độ hợp lý
    if (temp < -50 || temp > 200) {
      return -999.0;
    }
    return temp;
  } else {
    rtd->clearFault();
    return -999.0;
  }
}

bool TempSensor::isReady() { return isOk; }

uint8_t TempSensor::getFault() { return rtd->readFault(); }

void TempSensor::clearFault() { rtd->clearFault(); }

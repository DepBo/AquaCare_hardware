#ifndef TEMP_SENSOR_H
#define TEMP_SENSOR_H

#include <Arduino.h>
#include <SPI.h>
#include <Adafruit_MAX31865.h>

class TempSensor {
private:
    Adafruit_MAX31865* rtd;
    uint8_t csPin;
    uint8_t diPin;
    uint8_t doPin;
    uint8_t clkPin;
    float rref;
    float rnominal;
    bool isOk;

public:
    uint8_t lastFault;
    // Constructor - SPI pins cho PT100
    TempSensor(uint8_t cs, uint8_t di, uint8_t dout, uint8_t clk, 
               float refResistor = 430.0, float nominalResistor = 100.0);
    
    // Destructor
    ~TempSensor();
    
    // Khởi tạo cảm biến
    bool begin();
    
    // Đọc nhiệt độ (°C)
    float readTemperature();
    
    // Kiểm tra trạng thái
    bool isReady();
    
    // Đọc lỗi
    uint8_t getFault();
    
    // Xóa lỗi
    void clearFault();
};

#endif

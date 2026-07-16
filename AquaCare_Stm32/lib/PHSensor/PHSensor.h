#ifndef PHSENSOR_H
#define PHSENSOR_H

#include <Arduino.h>

class PHSensor {
private:
    uint8_t pin;
    float adcMax;
    float vrefMv;
    float offset;
    
    // Thuật toán lọc nhiễu theo mẫu của nhà sản xuất
    static const int ArrayLength = 40;
    int pHArray[ArrayLength];
    int pHArrayIndex;
    double averageArray(int* arr, int number);

    // Chứa biến tạm nhiệt độ để giữ form giống code cũ
    float temperature;

public:
    // Constructor (giữ nguyên chữ ký để tương thích)
    PHSensor(uint8_t sensorPin, float defaultTemp = 25.0);
    
    // Khởi tạo cảm biến
    void begin();
    
    // Đọc giá trị pH (Thuật toán của cảm biến mới giá 756k)
    float readPH();
    
    // Đọc pH với nhiệt độ cụ thể (tương thích code cũ, không tính bù nhiệt)
    float readPH(float temp);
    
    // Cập nhật nhiệt độ cho bù nhiệt (tương thích code cũ)
    void setTemperature(float temp);
    
    // Hiệu chỉnh pH (tương thích code cũ)
    void calibration(float temp);
    
    // Thiết lập Offset tùy chỉnh
    void setOffset(float newOffset);
};

#endif

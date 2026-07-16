#ifndef WATER_LEVEL_H
#define WATER_LEVEL_H

#include <Arduino.h>

class WaterLevel {
private:
    uint8_t pin;

public:
    WaterLevel(uint8_t sensorPin);
    void begin();
    
    // Trả về true nếu ngập nước, false nếu cạn nước
    bool isWaterPresent(); 
};

#endif
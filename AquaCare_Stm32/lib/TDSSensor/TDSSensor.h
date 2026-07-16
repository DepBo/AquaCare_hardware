#ifndef TDS_SENSOR_H
#define TDS_SENSOR_H

#include <Arduino.h>
#include <EEPROM.h>
#include <GravityTDS.h>

class TDSSensor {
private:
    uint8_t pin;
    GravityTDS gravityTds;
    float temperature;
    float aref;
    int adcRange;

public:
    TDSSensor(uint8_t sensorPin, float vref = 3.3, int adcBits = 4096);
    void begin();
    float readTDS();
    void setTemperature(float temp);
    void calibration(); // Dùng để hiệu chuẩn qua Serial
};

#endif
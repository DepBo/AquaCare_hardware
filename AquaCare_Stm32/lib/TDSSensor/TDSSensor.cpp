#include "TDSSensor.h"

TDSSensor::TDSSensor(uint8_t sensorPin, float vref, int adcBits) {
    pin = sensorPin;
    aref = vref;
    adcRange = adcBits;
    temperature = 25.0; 
}

void TDSSensor::begin() {
    pinMode(pin, INPUT_ANALOG);
    gravityTds.setPin(pin);
    gravityTds.setAref(aref);        
    gravityTds.setAdcRange(adcRange); 
    gravityTds.begin();
}

float TDSSensor::readTDS() {
    gravityTds.setTemperature(temperature);
    gravityTds.update();
    return gravityTds.getTdsValue();
}

void TDSSensor::setTemperature(float temp) {
    temperature = temp;
}
#include "PHSensor.h"

PHSensor::PHSensor(uint8_t sensorPin, float defaultTemp) {
    pin = sensorPin;
    adcMax = 4095.0;     // STM32 12-bit ADC
    
    // -- CẤU HÌNH ĐIỆN ÁP THAM CHIẾU (VREF) --
    vrefMv = 3300.0;     // Mặc định cho STM32 cắm trực tiếp (ADC của STM32 lấy mốc 3.3V)
    // vrefMv = 5000.0;  // Bỏ comment dòng này (và comment dòng 3300.0 ở trên) NẾU:
                         // 1. Bạn mang code này qua chạy trên Arduino Uno (ADC 5V)
                         // 2. HOẶC bạn dùng STM32 nhưng có lắp thêm mạch Cầu Phân Áp (hạ 5V xuống 3.3V) để bảo vệ STM32.
    temperature = defaultTemp;
    offset = 0.00;       // Độ lệch bù trừ (hiệu chuẩn)
    pHArrayIndex = 0;
}

void PHSensor::begin() {
    pinMode(pin, INPUT_ANALOG);
    
    // Lấy trước mẫu để tránh mảng 0 làm sai kết quả khi mới chạy
    int initialVal = analogRead(pin);
    for (int i = 0; i < ArrayLength; i++) {
        pHArray[i] = initialVal;
    }
}

double PHSensor::averageArray(int* arr, int number) {
    int i;
    int max, min;
    double avg;
    long amount = 0;
    
    if (number <= 0) {
        return 0;
    }
    
    if (number < 5) {
        for (i = 0; i < number; i++) {
            amount += arr[i];
        }
        avg = amount / number;
        return avg;
    } else {
        if (arr[0] < arr[1]) {
            min = arr[0]; max = arr[1];
        } else {
            min = arr[1]; max = arr[0];
        }
        for (i = 2; i < number; i++) {
            if (arr[i] < min) {
                amount += min;
                min = arr[i];
            } else {
                if (arr[i] > max) {
                    amount += max;
                    max = arr[i];
                } else {
                    amount += arr[i];
                }
            }
        }
        avg = (double)amount / (number - 2);
    }
    return avg;
}

float PHSensor::readPH() {
    // 1. Lấy dữ liệu Analog mới bỏ vào mảng
    pHArray[pHArrayIndex++] = analogRead(pin);
    if (pHArrayIndex == ArrayLength) {
        pHArrayIndex = 0;
    }
    
    // 2. Lấy trung bình giá trị từ mảng theo thuật toán lọc nhiễu
    float avgADC = averageArray(pHArray, ArrayLength);
    
    // 3. Tính toán Voltage (V). vrefMv = 3300.0 tương đương 3.3V
    float voltage = avgADC * (vrefMv / 1000.0) / adcMax;
    
    // 4. Tính toán pH: pH = 3.5 * V + Offset
    float phValue = 3.5 * voltage + offset;
    
    return phValue;
}

float PHSensor::readPH(float temp) {
    temperature = temp;
    return readPH();
}

void PHSensor::setTemperature(float temp) {
    temperature = temp;
}

void PHSensor::calibration(float temp) {
    // Không dùng với cảm biến mới, nhưng vẫn giữ để main.cpp gọi không bị lỗi biên dịch.
    // Việc hiệu chuẩn sẽ dựa vào hàm setOffset(). 
}

void PHSensor::setOffset(float newOffset) {
    offset = newOffset;
}

#include "WaterLevel.h"

WaterLevel::WaterLevel(uint8_t sensorPin) {
    pin = sensorPin;
}

void WaterLevel::begin() {
    // Không cần INPUT_PULLUP vì mạch phân áp đã kéo xuống Mass rồi
    pinMode(pin, INPUT); 
}

// bool WaterLevel::isWaterPresent() {
//     // Vì mức LOW của mạch đang là 1.27V (cao hơn chuẩn LOW của STM32 là ~0.9V)
//     // Nên digitalRead() luôn hiểu nhầm 1.27V là HIGH (dẫn tới bị kẹt ở 1 trạng thái).
//     // Giải pháp: Đọc bằng analog (0-4095 cho 0-3.3V).
//     // 4.28V sẽ tràn ngưỡng -> đọc ra ~4095
//     // 1.27V -> đọc ra khoảng ~1576
//     // Chọn mốc 2500 làm ranh giới.
    
//     int analogVal = analogRead(pin);
    
//     // analogVal < 2500 tương đương với mức ~1.27V (Có nước)
//     return (analogVal < 2500); 
// }

bool WaterLevel::isWaterPresent() {
    long sum = 0;
    int samples = 10; // Đọc 10 mẫu để lấy trung bình
    
    for(int i = 0; i < samples; i++) {
        sum += analogRead(pin);
        // Không delay() ở đây - tránh block main loop làm trễ relay command
    }
    
    int avg = sum / samples;
    
    // DEBUG: Hãy mở Serial Monitor xem giá trị 'avg' này là bao nhiêu 
    // khi khô và khi ướt.
    // Serial.print("Avg: "); Serial.println(avg);
    
    // NGƯỠNG MỚI: 
    // Giả sử sau khi phân áp: 
    // - Khi khô: bạn đo được ~2.14V (tương đương ~2650 trên thang 4096)
    // - Khi ướt: bạn đo được ~0.63V (tương đương ~780 trên thang 4096)
    // Bạn nên chọn ngưỡng ở giữa, ví dụ: 1700
    
    return (avg < 1700); 
}
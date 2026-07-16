#ifndef TIMER_INTERRUPT_H
#define TIMER_INTERRUPT_H

#include <Arduino.h>

class TimerInterrupt {
private:
    HardwareTimer *timer;
    uint32_t interval;
    void (*callback)();

public:
    TimerInterrupt(TIM_TypeDef *instance);
    ~TimerInterrupt();
    void begin(uint32_t intervalMs, void (*userCallback)());
    void stop();
    void resume();
    void setInterval(uint32_t intervalMs);
    uint32_t getInterval();
};

#endif

#include "TimerInterrupt.h"

TimerInterrupt::TimerInterrupt(TIM_TypeDef *instance) {
    timer = new HardwareTimer(instance);
    interval = 1000;
    callback = nullptr;
}

TimerInterrupt::~TimerInterrupt() {
    if (timer) {
        timer->pause();
        delete timer;
    }
}

void TimerInterrupt::begin(uint32_t intervalMs, void (*userCallback)()) {
    if (!userCallback) return;
    
    interval = intervalMs;
    callback = userCallback;
    
    // Convert ms to us for better precision
    timer->setOverflow(interval * 1000, MICROSEC_FORMAT);
    timer->attachInterrupt(callback);
    timer->resume();
}

void TimerInterrupt::stop() {
    if (timer) {
        timer->pause();
    }
}

void TimerInterrupt::resume() {
    if (timer) {
        timer->resume();
    }
}

void TimerInterrupt::setInterval(uint32_t intervalMs) {
    interval = intervalMs;
    if (timer) {
        timer->setOverflow(interval * 1000, MICROSEC_FORMAT);
    }
}

uint32_t TimerInterrupt::getInterval() {
    return interval;
}

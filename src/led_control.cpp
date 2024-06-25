#include "led_control.h"
#include "logger.h"

static const char *TAG = "LED_CONTROL";

EventDispatcher *LEDControl::eventDispatcher = nullptr;

LEDControl::LEDControl(int pin) : _pin(pin) {}

void LEDControl::begin(EventDispatcher &dispatcher) {
    eventDispatcher = &dispatcher;
    pinMode(_pin, OUTPUT);
    digitalWrite(_pin, LOW);
    LOG_I(TAG, "LED control initialized");
}

void LEDControl::turnOn() {
//    digitalWrite(_pin, HIGH);
    LOG_I(TAG, "LED turned on");
    eventDispatcher->dispatchEvent({LED_TURNED_ON, ""});
}

void LEDControl::turnOff() {
    digitalWrite(_pin, LOW);
    LOG_I(TAG, "LED turned off");
    eventDispatcher->dispatchEvent({LED_TURNED_OFF, ""});
}
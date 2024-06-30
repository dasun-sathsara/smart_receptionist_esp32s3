#include "led_control.h"
#include "logger.h"

static const char *TAG = "LED_CONTROL";

EventDispatcher *LEDControl::eventDispatcher = nullptr;

LEDControl::LEDControl(int pin) : pin(pin) {}

void LEDControl::begin(EventDispatcher &dispatcher) {
    eventDispatcher = &dispatcher;
    pinMode(pin, OUTPUT);
    digitalWrite(pin, LOW);
    LOG_I(TAG, "LED control initialized");
}

void LEDControl::turnOn() {
//    digitalWrite(pin, HIGH);
    LOG_I(TAG, "LED turned on");
    eventDispatcher->dispatchEvent({LED_TURNED_ON, ""});
}

void LEDControl::turnOff() {
    digitalWrite(pin, LOW);
    LOG_I(TAG, "LED turned off");
    eventDispatcher->dispatchEvent({LED_TURNED_OFF, ""});
}
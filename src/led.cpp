#include "led.h"
#include "logger.h"
#include "config.h"

static const char *TAG = "LED";

EventDispatcher *LED::eventDispatcher = nullptr;

LED::LED() : pin(LED_STRIP_PIN) {}

void LED::begin(EventDispatcher &dispatcher) {
    eventDispatcher = &dispatcher;

    pinMode(pin, OUTPUT);
    digitalWrite(pin, HIGH);
    LOG_I(TAG, "LED initialized");
}

void LED::turnOn() {
    digitalWrite(pin, LOW);
    LOG_I(TAG, "LED turned on");
    eventDispatcher->dispatchEvent({LED_TURNED_ON, ""});
}

void LED::turnOff() {
    digitalWrite(pin, HIGH);
    LOG_I(TAG, "LED turned off");
    eventDispatcher->dispatchEvent({LED_TURNED_OFF, ""});
}

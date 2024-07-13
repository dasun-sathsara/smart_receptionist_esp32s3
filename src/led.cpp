#include "led.h"
#include "logger.h"
#include "config.h"

static const char *TAG = "LED";

EventDispatcher *LED::eventDispatcher = nullptr;

LED::LED() : pin(LED_STRIP_PIN) {}

void LED::begin(EventDispatcher &dispatcher) {
    eventDispatcher = &dispatcher;
    pinMode(pin, OUTPUT);
    digitalWrite(pin, HIGH);  // Initialize the relay in OFF state
    LOG_I(TAG, "LED (Relay) initialized");
}

void LED::turnOn() {
    digitalWrite(pin, LOW);  // Trigger the relay LOW to turn ON
    LOG_I(TAG, "LED (Relay) turned on");
    eventDispatcher->dispatchEvent({LED_TURNED_ON, ""});
}

void LED::turnOff() {
    digitalWrite(pin, HIGH);  // Set the relay HIGH to turn OFF
    LOG_I(TAG, "LED (Relay) turned off");
    eventDispatcher->dispatchEvent({LED_TURNED_OFF, ""});
}
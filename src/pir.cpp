#include "pir.h"
#include "logger.h"
#include "config.h"

static const char *TAG_PIR = "PIR_SENSOR";

EventDispatcher *PIRSensor::eventDispatcher = nullptr;

PIRSensor::PIRSensor() : lastDebounceTime(0), lastState(LOW), state(LOW) { pin = PIR_PIN; }

void PIRSensor::begin(EventDispatcher &dispatcher) {
    eventDispatcher = &dispatcher;

    pinMode(pin, INPUT);
    xTaskCreate(pirTask, "PIR Sensor Task", 2048, this, 1, nullptr);
    LOG_I(TAG_PIR, "PIR sensor initialized");
}

void PIRSensor::pirTask(void *parameter) {
    auto *pirSensor = static_cast<PIRSensor *>(parameter);
    const unsigned long debounceDelay = 50; // 50ms debounce time
    const unsigned long cooldownTime = 60000; // 1 minute cooldown
    const unsigned long inactivityThreshold = 300000; // 5 minutes of inactivity
    unsigned long lastTriggerTime = 0;
    unsigned long lastActivityTime = 0;

    while (true) {
        int reading = digitalRead(pirSensor->pin);

        if (reading != pirSensor->lastState) {
            pirSensor->lastDebounceTime = millis();
        }

        if ((millis() - pirSensor->lastDebounceTime) > debounceDelay) {
            if (reading != pirSensor->state) {
                pirSensor->state = reading;

                if (pirSensor->state == HIGH && (millis() - lastTriggerTime) > cooldownTime) {
                    eventDispatcher->dispatchEvent({MOTION_DETECTED, ""});
                    lastTriggerTime = millis();
                }
            }
        }

        // Check for inactivity
        if ((millis() - lastActivityTime) > inactivityThreshold) {
            eventDispatcher->dispatchEvent({INACTIVITY_DETECTED, ""});
            lastActivityTime = millis(); // Reset to avoid continuous events
        }

        pirSensor->lastState = reading;
        vTaskDelay(pdMS_TO_TICKS(10)); // Check every 10ms
    }
}

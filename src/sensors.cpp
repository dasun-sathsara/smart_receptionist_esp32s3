#include "sensors.h"
#include "logger.h"
#include "config.h"

static const char *TAG_PIR = "PIR_SENSOR";
static const char *TAG_BREAK_BEAM = "BREAK_BEAM_SENSOR";

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
    unsigned long lastTriggerTime = 0;

    while (true) {
        int reading = digitalRead(pirSensor->pin);

        if (reading != pirSensor->lastState) {
            pirSensor->lastDebounceTime = millis();
        }

        if ((millis() - pirSensor->lastDebounceTime) > debounceDelay) {
            if (reading != pirSensor->state) {
                pirSensor->state = reading;

                if (pirSensor->state == HIGH && (millis() - lastTriggerTime) > cooldownTime) {
                    LOG_I(TAG_PIR, "Motion detected");
                    eventDispatcher->dispatchEvent({MOTION_DETECTED, ""});
                    lastTriggerTime = millis();
                }
            }
        }

        pirSensor->lastState = reading;
        vTaskDelay(pdMS_TO_TICKS(10)); // Check every 10ms
    }
}

EventDispatcher *BreakBeamSensor::eventDispatcher = nullptr;

BreakBeamSensor::BreakBeamSensor() : lastDebounceTime(0), lastState(HIGH), state(HIGH) { pin = BREAK_BEAM_PIN; }

void BreakBeamSensor::begin(EventDispatcher &dispatcher) {
    eventDispatcher = &dispatcher;

    pinMode(pin, INPUT_PULLUP);
    xTaskCreate(breakBeamTask, "Break Beam Sensor Task", 2048, this, 1, nullptr);
    LOG_I(TAG_BREAK_BEAM, "Break beam sensor initialized");
}

void BreakBeamSensor::breakBeamTask(void *parameter) {
    auto *breakBeamSensor = static_cast<BreakBeamSensor *>(parameter);
    const unsigned long debounceDelay = 50;

    while (true) {
        int reading = digitalRead(breakBeamSensor->pin);

        if (reading != breakBeamSensor->lastState) {
            breakBeamSensor->lastDebounceTime = millis();
        }

        if ((millis() - breakBeamSensor->lastDebounceTime) > debounceDelay) {
            if (reading != breakBeamSensor->state) {
                breakBeamSensor->state = reading;

                if (breakBeamSensor->state == LOW) {
                    LOG_I(TAG_BREAK_BEAM, "Break beam triggered");
                    eventDispatcher->dispatchEvent({VISITOR_ENTERED, ""});
                }
            }
        }

        breakBeamSensor->lastState = reading;
        vTaskDelay(pdMS_TO_TICKS(10)); // Check every 10ms
    }
}
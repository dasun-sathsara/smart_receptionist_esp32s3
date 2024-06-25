#include "sensors.h"
#include "logger.h"

static const char *TAG_PIR = "PIR_SENSOR";
static const char *TAG_BREAK_BEAM = "BREAK_BEAM_SENSOR";

// PIR Sensor Implementation
EventDispatcher *PIRSensor::eventDispatcher = nullptr;

PIRSensor::PIRSensor(int pin) : _pin(pin), _lastDebounceTime(0), _lastState(LOW), _state(LOW) {}

void PIRSensor::begin(EventDispatcher &dispatcher) {
    eventDispatcher = &dispatcher;
    pinMode(_pin, INPUT);
    xTaskCreate(pirTask, "PIR Sensor Task", 2048, this, 1, nullptr);
    LOG_I(TAG_PIR, "PIR sensor initialized");
}

void PIRSensor::pirTask(void *parameter) {
    auto *pirSensor = static_cast<PIRSensor *>(parameter);
    const unsigned long debounceDelay = 50; // 50ms debounce time
    const unsigned long cooldownTime = 60000; // 1 minute cooldown
    unsigned long lastTriggerTime = 0;

    while (true) {
        int reading = digitalRead(pirSensor->_pin);

        if (reading != pirSensor->_lastState) {
            pirSensor->_lastDebounceTime = millis();
        }

        if ((millis() - pirSensor->_lastDebounceTime) > debounceDelay) {
            if (reading != pirSensor->_state) {
                pirSensor->_state = reading;

                if (pirSensor->_state == HIGH && (millis() - lastTriggerTime) > cooldownTime) {
                    LOG_I(TAG_PIR, "Motion detected");
                    eventDispatcher->dispatchEvent({MOTION_DETECTED, ""});
                    lastTriggerTime = millis();
                }
            }
        }

        pirSensor->_lastState = reading;
        vTaskDelay(pdMS_TO_TICKS(10)); // Check every 10ms
    }
}

// Break Beam Sensor Implementation
EventDispatcher *BreakBeamSensor::eventDispatcher = nullptr;

BreakBeamSensor::BreakBeamSensor(int pin) : _pin(pin), _lastDebounceTime(0), _lastState(HIGH), _state(HIGH) {}

void BreakBeamSensor::begin(EventDispatcher &dispatcher) {
    eventDispatcher = &dispatcher;
    pinMode(_pin, INPUT_PULLUP);
    xTaskCreate(breakBeamTask, "Break Beam Sensor Task", 2048, this, 1, nullptr);
    LOG_I(TAG_BREAK_BEAM, "Break beam sensor initialized");
}

void BreakBeamSensor::breakBeamTask(void *parameter) {
    auto *breakBeamSensor = static_cast<BreakBeamSensor *>(parameter);
    const unsigned long debounceDelay = 50; // 50ms debounce time

    while (true) {
        int reading = digitalRead(breakBeamSensor->_pin);

        if (reading != breakBeamSensor->_lastState) {
            breakBeamSensor->_lastDebounceTime = millis();
        }

        if ((millis() - breakBeamSensor->_lastDebounceTime) > debounceDelay) {
            if (reading != breakBeamSensor->_state) {
                breakBeamSensor->_state = reading;

                if (breakBeamSensor->_state == LOW) {
                    LOG_I(TAG_BREAK_BEAM, "Break beam triggered");
                    eventDispatcher->dispatchEvent({BREAK_BEAM_TRIGGERED, ""});
                }
            }
        }

        breakBeamSensor->_lastState = reading;
        vTaskDelay(pdMS_TO_TICKS(10)); // Check every 10ms
    }
}
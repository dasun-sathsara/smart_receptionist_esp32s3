#include "sensors.h"
#include "logger.h"
#include "network_manager.h"

static const char *TAG_PIR = "PIR_SENSOR";
static const char *TAG_BREAK_BEAM = "BREAK_BEAM_SENSOR";

EventDispatcher *PIRSensor::eventDispatcher = nullptr;
volatile bool PIRSensor::cooldownActive = false;

EventDispatcher *BreakBeamSensor::eventDispatcher = nullptr;

PIRSensor::PIRSensor(int pin) : _pin(pin) {}

void PIRSensor::begin(EventDispatcher &dispatcher) {
    eventDispatcher = &dispatcher;
    pinMode(_pin, INPUT);
    xTaskCreate(pirTask, "PIR Sensor Task", 2048, this, 1, nullptr);
    LOG_I(TAG_PIR, "PIR sensor initialized");
}

void PIRSensor::pirTask(void *parameter) {
    auto *pirSensor = static_cast<PIRSensor *>(parameter);
    TickType_t cooldownTime = pdMS_TO_TICKS(60000); // 1 minute cooldown

    while (true) {
        if (digitalRead(pirSensor->_pin) == HIGH && !cooldownActive) {
            LOG_I(TAG_PIR, "Motion detected");
            StaticJsonDocument<1> emptyDoc;
            NetworkManager::sendEvent("motion_detected", emptyDoc.as<JsonObject>());

            cooldownActive = true;
            vTaskDelay(cooldownTime);
            cooldownActive = false;
        }
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

BreakBeamSensor::BreakBeamSensor(int pin) : _pin(pin) {}

void BreakBeamSensor::begin(EventDispatcher &dispatcher) {
    eventDispatcher = &dispatcher;
    pinMode(_pin, INPUT_PULLUP);
    xTaskCreate(breakBeamTask, "Break Beam Sensor Task", 2048, this, 1, nullptr);
    LOG_I(TAG_BREAK_BEAM, "Break beam sensor initialized");
}

void BreakBeamSensor::breakBeamTask(void *parameter) {
    auto *breakBeamSensor = static_cast<BreakBeamSensor *>(parameter);
    bool lastState = digitalRead(breakBeamSensor->_pin);

    while (true) {
        bool currentState = digitalRead(breakBeamSensor->_pin);
        if (currentState != lastState) {
            if (currentState == LOW) {
                LOG_I(TAG_BREAK_BEAM, "Break beam triggered");
                eventDispatcher->dispatchEvent({BREAK_BEAM_TRIGGERED, ""});
            }
            lastState = currentState;
        }
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}
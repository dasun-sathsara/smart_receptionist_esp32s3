#include "pir.h"
#include "logger.h"
#include "config.h"

static const char *TAG_PIR = "PIR_SENSOR";

EventDispatcher *PIRSensor::eventDispatcher = nullptr;
int PIRSensor::pin = PIR_PIN;

PIRSensor::PIRSensor() {}

void PIRSensor::begin(EventDispatcher &dispatcher) {
    eventDispatcher = &dispatcher;
    pinMode(pin, INPUT);
    xTaskCreate(pirTask, "PIR Sensor Task", 8192, this, 1, nullptr);
    LOG_I(TAG_PIR, "PIR sensor initialized");
}

bool PIRSensor::motionDetectionEnabled = false;

void PIRSensor::enableMotionDetection() {
    motionDetectionEnabled = true;
    LOG_I(TAG_PIR, "Motion detection enabled");
}

void PIRSensor::pirTask(void *parameter) {
    auto *pirSensor = static_cast<PIRSensor *>(parameter);
    int lastState = LOW;
    while (true) {
        if (motionDetectionEnabled) {
            int currentState = digitalRead(pirSensor->pin);
            if (currentState != lastState) {
                if (currentState == HIGH) {
                    LOG_I(TAG_PIR, "Motion detected");
                    eventDispatcher->dispatchEvent({MOTION_DETECTED, ""});
                    vTaskDelay(pdMS_TO_TICKS(180000)); // Wait for 3 minutes before checking again
                }
                lastState = currentState;
            }
        }
        vTaskDelay(pdMS_TO_TICKS(10)); // Check every 10ms
    }
}


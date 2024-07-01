#include "gate_control.h"
#include "logger.h"
#include "config.h"

static const char *TAG = "GATE_CONTROL";

EventDispatcher *GateControl::eventDispatcher = nullptr;
volatile bool GateControl::isGateOpen = false;
SemaphoreHandle_t GateControl::gateStateMutex = xSemaphoreCreateMutex();

GateControl::GateControl() {
    motorPin1 = MOTOR_PIN1;
    motorPin2 = MOTOR_PIN2;
    enablePin = MOTOR_ENABLE;
};

void GateControl::begin(EventDispatcher &dispatcher) {
    eventDispatcher = &dispatcher;

    pinMode(motorPin1, OUTPUT);
    pinMode(motorPin2, OUTPUT);
    pinMode(enablePin, OUTPUT);

    xTaskCreate(gateControlTask, "Gate Control Task", 4096, this, 1, nullptr);

    LOG_I(TAG, "Gate control initialized");
}

void GateControl::openGate() {
    if (xSemaphoreTake(gateStateMutex, portMAX_DELAY) == pdTRUE) {
        if (!isGateOpen && !isMoving) {
            digitalWrite(motorPin1, HIGH);
            digitalWrite(motorPin2, LOW);
            digitalWrite(enablePin, HIGH);
            movementStartTime = millis();
            isMoving = true;
            LOG_I(TAG, "Gate opening started...");
        } else {
            LOG_I(TAG, "Gate is already open or in motion.");
        }
        xSemaphoreGive(gateStateMutex);
    }
}

void GateControl::closeGate() {
    if (xSemaphoreTake(gateStateMutex, portMAX_DELAY) == pdTRUE) {
        if (isGateOpen && !isMoving) {
            digitalWrite(motorPin1, LOW);
            digitalWrite(motorPin2, HIGH);
            digitalWrite(enablePin, HIGH);
            movementStartTime = millis();
            isMoving = true;
            LOG_I(TAG, "Gate closing started...");
        } else {
            LOG_I(TAG, "Gate is already closed or in motion.");
        }
        xSemaphoreGive(gateStateMutex);
    }
}

bool GateControl::isGateMoving() const {
    return isMoving;
}


void GateControl::gateControlTask(void *parameter) {
    auto *gateControl = static_cast<GateControl *>(parameter);

    while (true) {
        if (gateControl->isGateMoving() && millis() - gateControl->movementStartTime > 7000) {
            digitalWrite(gateControl->enablePin, LOW);
            gateControl->isMoving = false;

            // Update isGateOpen within the mutex
            if (xSemaphoreTake(gateStateMutex, portMAX_DELAY) == pdTRUE) {
                isGateOpen = !isGateOpen; // Toggle the state
                xSemaphoreGive(gateStateMutex);
            }

            // Dispatch events after updating isGateOpen
            if (isGateOpen) {
                eventDispatcher->dispatchEvent({GATE_OPENED, ""});
            } else {
                eventDispatcher->dispatchEvent({GATE_CLOSED, ""});
            }
        }
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

bool GateControl::getIsGateOpen() {
    return isGateOpen;
}

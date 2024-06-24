#include "gate_control.h"
#include "logger.h"

static const char *TAG = "GATE_CONTROL";

EventDispatcher *GateControl::eventDispatcher = nullptr;
volatile bool GateControl::_isGateOpen = false;
SemaphoreHandle_t GateControl::_gateStateMutex = xSemaphoreCreateMutex(); // Initialize the mutex

GateControl::GateControl(int motorPin1, int motorPin2, int enablePin)
        : _motorPin1(motorPin1), _motorPin2(motorPin2), _enablePin(enablePin) {}

void GateControl::begin(EventDispatcher &dispatcher) {
    eventDispatcher = &dispatcher;
    pinMode(_motorPin1, OUTPUT);
    pinMode(_motorPin2, OUTPUT);
    pinMode(_enablePin, OUTPUT);

    xTaskCreate(gateControlTask, "Gate Control Task", 4096, this, 1, nullptr);

    LOG_I(TAG, "Gate control initialized");
}

void GateControl::openGate() {
    if (xSemaphoreTake(_gateStateMutex, portMAX_DELAY) == pdTRUE) {
        if (!_isGateOpen && !_isMoving) { // Check if gate is closed and not moving
            digitalWrite(_motorPin1, HIGH);
            digitalWrite(_motorPin2, LOW);
            digitalWrite(_enablePin, HIGH);
            _movementStartTime = millis();
            _isMoving = true;
            LOG_I(TAG, "Gate opening started...");
        } else {
            LOG_I(TAG, "Gate is already open or in motion.");
        }
        xSemaphoreGive(_gateStateMutex);
    }
}

void GateControl::closeGate() {
    if (xSemaphoreTake(_gateStateMutex, portMAX_DELAY) == pdTRUE) {
        if (_isGateOpen && !_isMoving) { // Check if gate is open and not moving
            digitalWrite(_motorPin1, LOW);
            digitalWrite(_motorPin2, HIGH);
            digitalWrite(_enablePin, HIGH);
            _movementStartTime = millis();
            _isMoving = true;
            LOG_I(TAG, "Gate closing started...");
        } else {
            LOG_I(TAG, "Gate is already closed or in motion.");
        }
        xSemaphoreGive(_gateStateMutex);
    }
}

bool GateControl::isGateMoving() const {
    return _isMoving;
}

// Make isGateOpen() non-const and protect with mutex
bool GateControl::isGateOpen() {
    if (xSemaphoreTake(_gateStateMutex, portMAX_DELAY) == pdTRUE) {
        bool gateState = _isGateOpen;
        xSemaphoreGive(_gateStateMutex);
        return gateState;
    } else {
        // Handle mutex failure (e.g., return a default value)
        return false;
    }
}

void GateControl::gateControlTask(void *parameter) {
    auto *gateControl = static_cast<GateControl *>(parameter);

    while (true) {
        if (gateControl->isGateMoving() && millis() - gateControl->_movementStartTime > 7000) {
            digitalWrite(gateControl->_enablePin, LOW);
            gateControl->_isMoving = false;

            // Update _isGateOpen within the mutex
            if (xSemaphoreTake(_gateStateMutex, portMAX_DELAY) == pdTRUE) {
                _isGateOpen = !_isGateOpen; // Toggle the state
                xSemaphoreGive(_gateStateMutex);
            }

            // Dispatch events after updating _isGateOpen
            if (_isGateOpen) {
                eventDispatcher->dispatchEvent({GATE_OPENED, ""});
            } else {
                eventDispatcher->dispatchEvent({GATE_CLOSED, ""});
            }
        }
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

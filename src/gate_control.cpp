#include "gate_control.h"
#include "logger.h"

static const char *TAG = "GATE_CONTROL";

EventDispatcher *GateControl::eventDispatcher = nullptr;

GateControl::GateControl(int motorPin1, int motorPin2, int enablePin)
        : _motorPin1(motorPin1), _motorPin2(motorPin2), _enablePin(enablePin) {
}

void GateControl::begin(EventDispatcher &dispatcher) {
    eventDispatcher = &dispatcher;
    pinMode(_motorPin1, OUTPUT);
    pinMode(_motorPin2, OUTPUT);
    pinMode(_enablePin, OUTPUT);

    xTaskCreate(gateControlTask, "Gate Control Task", 4096, this, 1, nullptr);

    LOG_I(TAG, "Gate control initialized");
}

void GateControl::openGate() {
    // Check if the gate is already open or in motion
    if (isGateOpen() || isGateMoving()) {
        LOG_I(TAG, "Gate is already open or in motion.");
        return;
    } else {
        digitalWrite(_motorPin1, HIGH);
        digitalWrite(_motorPin2, LOW);
        digitalWrite(_enablePin, HIGH);
        _movementStartTime = millis();
        _isMoving = true;
        LOG_I(TAG, "Gate opening started...");
    }
}

void GateControl::closeGate() {
    // Check if the gate is already closed or in motion
    if (!isGateOpen() || isGateMoving()) {
        LOG_I(TAG, "Gate is already closed or in motion.");
        return;
    } else {
        digitalWrite(_motorPin1, LOW);
        digitalWrite(_motorPin2, HIGH);
        digitalWrite(_enablePin, HIGH);
        _movementStartTime = millis();
        _isMoving = true;
        LOG_I(TAG, "Gate closing started...");
    }

}

bool GateControl::isGateMoving() const {
    return _isMoving;
}

bool GateControl::isGateOpen() const {
    return _isGateOpen;
}


void GateControl::gateControlTask(void *parameter) {
    auto *gateControl = static_cast<GateControl *>(parameter);

    while (true) {
        if (gateControl->isGateMoving() && millis() - gateControl->_movementStartTime > 7000) {
            // Stop the motor
            digitalWrite(gateControl->_enablePin, LOW);
            gateControl->_isMoving = false;

            if (gateControl->isGateOpen()) {
                // Gate is open
                _isGateOpen = true;
            } else {
                // Gate is closed
                _isGateOpen = false;

            }
        }

        vTaskDelay(pdMS_TO_TICKS(100)); // Check every 100ms
    }
}




#include "gate.h"
#include "config.h"
#include "logger.h"

static const char *TAG = "GATE";

EventDispatcher *Gate::eventDispatcher = nullptr;

Gate::Gate()
        : motorPin1(MOTOR_PIN1),
          motorPin2(MOTOR_PIN2),
          enablePin(MOTOR_ENABLE),
          breakBeamPin(BREAK_BEAM_PIN),
          reedSwitchPin(REED_SWITCH_PIN),
          currentState(G_CLOSED),
          stateStartTime(0),
          personEntered(false) {}

void Gate::begin(EventDispatcher &dispatcher) {
    eventDispatcher = &dispatcher;
    pinMode(motorPin1, OUTPUT);
    pinMode(motorPin2, OUTPUT);
    pinMode(enablePin, OUTPUT);
    pinMode(breakBeamPin, INPUT_PULLUP);
    pinMode(reedSwitchPin, INPUT_PULLUP);

    ledcSetup(GATE_PWM_CHANNEL, GATE_PWM_FREQ, GATE_PWM_RESOLUTION);
    ledcAttachPin(enablePin, GATE_PWM_CHANNEL);

    xTaskCreate(gateTask, "Gate Task", 4096, this, 1, nullptr);
    LOG_I(TAG, "Gate control system initialized");
}

void Gate::gateTask(void *parameter) {
    auto *gate = static_cast<Gate *>(parameter);
    while (true) {
        switch (gate->currentState) {
            case G_CLOSED:
                // Gate is closed, waiting for open command
                break;
            case G_OPENING:
                if (millis() - gate->stateStartTime >= GATE_OPERATION_TIME) {
                    gate->stopGate();
                    gate->currentState = G_OPEN;
                    gate->stateStartTime = millis();
                    gate->personEntered = false;
                    eventDispatcher->dispatchEvent({GATE_OPENED, ""});
                }
                break;
            case G_OPEN:
                if (gate->personEntered && (millis() - gate->stateStartTime >= CLOSE_DELAY)) {
                    gate->closeGate();
                }
                break;
            case G_CLOSING:
                if (digitalRead(gate->breakBeamPin) == LOW) {
                    gate->stopGate();
                    while (digitalRead(gate->breakBeamPin) == LOW) {
                        vTaskDelay(pdMS_TO_TICKS(100));
                    }
                    gate->resumeClosing();
                }

                if (digitalRead(gate->reedSwitchPin) == LOW) {
                    gate->gateFullyClosed();
                }
                break;
        }

        if (gate->currentState == G_OPEN && !gate->personEntered) {
            if (digitalRead(gate->breakBeamPin) == LOW) {
                gate->personEntered = true;
                gate->stateStartTime = millis();
                LOG_I(TAG, "Person entered, gate will close in 3 seconds");
                eventDispatcher->dispatchEvent({VISITOR_ENTERED, ""});
            }
        }
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

void Gate::openGate() {
    if (currentState == G_CLOSED) {
        LOG_I(TAG, "Opening gate");
        digitalWrite(motorPin1, HIGH);
        digitalWrite(motorPin2, LOW);
        ledcWrite(GATE_PWM_CHANNEL, GATE_DUTY_CYCLE);
        currentState = G_OPENING;
        stateStartTime = millis();
        personEntered = false;
    }
}

void Gate::closeGate() {
    if (currentState == G_OPEN) {
        LOG_I(TAG, "Closing gate");
        digitalWrite(motorPin1, LOW);
        digitalWrite(motorPin2, HIGH);
        ledcWrite(GATE_PWM_CHANNEL, GATE_DUTY_CYCLE);
        currentState = G_CLOSING;
    }
}

void Gate::stopGate() const {
    LOG_I(TAG, "Stopping gate");
    digitalWrite(motorPin1, LOW);
    digitalWrite(motorPin2, LOW);
    ledcWrite(GATE_PWM_CHANNEL, 0);
}

void Gate::resumeClosing() const {
    LOG_I(TAG, "Resuming gate closure");
    digitalWrite(motorPin1, LOW);
    digitalWrite(motorPin2, HIGH);
    ledcWrite(GATE_PWM_CHANNEL, GATE_DUTY_CYCLE);
}

void Gate::gateFullyClosed() {
    vTaskDelay(pdMS_TO_TICKS(100)); // Wait for the gate to fully close
    stopGate();
    currentState = G_CLOSED;
    eventDispatcher->dispatchEvent({GATE_CLOSED, ""});
    LOG_I(TAG, "Gate fully closed");
}
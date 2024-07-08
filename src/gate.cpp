#include "gate.h"
#include "config.h"
#include "logger.h"

static const char *TAG = "GATE";
EventDispatcher *Gate::eventDispatcher = nullptr;

// Constructor initializes all member variables
Gate::Gate()
        : motorPin1(MOTOR_PIN1),
          motorPin2(MOTOR_PIN2),
          enablePin(MOTOR_ENABLE),
          breakBeamPin(BREAK_BEAM_PIN),
          reedSwitchPin(REED_SWITCH_PIN),
          currentState(G_CLOSED),
          stateStartTime(0),
          lastBreakBeamDebounceTime(0),
          lastReedSwitchDebounceTime(0),
          lastBreakBeamState(HIGH),
          lastReedSwitchState(HIGH),
          personEntered(false) {}

// Initialize gate control system
void Gate::begin(EventDispatcher &dispatcher) {
    eventDispatcher = &dispatcher;
    // Set up GPIO pins
    pinMode(motorPin1, OUTPUT);
    pinMode(motorPin2, OUTPUT);
    pinMode(enablePin, OUTPUT);
    pinMode(breakBeamPin, INPUT_PULLUP);
    pinMode(reedSwitchPin, INPUT_PULLUP);
    // Configure PWM for motor control
    ledcSetup(GATE_PWM_CHANNEL, GATE_PWM_FREQ, GATE_PWM_RESOLUTION);
    ledcAttachPin(enablePin, GATE_PWM_CHANNEL);
    // Create gate control task
    xTaskCreate(gateTask, "Gate Task", 4096, this, 1, nullptr);
    LOG_I(TAG, "Gate control system initialized");
}

// Main gate control loop
void Gate::gateTask(void *parameter) {
    auto *gate = static_cast<Gate *>(parameter);
    while (true) {
        switch (gate->currentState) {
            case G_CLOSED:
                // Gate is closed, waiting for open command
                break;
            case G_OPENING:
                // Check if gate has been opening for the full operation time
                if (millis() - gate->stateStartTime >= GATE_OPERATION_TIME) {
                    gate->stopGate();
                    gate->currentState = G_OPEN;
                    gate->stateStartTime = millis();
                    gate->personEntered = false;
                    eventDispatcher->dispatchEvent({GATE_OPENED, ""});
                }
                break;
            case G_OPEN:
                // If someone has entered and delay time has passed, close the gate
                if (gate->personEntered && (millis() - gate->stateStartTime >= CLOSE_DELAY)) {
                    gate->closeGate();
                }
                break;
            case G_CLOSING:
                // Handle break beam sensor (obstacle detection)
                int breakBeamReading = digitalRead(gate->breakBeamPin);
                if (breakBeamReading != gate->lastBreakBeamState) {
                    gate->lastBreakBeamDebounceTime = millis();
                }
                if ((millis() - gate->lastBreakBeamDebounceTime) > DEBOUNCE_DELAY) {
                    if (breakBeamReading != gate->lastBreakBeamState) {
                        gate->lastBreakBeamState = breakBeamReading;
                        if (gate->lastBreakBeamState == LOW) {
                            // Obstacle detected, stop gate and wait
                            gate->stopGate();
                            while (digitalRead(gate->breakBeamPin) == LOW) {
                                vTaskDelay(pdMS_TO_TICKS(100));
                            }
                            gate->resumeClosing();
                        }
                    }
                }
                // Handle reed switch (gate fully closed detection)
                int reedSwitchReading = digitalRead(gate->reedSwitchPin);
                if (reedSwitchReading != gate->lastReedSwitchState) {
                    gate->lastReedSwitchDebounceTime = millis();
                }
                if ((millis() - gate->lastReedSwitchDebounceTime) > DEBOUNCE_DELAY) {
                    if (reedSwitchReading != gate->lastReedSwitchState) {
                        gate->lastReedSwitchState = reedSwitchReading;
                        if (gate->lastReedSwitchState == LOW) {
                            gate->gateFullyClosed();
                        }
                    }
                }
                break;
        }

        // Detect person entering when gate is open
        if (gate->currentState == G_OPEN && !gate->personEntered) {
            int breakBeamReading = digitalRead(gate->breakBeamPin);
            if (breakBeamReading != gate->lastBreakBeamState) {
                gate->lastBreakBeamDebounceTime = millis();
            }
            if ((millis() - gate->lastBreakBeamDebounceTime) > DEBOUNCE_DELAY) {
                if (breakBeamReading != gate->lastBreakBeamState) {
                    gate->lastBreakBeamState = breakBeamReading;
                    if (gate->lastBreakBeamState == LOW) {
                        gate->personEntered = true;
                        gate->stateStartTime = millis(); // Reset timer for closing delay
                        LOG_I(TAG, "Person entered, gate will close in 3 seconds");

                        eventDispatcher->dispatchEvent({VISITOR_ENTERED, ""});
                    }
                }
            }
        }

        vTaskDelay(pdMS_TO_TICKS(10)); // Prevent task from hogging CPU
    }
}

// Start opening the gate
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

// Start closing the gate
void Gate::closeGate() {
    if (currentState == G_OPEN) {
        LOG_I(TAG, "Closing gate");
        digitalWrite(motorPin1, LOW);
        digitalWrite(motorPin2, HIGH);
        ledcWrite(GATE_PWM_CHANNEL, GATE_DUTY_CYCLE);
        currentState = G_CLOSING;
    }
}

// Stop gate movement
void Gate::stopGate() const {
    LOG_I(TAG, "Stopping gate");
    digitalWrite(motorPin1, LOW);
    digitalWrite(motorPin2, LOW);
    ledcWrite(GATE_PWM_CHANNEL, 0);
}

// Resume closing the gate after obstacle is cleared
void Gate::resumeClosing() const {
    LOG_I(TAG, "Resuming gate closure");
    digitalWrite(motorPin1, LOW);
    digitalWrite(motorPin2, HIGH);
    ledcWrite(GATE_PWM_CHANNEL, GATE_DUTY_CYCLE);
}

// Handle gate fully closed state
void Gate::gateFullyClosed() {
    stopGate();
    currentState = G_CLOSED;
    eventDispatcher->dispatchEvent({GATE_CLOSED, ""});
    LOG_I(TAG, "Gate fully closed");
}
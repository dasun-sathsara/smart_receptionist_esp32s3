#ifndef GATE_CONTROL_H
#define GATE_CONTROL_H

#include <Arduino.h>
#include "events.h"

class GateControl {
public:
    GateControl();

    void begin(EventDispatcher &dispatcher);

    void openGate();

    void closeGate();

    bool isGateMoving() const;

    static bool getIsGateOpen();

private:
    static void gateControlTask(void *parameter);

    int motorPin1{};
    int motorPin2{};
    int enablePin{};

    unsigned long movementStartTime = 0;
    bool isMoving = false;
    static volatile bool isGateOpen;

    static EventDispatcher *eventDispatcher;

    // Use a mutex to protect isGateOpen
    static SemaphoreHandle_t gateStateMutex;
};

#endif // GATE_CONTROL_H

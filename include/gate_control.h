#ifndef GATE_CONTROL_H
#define GATE_CONTROL_H

#include <Arduino.h>
#include "events.h"

class GateControl {
public:
    GateControl(int motorPin1, int motorPin2, int enablePin);

    void begin(EventDispatcher &dispatcher);

    void openGate();

    void closeGate();

    bool isGateMoving() const;

    bool isGateOpen() const;


private:
    static void gateControlTask(void *parameter);


    int _motorPin1;
    int _motorPin2;
    int _enablePin;

    unsigned long _movementStartTime = 0;
    bool _isMoving = false;
    static volatile bool _isGateOpen;

    static EventDispatcher *eventDispatcher;


};

#endif // GATE_CONTROL_H

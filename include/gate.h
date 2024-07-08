#ifndef GATE_H
#define GATE_H

#include <Arduino.h>
#include "events.h"

enum GateState {
    G_CLOSED,
    G_OPENING,
    G_OPEN,
    G_CLOSING
};

class Gate {
public:
    Gate();

    void begin(EventDispatcher &dispatcher);

    void openGate();

    void closeGate();

private:
    static void gateTask(void *parameter);

    void stopGate() const;

    void resumeClosing() const;

    void gateFullyClosed();

    int motorPin1;
    int motorPin2;
    int enablePin;
    int breakBeamPin;
    int reedSwitchPin;
    GateState currentState;
    unsigned long stateStartTime;

    bool personEntered;

    // Debounce variables
    unsigned long lastBreakBeamDebounceTime;
    unsigned long lastReedSwitchDebounceTime;
    int lastBreakBeamState;
    int lastReedSwitchState;

    static EventDispatcher *eventDispatcher;
    static const unsigned long DEBOUNCE_DELAY = 50; // 50ms debounce time
    static const unsigned long CLOSE_DELAY = 3000; // 3 seconds delay before closing
};

#endif // GATE_H
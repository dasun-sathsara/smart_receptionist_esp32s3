#ifndef SENSORS_H
#define SENSORS_H

#include <Arduino.h>
#include "events.h"

class PIRSensor {
public:
    PIRSensor();

    void begin(EventDispatcher &dispatcher);

private:
    static void pirTask(void *parameter);

    int pin;
    int lastState;
    int state;
    static EventDispatcher *eventDispatcher;
    unsigned long lastDebounceTime;
};

class BreakBeamSensor {
public:
    BreakBeamSensor();

    void begin(EventDispatcher &dispatcher);

private:
    static void breakBeamTask(void *parameter);

    int pin;
    unsigned long lastDebounceTime;
    int lastState;
    int state;
    static EventDispatcher *eventDispatcher;
};

#endif // SENSORS_H
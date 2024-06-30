#ifndef SENSORS_H
#define SENSORS_H

#include <Arduino.h>
#include "events.h"

class PIRSensor {
public:
    explicit PIRSensor(int pin);
    void begin(EventDispatcher &dispatcher);

private:
    static void pirTask(void *parameter);
    int pin;
    unsigned long lastDebounceTime;
    int lastState;
    int state;
    static EventDispatcher *eventDispatcher;
};

class BreakBeamSensor {
public:
    explicit BreakBeamSensor(int pin);
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
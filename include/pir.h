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
#endif // SENSORS_H
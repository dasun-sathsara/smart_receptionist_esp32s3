#ifndef SENSORS_H
#define SENSORS_H

#include <Arduino.h>
#include "events.h"

class PIRSensor {
public:
    PIRSensor();

    void begin(EventDispatcher &dispatcher);

    void enableMotionDetection();

private:
    static void pirTask(void *parameter);

    static int pin;
    static EventDispatcher *eventDispatcher;
    static bool motionDetectionEnabled;
};

#endif // SENSORS_H
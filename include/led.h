#ifndef LED_H
#define LED_H

#include <Arduino.h>
#include "events.h"

class LED {
public:
    LED();

    void begin(EventDispatcher &dispatcher);

    void turnOn();

    void turnOff();

private:
    int pin;
    static EventDispatcher *eventDispatcher;
};

#endif // LED_H
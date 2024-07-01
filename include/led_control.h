#ifndef LED_CONTROL_H
#define LED_CONTROL_H

#include <Arduino.h>
#include "events.h"

class LEDControl {
public:
    LEDControl();

    void begin(EventDispatcher &dispatcher);

    void turnOn();

    void turnOff();

private:
    int pin;
    static EventDispatcher *eventDispatcher;
};

#endif // LED_CONTROL_H
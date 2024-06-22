#ifndef LED_CONTROL_H
#define LED_CONTROL_H

#include <Arduino.h>
#include "events.h"

class LEDControl {
public:
    explicit LEDControl(int pin);

    void begin(EventDispatcher &dispatcher);

    void turnOn();

    void turnOff();

private:
    int _pin;
    static EventDispatcher *eventDispatcher;
};

#endif // LED_CONTROL_H
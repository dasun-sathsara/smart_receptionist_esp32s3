#ifndef FINGERPRINT_H
#define FINGERPRINT_H

#include "Adafruit_Fingerprint.h"
#include "events.h"

class FingerprintHandler {
public:
    explicit FingerprintHandler(const HardwareSerial &serial);

    void begin(EventDispatcher &dispatcher);

    void enableSensor();

    void disableSensor();

    void startEnrollment(uint8_t id);

private:
    static void fingerprintTask(void *parameter);

    uint8_t getFingerprintEnroll();


    HardwareSerial mySerial;
    bool isEnrolling = false;
    uint8_t enrollId = 0;
    bool sensorEnabled;
    Adafruit_Fingerprint fingerprint;
    static EventDispatcher *eventDispatcher;
};

#endif // FINGERPRINT_H
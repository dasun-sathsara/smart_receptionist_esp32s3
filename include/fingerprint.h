#ifndef FINGERPRINT_H
#define FINGERPRINT_H

#include "Adafruit_Fingerprint.h"
#include "events.h"

class FingerprintHandler {
public:
    explicit FingerprintHandler(const HardwareSerial &serial);

    void begin(EventDispatcher &dispatcher);

private:
    static void fingerprintTask(void *parameter);

    HardwareSerial mySerial;
    Adafruit_Fingerprint fingerprint;
    static EventDispatcher *eventDispatcher;
};

#endif // FINGERPRINT_H
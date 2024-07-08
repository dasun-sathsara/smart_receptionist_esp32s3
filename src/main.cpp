#include <Arduino.h>
#include "events.h"
#include "audio.h"
#include "network_manager.h"
#include "ui.h"
#include "fingerprint.h"
#include "logger.h"
#include "esp_now_manager.h"
#include "gate.h"
#include "pir.h"
#include "led.h"
#include "config.h"
#include "event_handler.h"


static const char *TAG = "MAIN";

EventDispatcher eventDispatcher;
NetworkManager wifiHandler;
UI ui;
HardwareSerial fingerprintSerial(1);
FingerprintHandler fingerprintHandler(fingerprintSerial);
PIRSensor pirSensor;
Gate gate;
LED led;
Audio audio;
ESPNow espNow;
EventHandler eventHandler(audio, wifiHandler, gate, led, ui, espNow);

void setup() {
    eventHandler.registerCallbacks(eventDispatcher);
    Serial.begin(115200);

    wifiHandler.begin(eventDispatcher);
    ui.begin(eventDispatcher);
    audio.begin(eventDispatcher);
    fingerprintHandler.begin(eventDispatcher);
    pirSensor.begin(eventDispatcher);
    led.begin(eventDispatcher);
    espNow.begin(eventDispatcher);

    LOG_I(TAG, "System initialization complete");
}

void loop() {
}

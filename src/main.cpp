#include <Arduino.h>
#include "events.h"
#include "audio.h"
#include "network_manager.h"
#include "ui.h"
#include "fingerprint.h"
#include "logger.h"
#include "esp_now_manager.h"
#include "gate_control.h"
#include "sensors.h"
#include "led_control.h"
#include "config.h"
#include "event_handler.h"


static const char *TAG = "MAIN";

EventDispatcher eventDispatcher;
NetworkManager wifiHandler;
UI ui;
HardwareSerial fingerprintSerial(1);
FingerprintHandler fingerprintHandler(fingerprintSerial);
GateControl gateControl;
PIRSensor pirSensor;
BreakBeamSensor breakBeamSensor;
LEDControl ledControl;
Audio audio;
ESPNow espNow;
EventHandler eventHandler(audio, wifiHandler, gateControl, ledControl, ui, espNow);

void setup() {
    eventHandler.registerCallbacks(eventDispatcher);
    Serial.begin(115200);

    wifiHandler.begin(eventDispatcher);
    ui.begin(eventDispatcher);
    audio.begin(eventDispatcher);
    fingerprintHandler.begin(eventDispatcher);
    gateControl.begin(eventDispatcher);
    pirSensor.begin(eventDispatcher);
    breakBeamSensor.begin(eventDispatcher);
    ledControl.begin(eventDispatcher);
    espNow.begin(eventDispatcher);

    LOG_I(TAG, "System initialization complete");
}

void loop() {
}

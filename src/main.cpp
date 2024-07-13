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
#include "soc/soc.h"
#include "soc/rtc_cntl_reg.h"

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
EventHandler eventHandler(audio, wifiHandler, gate, led, ui, espNow, fingerprintHandler);

void setup() {
    Serial.begin(115200);

    // Disable brownout detector
    WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0);
    LOG_I(TAG, "Brownout detector disabled");

    // Check PSRAM
    if (psramFound()) {
        size_t psramSize = ESP.getPsramSize();
        LOG_I(TAG, "PSRAM is available. Capacity: %d bytes", psramSize);
    } else {
        LOG_E(TAG, "PSRAM is not available or not working!");
    }

    eventHandler.registerCallbacks(eventDispatcher);

    wifiHandler.begin(eventDispatcher);
    gate.begin(eventDispatcher);
    ui.begin(eventDispatcher);
    audio.begin(eventDispatcher);
    fingerprintHandler.begin(eventDispatcher);
    pirSensor.begin(eventDispatcher);
    led.begin(eventDispatcher);
    espNow.begin(eventDispatcher);

    LOG_I(TAG, "System initialization complete");
}

void loop() {
    // The main loop is empty because tasks are handled by FreeRTOS
}
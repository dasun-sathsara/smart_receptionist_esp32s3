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

#define LOG_BUFFER_SIZE 128

// Define currentLogLevel
LogLevel currentLogLevel = LOG_DEBUG;

// Define the logger function
void logger(LogLevel level, const char *tag, const char *format, ...) {
    if (level > currentLogLevel) return; // Skip if below current log level

    char message[LOG_BUFFER_SIZE];
    va_list args;
    va_start(args, format);
    vsnprintf(message, LOG_BUFFER_SIZE, format, args);
    va_end(args);

    // Formatted output
    Serial.printf("[%s][%s]: %s\n",
                  level == LOG_ERROR ? "ERROR" : (
                          level == LOG_WARN ? "WARN" : (
                                  level == LOG_INFO ? "INFO" : (
                                          level == LOG_DEBUG ? "DEBUG" : "???"))),
                  tag, message);
}

static const char *TAG = "MAIN";

void checkPSRAM() {
    if (psramFound()) {
        size_t psramSize = ESP.getPsramSize();
        size_t freePsram = ESP.getFreePsram();

        LOG_I(TAG, "PSRAM is available");
        LOG_I(TAG, "Total PSRAM: %d bytes", psramSize);
        LOG_I(TAG, "Free PSRAM: %d bytes", freePsram);
    } else {
        LOG_E(TAG, "PSRAM is not available or not initialized");
    }
}

EventDispatcher eventDispatcher;
NetworkManager wifiHandler;
UI ui;
HardwareSerial fingerprintSerial(1);
FingerprintHandler fingerprintHandler(fingerprintSerial);
GateControl gateControl(MOTOR_PIN1, MOTOR_PIN2, MOTOR_ENABLE);
PIRSensor pirSensor(PIR_PIN);
BreakBeamSensor breakBeamSensor(BREAK_BEAM_PIN);
LEDControl ledControl(LED_STRIP_PIN);
Audio audio;
EventHandler eventHandler(audio, wifiHandler, gateControl, ledControl);

void setup() {
    Serial.begin(115200);
    wifiHandler.begin(eventDispatcher);
    ui.begin(eventDispatcher);
    audio.begin();
    fingerprintHandler.begin(eventDispatcher);
    gateControl.begin(eventDispatcher);
    pirSensor.begin(eventDispatcher);
    breakBeamSensor.begin(eventDispatcher);
    ledControl.begin(eventDispatcher);
    checkPSRAM();
    ESPNow::begin(eventDispatcher);

    eventHandler.registerCallbacks(eventDispatcher);

    LOG_I("MAIN", "System initialization complete");
}

void loop() {
}

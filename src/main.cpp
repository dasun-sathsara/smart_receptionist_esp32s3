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

// Callback function prototypes
void handleRecordStart(const Event &event);

void handleRecordStop(const Event &event);

void handlePlaybackStart(const Event &event);

void handlePlaybackStop(const Event &event);

void handleWebSocketConnected(const Event &event);

void handleAudioDataReceived(const Event &event);

void handleAudioChunkRead(const Event &event);

void handleFingerprintMatch(const Event &event);

void handleFingerprintNoMatch(const Event &event);

void handleSendCaptureImageCommand(const Event &event);

void handleChangeState(const Event &event);

void setup() {
    Serial.begin(115200);

    wifiHandler.begin(eventDispatcher);
    ui.begin(eventDispatcher);
    Audio::begin();
    fingerprintHandler.begin(eventDispatcher);
    gateControl.begin(eventDispatcher);
    pirSensor.begin(eventDispatcher);
    breakBeamSensor.begin(eventDispatcher);
    ledControl.begin(eventDispatcher);

    checkPSRAM();

    ESPNow::begin(eventDispatcher);

    // Register callback functions with event dispatcher
    eventDispatcher.registerCallback(RECORD_START, &handleRecordStart);
    eventDispatcher.registerCallback(RECORD_STOP, &handleRecordStop);
    eventDispatcher.registerCallback(PLAYBACK_START, &handlePlaybackStart);
    eventDispatcher.registerCallback(PLAYBACK_STOP, &handlePlaybackStop);
    eventDispatcher.registerCallback(WEBSOCKET_CONNECTED, &handleWebSocketConnected);
    eventDispatcher.registerCallback(AUDIO_DATA_RECEIVED, &handleAudioDataReceived);
    eventDispatcher.registerCallback(AUDIO_CHUNK_READ, &handleAudioChunkRead);
    eventDispatcher.registerCallback(FINGERPRINT_MATCH, &handleFingerprintMatch);
    eventDispatcher.registerCallback(FINGERPRINT_NO_MATCH, &handleFingerprintNoMatch);
    eventDispatcher.registerCallback(SEND_CAPTURE_IMAGE_COMMAND, &handleSendCaptureImageCommand);
    eventDispatcher.registerCallback(CHANGE_STATE, &handleChangeState);

    LOG_I(TAG, "System initialization complete");
}

void loop() {
    // No need to implement loop as FreeRTOS tasks are handling the logic
}

void handleRecordStart(const Event &event) {
    Audio::startRecording();
    LOG_I(TAG, "Recording started");
}

void handleRecordStop(const Event &event) {
    Audio::stopRecording();
    LOG_I(TAG, "Recording stopped");
}

void handlePlaybackStart(const Event &event) {
    Audio::startPlayback();
    LOG_I(TAG, "Playback started");
}

void handlePlaybackStop(const Event &event) {
    Audio::stopPlayback();
    LOG_I(TAG, "Playback stopped");
}

void handleWebSocketConnected(const Event &event) {
    NetworkManager::sendInitMessage();
    LOG_I(TAG, "WebSocket connected, init message sent");
}

void handleAudioDataReceived(const Event &event) {
    Audio::addDataToBuffer(reinterpret_cast<const uint8_t *>(event.data.data()), event.dataLength);
    LOG_D(TAG, "Audio data received and added to buffer");
}

void handleAudioChunkRead(const Event &event) {
    NetworkManager::sendAudioChunk(reinterpret_cast<const uint8_t *>(event.data.data()), event.dataLength);
    LOG_D(TAG, "Audio chunk read and sent");
}

void handleFingerprintMatch(const Event &event) {
    LOG_I(TAG, "Fingerprint match found!");
//    gateControl.openGate();
//    ledControl.turnOn();
}

void handleFingerprintNoMatch(const Event &event) {
    LOG_I(TAG, "Fingerprint no match found!");
//    ledControl.turnOff();
}

void handleSendCaptureImageCommand(const Event &event) {
    ESPNow::sendCommand("capture_image");
    LOG_I(TAG, "Capture image command sent");
}

void handleChangeState(const Event &event) {
    if (event.data == "OPEN") {
        gateControl.openGate();
        ledControl.turnOn();
        LOG_I(TAG, "Gate opened and LED turned on");
    } else if (event.data == "CLOSE") {
        gateControl.closeGate();
        ledControl.turnOff();
        LOG_I(TAG, "Gate closed and LED turned off");
    }
}
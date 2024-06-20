#include <Arduino.h>
#include "events.h"
#include "audio.h"
#include "network_manager.h"
#include "ui.h"
#include "fingerprint.h"
#include "logger.h"

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

EventDispatcher eventDispatcher;
NetworkManager wifiHandler;
//UI ui;
FingerprintHandler fingerprintHandler;

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

void setup() {
    Serial.begin(115200);

    delay(2000);

    Audio::begin();
    wifiHandler.begin(eventDispatcher);
//    ui.begin(eventDispatcher);
    fingerprintHandler.begin(eventDispatcher);

    // Register callback functions with event dispatcher
    eventDispatcher.registerCallback(EVENT_RECORD_START, &handleRecordStart);
    eventDispatcher.registerCallback(EVENT_RECORD_STOP, &handleRecordStop);
    eventDispatcher.registerCallback(EVENT_PLAYBACK_START, &handlePlaybackStart);
    eventDispatcher.registerCallback(EVENT_PLAYBACK_STOP, &handlePlaybackStop);
    eventDispatcher.registerCallback(EVENT_WEBSOCKET_CONNECTED, &handleWebSocketConnected);
    eventDispatcher.registerCallback(EVENT_AUDIO_DATA_RECEIVED, &handleAudioDataReceived);
    eventDispatcher.registerCallback(EVENT_AUDIO_CHUNK_READ, &handleAudioChunkRead);
    eventDispatcher.registerCallback(EVENT_FINGERPRINT_MATCH, &handleFingerprintMatch);
    eventDispatcher.registerCallback(EVENT_FINGERPRINT_NO_MATCH, &handleFingerprintNoMatch);
}

void loop() {
    // No need to implement loop as FreeRTOS tasks are handling the logic
}


void handleRecordStart(const Event &event) {
    Audio::startRecording();
}

void handleRecordStop(const Event &event) {
    Audio::stopRecording();
}

void handlePlaybackStart(const Event &event) {
    Audio::startPlayback();
}

void handlePlaybackStop(const Event &event) {
    Audio::stopPlayback();
}

void handleWebSocketConnected(const Event &event) {
    NetworkManager::sendInitMessage();
}

void handleAudioDataReceived(const Event &event) {
    Audio::addDataToBuffer(reinterpret_cast<const uint8_t *>(event.data.data()), event.dataLength);
}

void handleAudioChunkRead(const Event &event) {
    NetworkManager::sendAudioChunk(reinterpret_cast<const uint8_t *>(event.data.data()), event.dataLength);
}

void handleFingerprintMatch(const Event &event) {
    LOG_I(TAG, "Fingerprint match found!");
    // TODO: Implement
}

void handleFingerprintNoMatch(const Event &event) {
    LOG_I(TAG, "Fingerprint no match found!");
    // TODO: Implement
}

#include "event_handler.h"
#include "logger.h"
#include "esp_now_manager.h"
#include <ArduinoJson.h>

static const char *TAG = "EventHandler";

EventHandler::EventHandler(Audio &audio, NetworkManager &network, GateControl &gate, LEDControl &led, UI &ui, ESPNow &espNow)
        : audio(audio), network(network), gate(gate), led(led), ui(ui), espNow(espNow) {}

void EventHandler::registerCallbacks(EventDispatcher &dispatcher) {
    dispatcher.registerCallback(CMD_RECORD_START, [this](const Event &e) { handleRecordStart(e); });
    dispatcher.registerCallback(CMD_RECORD_STOP, [this](const Event &e) { handleRecordStop(e); });
    dispatcher.registerCallback(CMD_PLAYBACK_START, [this](const Event &e) { handlePlaybackStart(e); });
    dispatcher.registerCallback(CMD_PLAYBACK_STOP, [this](const Event &e) { handlePlaybackStop(e); });
    dispatcher.registerCallback(WS_CONNECTED, [this](const Event &e) { handleWebSocketConnected(e); });
    dispatcher.registerCallback(AUDIO_DATA_RECEIVED, [this](const Event &e) { handleAudioDataReceived(e); });
    dispatcher.registerCallback(AUDIO_CHUNK_READ, [this](const Event &e) { handleAudioChunkRead(e); });
    dispatcher.registerCallback(FINGERPRINT_NO_MATCH, [this](const Event &e) { handleFingerprintNoMatch(e); });
    dispatcher.registerCallback(CMD_CHANGE_STATE, [this](const Event &e) { handleChangeState(e); });
    dispatcher.registerCallback(GATE_OPENED, [this](const Event &e) { handleChangeStateSuccess(e); });
    dispatcher.registerCallback(GATE_CLOSED, [this](const Event &e) { handleChangeStateSuccess(e); });
    dispatcher.registerCallback(LED_TURNED_ON, [this](const Event &e) { handleChangeStateSuccess(e); });
    dispatcher.registerCallback(LED_TURNED_OFF, [this](const Event &e) { handleChangeStateSuccess(e); });
    dispatcher.registerCallback(FINGERPRINT_MATCHED, [this](const Event &e) { handleResidentAuthorized(e); });
    dispatcher.registerCallback(PASSWORD_VALIDATED, [this](const Event &e) { handleResidentAuthorized(e); });
    dispatcher.registerCallback(CMD_GRANT_ACCESS, [this](const Event &e) { handleGrantAccess(e); });
    dispatcher.registerCallback(CMD_DENY_ACCESS, [this](const Event &e) { handleDenyAccess(e); });
    dispatcher.registerCallback(MOTION_DETECTED, [this](const Event &e) { handleMotionDetected(e); });
}


void EventHandler::handleRecordStart(const Event &event) {
    audio.startRecording();
    LOG_I(TAG, "Recording started");
}

void EventHandler::handleRecordStop(const Event &event) {
    audio.stopRecording();
    LOG_I(TAG, "Recording stopped");
}

void EventHandler::handlePlaybackStart(const Event &event) {
    audio.startPlayback();
    LOG_I(TAG, "Playback started");
}

void EventHandler::handlePlaybackStop(const Event &event) {
    audio.stopPlayback();
    LOG_I(TAG, "Playback stopped");
}

void EventHandler::handleWebSocketConnected(const Event &event) {
    network.sendInitMessage();
}

void EventHandler::handleAudioDataReceived(const Event &event) {
    audio.addDataToBuffer(reinterpret_cast<const uint8_t *>(event.data.data()), event.dataLength);
    LOG_D(TAG, "Audio data received and added to buffer");
}

void EventHandler::handleAudioChunkRead(const Event &event) {
    network.sendAudioChunk(reinterpret_cast<const uint8_t *>(event.data.data()), event.dataLength);
    LOG_D(TAG, "Audio chunk read and sent");
}

void EventHandler::handleFingerprintMatch(const Event &event) {
    LOG_I(TAG, "Fingerprint match found!");
    gate.openGate();
    led.turnOn();
}

void EventHandler::handleFingerprintNoMatch(const Event &event) {
    LOG_I(TAG, "Fingerprint no match found!");
    led.turnOff();
}

void EventHandler::handleChangeState(const Event &event) {
    StaticJsonDocument<256> doc;
    deserializeJson(doc, event.data);

    const char *device = doc["device"];
    const char *state = doc["state"];

    if (strcmp(device, "gate") == 0) {
        strcmp(state, "open") == 0 ? gate.openGate() : gate.closeGate();
    } else if (strcmp(device, "light") == 0) {
        strcmp(state, "on") == 0 ? led.turnOn() : led.turnOff();
    } else {
        LOG_W(TAG, "Unknown device: %s", device);
    }
}

void EventHandler::handleChangeStateSuccess(const Event &event) {
    StaticJsonDocument<256> data;

    if (event.type == GATE_OPENED || event.type == GATE_CLOSED) {
        data["device"] = "gate";
        data["state"] = event.type == GATE_OPENED ? "open" : "close";
    } else if (event.type == LED_TURNED_ON || event.type == LED_TURNED_OFF) {
        data["device"] = "light";
        data["state"] = event.type == LED_TURNED_ON ? "on" : "off";
    }

    network.sendEvent("change_state", data.as<JsonObject>());
}

void EventHandler::handleResidentAuthorized(const Event &event) {
    gate.openGate();
}

void EventHandler::handleGrantAccess(const Event &event) {
    gate.openGate();
    ui.displayAccessGranted();
}

void EventHandler::handleDenyAccess(const Event &event) {
    ui.displayAccessDenied();
}

void EventHandler::handleMotionDetected(const Event &event) {
    espNow.sendCommand("capture_image");
    network.sendEvent("motion_detected", JsonObject());
}

#include "event_handler.h"
#include "logger.h"
#include "esp_now_manager.h"
#include <ArduinoJson.h>

static const char *TAG = "EventHandler";

EventHandler::EventHandler(Audio &audio, NetworkManager &network, GateControl &gate, LEDControl &led)
        : _audio(audio), _network(network), _gate(gate), _led(led) {}

void EventHandler::registerCallbacks(EventDispatcher &dispatcher) {
    dispatcher.registerCallback(CMD_RECORD_START, [this](const Event &e) { handleRecordStart(e); });
    dispatcher.registerCallback(CMD_RECORD_STOP, [this](const Event &e) { handleRecordStop(e); });
    dispatcher.registerCallback(CMD_PLAYBACK_START, [this](const Event &e) { handlePlaybackStart(e); });
    dispatcher.registerCallback(CMD_PLAYBACK_STOP, [this](const Event &e) { handlePlaybackStop(e); });
    dispatcher.registerCallback(WS_CONNECTED, [this](const Event &e) { handleWebSocketConnected(e); });
    dispatcher.registerCallback(AUDIO_DATA_RECEIVED, [this](const Event &e) { handleAudioDataReceived(e); });
    dispatcher.registerCallback(AUDIO_CHUNK_READ, [this](const Event &e) { handleAudioChunkRead(e); });
    dispatcher.registerCallback(FINGERPRINT_MATCH, [this](const Event &e) { handleFingerprintMatch(e); });
    dispatcher.registerCallback(FINGERPRINT_NO_MATCH, [this](const Event &e) { handleFingerprintNoMatch(e); });
    dispatcher.registerCallback(SEND_CAPTURE_IMAGE_COMMAND,
                                [this](const Event &e) { handleSendCaptureImageCommand(e); });
    dispatcher.registerCallback(CMD_CHANGE_STATE, [this](const Event &e) { handleChangeState(e); });
    dispatcher.registerCallback(GATE_OPENED, [this](const Event &e) { handleChangeStateSuccess(e); });
    dispatcher.registerCallback(GATE_CLOSED, [this](const Event &e) { handleChangeStateSuccess(e); });
    dispatcher.registerCallback(LED_TURNED_ON, [this](const Event &e) { handleChangeStateSuccess(e); });
    dispatcher.registerCallback(LED_TURNED_OFF, [this](const Event &e) { handleChangeStateSuccess(e); });
    dispatcher.registerCallback(WS_DISCONNECTED, [this](const Event &e) { /* Handle WebSocket disconnected */ });

}


void EventHandler::handleRecordStart(const Event &event) {
    _audio.startRecording();
    LOG_I(TAG, "Recording started");
}

void EventHandler::handleRecordStop(const Event &event) {
    _audio.stopRecording();
    LOG_I(TAG, "Recording stopped");
}

void EventHandler::handlePlaybackStart(const Event &event) {
    _audio.startPlayback();
    LOG_I(TAG, "Playback started");
}

void EventHandler::handlePlaybackStop(const Event &event) {
    _audio.stopPlayback();
    LOG_I(TAG, "Playback stopped");
}

void EventHandler::handleWebSocketConnected(const Event &event) {
    _network.sendInitMessage();
}

void EventHandler::handleAudioDataReceived(const Event &event) {
    _audio.addDataToBuffer(reinterpret_cast<const uint8_t *>(event.data.data()), event.dataLength);
    LOG_D(TAG, "Audio data received and added to buffer");
}

void EventHandler::handleAudioChunkRead(const Event &event) {
    _network.sendAudioChunk(reinterpret_cast<const uint8_t *>(event.data.data()), event.dataLength);
    LOG_D(TAG, "Audio chunk read and sent");
}

void EventHandler::handleFingerprintMatch(const Event &event) {
    LOG_I(TAG, "Fingerprint match found!");
    _gate.openGate();
    _led.turnOn();
}

void EventHandler::handleFingerprintNoMatch(const Event &event) {
    LOG_I(TAG, "Fingerprint no match found!");
    _led.turnOff();
}

void EventHandler::handleSendCaptureImageCommand(const Event &event) {
    ESPNow::sendCommand("capture_image");
    LOG_I(TAG, "Capture image command sent");
}

void EventHandler::handleChangeState(const Event &event) {
    StaticJsonDocument<256> doc;
    DeserializationError error = deserializeJson(doc, event.data);

    const char *device = doc["device"];
    const char *state = doc["state"];

    if (strcmp(device, "gate") == 0) {
        strcmp(state, "open") == 0 ? _gate.openGate() : _gate.closeGate();
    } else if (strcmp(device, "light") == 0) {
        strcmp(state, "on") == 0 ? _led.turnOn() : _led.turnOff();
    } else {
        LOG_W(TAG, "Unknown device: %s", device);
    }
}

void EventHandler::handleChangeStateSuccess(const Event &event) {
    StaticJsonDocument<256> data;

    if (event.type == GATE_OPENED) {
        data["device"] = "gate";
        data["state"] = "open";
    } else if (event.type == GATE_CLOSED) {
        data["device"] = "gate";
        data["state"] = "close";
    } else if (event.type == LED_TURNED_ON) {
        data["device"] = "light";
        data["state"] = "on";
    } else if (event.type == LED_TURNED_OFF) {
        data["device"] = "light";
        data["state"] = "off";
    }

    _network.sendEvent("change_state", data.as<JsonObject>());
}
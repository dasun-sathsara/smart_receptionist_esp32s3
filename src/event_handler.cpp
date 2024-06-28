#include "event_handler.h"
#include "logger.h"
#include "esp_now_manager.h"
#include <ArduinoJson.h>

static const char *TAG = "EventHandler";

EventHandler::EventHandler(Audio &audio, NetworkManager &network, GateControl &gate, LEDControl &led, UI &ui, ESPNow &espNow)
        : audio(audio), network(network), gate(gate), led(led), ui(ui), espNow(espNow) {}

void EventHandler::registerCallbacks(EventDispatcher &dispatcher) {
    dispatcher.registerCallback(CMD_TG_AUDIO, [this](const Event &e) { handleTelegramAudioCommand(e); });
    dispatcher.registerCallback(CMD_ESP_AUDIO, [this](const Event &e) { handleESPAudioCommand(e); });
    dispatcher.registerCallback(AUDIO_DATA_RECEIVED, [this](const Event &e) { handleAudioDataReceived(e); });

    dispatcher.registerCallback(WS_CONNECTED, [this](const Event &e) { handleWebSocketConnected(e); });

    dispatcher.registerCallback(FINGERPRINT_MATCHED, [this](const Event &e) { handleFingerprintMatch(e); });
    dispatcher.registerCallback(FINGERPRINT_NO_MATCH, [this](const Event &e) { handleFingerprintNoMatch(e); });

    dispatcher.registerCallback(CMD_CHANGE_STATE, [this](const Event &e) { handleChangeState(e); });
    dispatcher.registerCallback(GATE_OPENED, [this](const Event &e) { handleChangeStateSuccess(e); });
    dispatcher.registerCallback(GATE_CLOSED, [this](const Event &e) { handleChangeStateSuccess(e); });
    dispatcher.registerCallback(LED_TURNED_ON, [this](const Event &e) { handleChangeStateSuccess(e); });
    dispatcher.registerCallback(LED_TURNED_OFF, [this](const Event &e) { handleChangeStateSuccess(e); });

    dispatcher.registerCallback(PASSWORD_VALID, [this](const Event &e) { handlePasswordValid(e); });
    dispatcher.registerCallback(PASSWORD_INVALID, [this](const Event &e) { handlePasswordInvalid(e); });

    dispatcher.registerCallback(CMD_GRANT_ACCESS, [this](const Event &e) { handleAccessGranted(e); });
    dispatcher.registerCallback(CMD_DENY_ACCESS, [this](const Event &e) { handleAccessDenied(e); });

    dispatcher.registerCallback(MOTION_DETECTED, [this](const Event &e) { handleMotionDetected(e); });
}


void EventHandler::handleTelegramAudioCommand(const Event &event) {
    std::string action = event.data;

    if (action == "start_recording") {
        audio.startRecording();
        LOG_I(TAG, "Recording started from Telegram.");
    } else if (action == "stop_recording") {
        audio.stopRecording();
        LOG_I(TAG, "Recording stopped from Telegram.");
    } else if (action == "start_playing") {
        audio.startPlayback();
        LOG_I(TAG, "Started playback from Telegram.");
    } else if (action == "stop_playing") {
        audio.stopPlayback();
        LOG_I(TAG, "Stopped playback from Telegram.");
    }
}

void EventHandler::handleESPAudioCommand(const Event &event) {
    std::string action = event.data;

    StaticJsonDocument<200> data;
    if (action == "start_recording") {
        audio.startRecording();
        data["action"] = "start_recording";
        network.sendEvent("audio", data.as<JsonObject>());
        LOG_I(TAG, "Recording started from ESP.");

    } else if (action == "stop_recording") {
        audio.stopRecording();
        data["action"] = "stop_recording";
        network.sendEvent("audio", data.as<JsonObject>());
        LOG_I(TAG, "Recording stopped from ESP.");

    } else if (action == "start_playing") {
        audio.startPlayback();
        data["action"] = "start_playing";
        network.sendEvent("audio", data.as<JsonObject>());
        LOG_I(TAG, "Started playback from ESP.");

    } else if (action == "stop_playing") {
        audio.stopPlayback();
        data["action"] = "stop_playing";
        network.sendEvent("audio", data.as<JsonObject>());
        LOG_I(TAG, "Stopped playback from ESP.");
    }
}


void EventHandler::handleWebSocketConnected(const Event &event) {
    network.sendInitMessage();
}

void EventHandler::handleAudioDataReceived(const Event &event) {
    audio.addDataToBuffer(reinterpret_cast<const uint8_t *>(event.data.data()), event.dataLength);
}

void EventHandler::handleFingerprintMatch(const Event &event) {
    ui.setStateFor(2, UIState::FINGERPRINT_MATCHED);
    handleResidentAuthorized(event);
    LOG_I(TAG, "Fingerprint match found!");
}

void EventHandler::handleFingerprintNoMatch(const Event &event) {
    ui.setStateFor(2, UIState::FINGERPRINT_NO_MATCH);
    LOG_I(TAG, "Fingerprint no match found!");
}

void EventHandler::handlePasswordValid(const Event &event) {
    handleResidentAuthorized(event);
    LOG_I(TAG, "Password correct!");
}

void EventHandler::handlePasswordInvalid(const Event &event) {
    LOG_I(TAG, "Password incorrect!");
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
    LOG_I(TAG, "Resident authorized!");
}

void EventHandler::handleAccessGranted(const Event &event) {
    ui.setState(UIState::ACCESS_GRANTED);
    gate.openGate();
    LOG_I(TAG, "Access granted!");
}

void EventHandler::handleAccessDenied(const Event &event) {
    ui.setState(UIState::ACCESS_DENIED);
    LOG_I(TAG, "Access denied!");
}

void EventHandler::handleMotionDetected(const Event &event) {
    ui.setStateFor(2, UIState::MOTION_DETECTED);
    espNow.sendCommand("capture_image");
    network.sendEvent("motion_detected", JsonObject());
    LOG_I(TAG, "Motion detected and visitor identification initiated!");
}

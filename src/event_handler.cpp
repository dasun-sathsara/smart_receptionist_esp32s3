#include "event_handler.h"
#include "logger.h"
#include "esp_now_manager.h"
#include <ArduinoJson.h>

static const char *TAG = "EventHandler";

EventHandler::EventHandler(Audio &audio, NetworkManager &network, Gate &gate, LED &led, UI &ui, ESPNow &espNow,
                           FingerprintHandler &fingerprint)
        : audio(audio), network(network), gate(gate), led(led), ui(ui), espNow(espNow), fingerprint(fingerprint) {}

void EventHandler::registerCallbacks(EventDispatcher &dispatcher) {
    // Audio Commands
    dispatcher.registerCallback(CMD_TG_AUDIO, [this](const Event &e) { handleTelegramAudioCommand(e); });
    dispatcher.registerCallback(CMD_ESP_AUDIO, [this](const Event &e) { handleESPAudioCommand(e); });
    dispatcher.registerCallback(AUDIO_DATA_RECEIVED, [this](const Event &e) { handleAudioDataReceived(e); });
    dispatcher.registerCallback(AUDIO_DATA_READY, [this](const Event &e) { handleAudioDataReady(e); });

    // WebSocket Events
    dispatcher.registerCallback(WS_CONNECTED, [this](const Event &e) { handleWebSocketConnected(); });

    // Authentication Events
    dispatcher.registerCallback(FINGERPRINT_MATCHED, [this](const Event &e) { handleFingerprintMatch(e); });
    dispatcher.registerCallback(FINGERPRINT_NO_MATCH, [this](const Event &e) { handleFingerprintNoMatch(); });
    dispatcher.registerCallback(PASSWORD_VALID, [this](const Event &e) { handlePasswordValid(e); });
    dispatcher.registerCallback(PASSWORD_INVALID, [this](const Event &e) { handlePasswordInvalid(); });

    // State Change Commands
    dispatcher.registerCallback(CMD_CHANGE_STATE, [this](const Event &e) { handleChangeState(e); });
    dispatcher.registerCallback(GATE_OPENED, [this](const Event &e) { handleChangeStateSuccess(e); });
    dispatcher.registerCallback(GATE_CLOSED, [this](const Event &e) { handleChangeStateSuccess(e); });
    dispatcher.registerCallback(LED_TURNED_ON, [this](const Event &e) { handleChangeStateSuccess(e); });
    dispatcher.registerCallback(LED_TURNED_OFF, [this](const Event &e) { handleChangeStateSuccess(e); });

    // Access Control
    dispatcher.registerCallback(CMD_GRANT_ACCESS, [this](const Event &e) { handleAccessGranted(); });
    dispatcher.registerCallback(CMD_DENY_ACCESS, [this](const Event &e) { handleAccessDenied(); });

    // Detection Events
    dispatcher.registerCallback(MOTION_DETECTED, [this](const Event &e) { handleMotionDetected(); });
    dispatcher.registerCallback(PERSON_DETECTED, [this](const Event &e) { handlePersonDetected(); });

    // Gate Events
    dispatcher.registerCallback(VISITOR_ENTERED, [this](const Event &e) { handleVisitorEntered(); });

    // Miscellaneous Events
    dispatcher.registerCallback(RECORDING_SENT, [this](const Event &e) { handleRecordingSent(); });
    dispatcher.registerCallback(NO_AUDIO_DATA, [this](const Event &e) { ui.setStateFor(3, UIState::NO_AUDIO_DATA); });

    // Power Saving
    dispatcher.registerCallback(MOTION_DETECTED, [this](const Event &e) { handleMotionDetected(e); });
    dispatcher.registerCallback(INACTIVITY_DETECTED, [this](const Event &e) { handleInactivityDetected(e); });
}


void EventHandler::handleTelegramAudioCommand(const Event &event) {
    std::string action = event.data;
    StaticJsonDocument<200> data;

    if (action == "start_recording") {
        audio.startRecording();
        data["action"] = "start_recording";
    } else if (action == "stop_recording") {
        audio.stopRecording();
        data["action"] = "stop_recording";
    } else if (action == "start_playing") {
        audio.startPlayback();
        data["action"] = "start_playing";
    } else if (action == "stop_playing") {
        audio.stopPlayback();
        data["action"] = "stop_playing";
    } else if (action == "start_prefetch") {
        audio.startPrefetch();
        data["action"] = "start_prefetch";
    } else if (action == "stop_prefetch") {
        audio.stopPrefetch();
        data["action"] = "stop_prefetch";
    } else {
        LOG_W(TAG, "Unknown Telegram audio command: %s", action.c_str());
        return;
    }

    LOG_I(TAG, "Telegram audio command executed: %s", action.c_str());
}

void EventHandler::handleRecordingSent() {
    StaticJsonDocument<200> data;
    data["event_type"] = "recording_sent";
    network.sendEvent("recording_sent", data.as<JsonObject>());
    LOG_I(TAG, "Recording sent event dispatched");
}

void EventHandler::handleAudioDataReady(const Event &event) {
    network.sendAudioChunk(reinterpret_cast<const uint8_t *>(event.data.c_str()), event.dataLength);
}

void EventHandler::handleESPAudioCommand(const Event &event) {
    std::string action = event.data;
    StaticJsonDocument<200> data;

    if (action == "start_recording") {
        audio.startRecording();
        data["action"] = "start_recording";
    } else if (action == "stop_recording") {
        audio.stopRecording();
        data["action"] = "stop_recording";
    } else if (action == "start_playing") {
        audio.startPlayback();
        data["action"] = "start_playing";
    } else if (action == "stop_playing") {
        audio.stopPlayback();
        data["action"] = "stop_playing";
    } else if (action == "start_prefetch") {
        audio.startPrefetch();
        data["action"] = "start_prefetch";
    } else if (action == "stop_prefetch") {
        audio.stopPrefetch();
        data["action"] = "stop_prefetch";
    } else {
        LOG_W(TAG, "Unknown ESP audio command: %s", action.c_str());
        return;
    }

    network.sendEvent("audio", data.as<JsonObject>());
    LOG_I(TAG, "ESP audio command executed: %s", action.c_str());
}


void EventHandler::handleWebSocketConnected() {
    network.sendInitMessage();
}

void EventHandler::handleAudioDataReceived(const Event &event) {
    const auto *audioData = reinterpret_cast<const uint8_t *>(event.data.c_str());
    size_t dataLength = event.dataLength;
    Audio::addPrefetchData(audioData, dataLength);
}

void EventHandler::handleFingerprintMatch(const Event &event) {
    ui.setStateFor(2, UIState::FINGERPRINT_MATCHED);
    handleResidentAuthorized();
    LOG_I(TAG, "Fingerprint match found!");
}

void EventHandler::handleFingerprintNoMatch() {
    ui.setStateFor(2, UIState::FINGERPRINT_NO_MATCH);
    LOG_I(TAG, "Fingerprint no match found!");
}

void EventHandler::handlePasswordValid(const Event &event) {
    handleResidentAuthorized();
    LOG_I(TAG, "Password correct!");
}

void EventHandler::handlePasswordInvalid() {
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

void EventHandler::handleResidentAuthorized() {
    gate.openGate();
    LOG_I(TAG, "Resident authorized!");
}

void EventHandler::handleAccessGranted() {
    ui.setState(UIState::ACCESS_GRANTED);
    gate.openGate();
    LOG_I(TAG, "Access granted!");
}

void EventHandler::handleAccessDenied() {
    ui.setState(UIState::ACCESS_DENIED);
    LOG_I(TAG, "Access denied!");
}

void EventHandler::handleMotionDetected() {
    ui.setStateFor(2, UIState::MOTION_DETECTED);
    espNow.sendCommand("capture_image");
    network.sendEvent("motion_detected", JsonObject());
    LOG_I(TAG, "Motion detected and visitor identification initiated!");
}

void EventHandler::handlePersonDetected() {
    network.sendEvent("person_detected", JsonObject());
    LOG_I(TAG, "Person detected!");
}

void EventHandler::handleVisitorEntered() {
    LOG_I(TAG, "Visitor entered the premises!");
    StaticJsonDocument<256> data;
    data["event_type"] = "visitor_entered";
    network.sendEvent("visitor_entered", data.as<JsonObject>());
}

void EventHandler::handleMotionDetected(const Event &event) {
    ui.enableDisplay();
    fingerprint.enableSensor();
    LOG_I(TAG, "Motion detected, components enabled");
}

void EventHandler::handleInactivityDetected(const Event &event) {
    ui.disableDisplay();
    fingerprint.disableSensor();
    LOG_I(TAG, "Inactivity detected, components disabled");
}
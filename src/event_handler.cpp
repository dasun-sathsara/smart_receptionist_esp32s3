#include "event_handler.h"
#include "logger.h"
#include "esp_now_manager.h"

static const char *TAG = "EventHandler";

EventHandler::EventHandler(Audio &audio, NetworkManager &network, GateControl &gate, LEDControl &led)
        : _audio(audio), _network(network), _gate(gate), _led(led) {}

void EventHandler::registerCallbacks(EventDispatcher &dispatcher) {
    dispatcher.registerCallback(RECORD_START, [this](const Event &e) { handleRecordStart(e); });
    dispatcher.registerCallback(RECORD_STOP, [this](const Event &e) { handleRecordStop(e); });
    dispatcher.registerCallback(PLAYBACK_START, [this](const Event &e) { handlePlaybackStart(e); });
    dispatcher.registerCallback(PLAYBACK_STOP, [this](const Event &e) { handlePlaybackStop(e); });
    dispatcher.registerCallback(WEBSOCKET_CONNECTED, [this](const Event &e) { handleWebSocketConnected(e); });
    dispatcher.registerCallback(AUDIO_DATA_RECEIVED, [this](const Event &e) { handleAudioDataReceived(e); });
    dispatcher.registerCallback(AUDIO_CHUNK_READ, [this](const Event &e) { handleAudioChunkRead(e); });
    dispatcher.registerCallback(FINGERPRINT_MATCH, [this](const Event &e) { handleFingerprintMatch(e); });
    dispatcher.registerCallback(FINGERPRINT_NO_MATCH, [this](const Event &e) { handleFingerprintNoMatch(e); });
    dispatcher.registerCallback(SEND_CAPTURE_IMAGE_COMMAND,
                                [this](const Event &e) { handleSendCaptureImageCommand(e); });
    dispatcher.registerCallback(CHANGE_STATE, [this](const Event &e) { handleChangeState(e); });
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
    LOG_I(TAG, "WebSocket connected, init message sent");
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
    if (event.data == "OPEN") {
        _gate.openGate();
        _led.turnOn();
        LOG_I(TAG, "Gate opened and LED turned on");
    } else if (event.data == "CLOSE") {
        _gate.closeGate();
        _led.turnOff();
        LOG_I(TAG, "Gate closed and LED turned off");
    }
}
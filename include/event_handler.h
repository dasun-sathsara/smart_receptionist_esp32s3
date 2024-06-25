#ifndef EVENT_HANDLER_H
#define EVENT_HANDLER_H

#include "events.h"
#include "audio.h"
#include "network_manager.h"
#include "gate_control.h"
#include "led_control.h"
#include "ui.h"
#include "esp_now.h"
#include "esp_now_manager.h"

class EventHandler {
public:
    EventHandler(Audio &audio, NetworkManager &network, GateControl &gate, LEDControl &led, UI &ui, ESPNow &espNow);

    void registerCallbacks(EventDispatcher &dispatcher);

private:
    Audio &audio;
    NetworkManager &network;
    GateControl &gate;
    LEDControl &led;
    UI &ui;
    ESPNow &espNow;

    void handleRecordStart(const Event &event);

    void handleRecordStop(const Event &event);

    void handlePlaybackStart(const Event &event);

    void handlePlaybackStop(const Event &event);

    void handleWebSocketConnected(const Event &event);

    void handleAudioDataReceived(const Event &event);

    void handleAudioChunkRead(const Event &event);

    void handleFingerprintMatch(const Event &event);

    void handleFingerprintNoMatch(const Event &event);

    void handleChangeState(const Event &event);

    void handleChangeStateSuccess(const Event &event);

    void handleResidentAuthorized(const Event &event);

    void handleGrantAccess(const Event &event);

    void handleDenyAccess(const Event &event);

    void handleMotionDetected(const Event &event);
};

#endif // EVENT_HANDLER_H
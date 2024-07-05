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

    void handleTelegramAudioCommand(const Event &event);

    void handleAudioDataReceived(const Event &event);

    void handleFingerprintMatch(const Event &event);

    void handleChangeState(const Event &event);

    void handlePasswordValid(const Event &event);

    void handleChangeStateSuccess(const Event &event);

    void handleESPAudioCommand(const Event &event);

    void handleAudioDataReady(const Event &event);

    void handleWebSocketConnected();

    void handleFingerprintNoMatch();

    void handleResidentAuthorized();

    void handleAccessGranted();

    void handleAccessDenied();

    void handleMotionDetected();

    void handlePasswordInvalid();

    void handlePersonDetected();

    void handleVisitorEntered();

    void handleRecordingSent();
};

#endif // EVENT_HANDLER_H
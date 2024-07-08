#ifndef EVENT_HANDLER_H
#define EVENT_HANDLER_H

#include "events.h"
#include "audio.h"
#include "network_manager.h"
#include "gate.h"
#include "led.h"
#include "ui.h"
#include "esp_now.h"
#include "esp_now_manager.h"

class EventHandler {
public:
    EventHandler(Audio &audio, NetworkManager &network, Gate &gate, LED &led, UI &ui, ESPNow &espNow);

    void registerCallbacks(EventDispatcher &dispatcher);

private:
    Audio &audio;
    NetworkManager &network;
    Gate &gate;
    LED &led;
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

    void handleRecordingSent();

    void handleBreakBeamTriggered();

    void handleGateFullyClosed();

    void handleVisitorEntered();
};

#endif // EVENT_HANDLER_H

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
#include "fingerprint.h"
#include "pir.h"

class EventHandler {
public:
    EventHandler(Audio &audio, NetworkManager &network, Gate &gate, LED &led, UI &ui, ESPNow &espNow, FingerprintHandler &fingerprint,
                 PIRSensor &pir);

    void registerCallbacks(EventDispatcher &dispatcher);

private:
    Audio &audio;
    NetworkManager &network;
    Gate &gate;
    LED &led;
    UI &ui;
    ESPNow &espNow;
    FingerprintHandler &fingerprint;
    PIRSensor &pir;

    void handleTelegramAudioCommand(const Event &event);

    void handleAudioDataReceived(const Event &event);

    void handleFingerprintMatch(const Event &event);

    void handleChangeState(const Event &event);

    void handlePasswordValid(const Event &event);

    void handleChangeStateSuccess(const Event &event);

    void handleESPAudioCommand(const Event &event);

    void handleAudioDataReady(const Event &event);


    void handleFingerprintNoMatch();

    void handleResidentAuthorized();

    void handleAccessGranted();

    void handleAccessDenied();

    void handleMotionDetected();

    void handlePasswordInvalid();

    void handlePersonDetected();

    void handleRecordingSent();

    void handleFingerprintEnroll(const Event &event);

    void handleInactivityDetected(const Event &event);

    void handleVisitorEntered();

    void handlePlaceFinger(const Event &event);

    void handlePlaceFingerAgain(const Event &event);

    void handleRemoveFinger(const Event &event);

    void handleFingerprintEnrolled(const Event &event);

    void handleFingerprintEnrollFailed(const Event &event);

    void handleMotionEnable(const Event &event);

};

#endif // EVENT_HANDLER_H

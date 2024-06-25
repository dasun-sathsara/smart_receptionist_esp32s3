#ifndef EVENT_HANDLER_H
#define EVENT_HANDLER_H

#include "events.h"
#include "audio.h"
#include "network_manager.h"
#include "gate_control.h"
#include "led_control.h"

class EventHandler {
public:
    EventHandler(Audio& audio, NetworkManager& network, GateControl& gate, LEDControl& led);
    void registerCallbacks(EventDispatcher& dispatcher);

private:
    Audio& _audio;
    NetworkManager& _network;
    GateControl& _gate;
    LEDControl& _led;

    void handleRecordStart(const Event& event);
    void handleRecordStop(const Event& event);
    void handlePlaybackStart(const Event& event);
    void handlePlaybackStop(const Event& event);
    void handleWebSocketConnected(const Event& event);
    void handleAudioDataReceived(const Event& event);
    void handleAudioChunkRead(const Event& event);
    void handleFingerprintMatch(const Event& event);
    void handleFingerprintNoMatch(const Event& event);
    void handleSendCaptureImageCommand(const Event& event);
    void handleChangeState(const Event& event);
};

#endif // EVENT_HANDLER_H
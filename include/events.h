#ifndef EVENTS_H
#define EVENTS_H

#include <functional>
#include <vector>
#include <bitset>
#include <cstdint>
#include <string>

// Increase or decrease this value based on the number of events you need
constexpr size_t MAX_EVENTS = 64;

using EventType = uint8_t;

struct Event {
    EventType type;
    std::string data;
    size_t dataLength;

    Event(EventType t, const std::string &d = "", size_t s = 0)
            : type(t), data(d), dataLength(s == 0 ? d.size() : s) {}
};

using EventCallback = std::function<void(const Event &)>;

class EventDispatcher {
public:
    void registerCallback(EventType type, EventCallback callback);

    void dispatchEvent(const Event &event);

    void removeCallback(EventType type);

private:
    struct CallbackEntry {
        EventType type;
        EventCallback callback;
    };

    std::vector<CallbackEntry> callbacks;
    std::bitset<MAX_EVENTS> registeredEvents;
};

// Define your event types here
enum Events : EventType {
    WS_CONNECTED = 0,
    CMD_START_RECORDING,
    CMD_STOP_RECORDING,
    CMD_START_PLAYING,
    CMD_STOP_PLAYING,
    FINGERPRINT_MATCHED,
    FINGERPRINT_NO_MATCH,
    AUDIO_DATA_RECEIVED,
    ESPNOW_DATA_RECEIVED,
    MOTION_DETECTED,
    BREAK_BEAM_TRIGGERED,
    CMD_CHANGE_STATE,
    GATE_OPENED,
    GATE_CLOSED,
    LED_TURNED_ON,
    LED_TURNED_OFF,
    PASSWORD_VALIDATED,
    CMD_GRANT_ACCESS,
    CMD_DENY_ACCESS

};

#endif // EVENTS_H
#ifndef EVENTS_H
#define EVENTS_H

#include <functional>
#include <vector>
#include <bitset>
#include <cstdint>
#include <string>

constexpr size_t MAX_EVENTS = 64;

using EventType = uint8_t;

struct Event {
    EventType type;
    std::string data;
    size_t dataLength;

    Event(EventType t, const std::string &d = "", size_t s = 0) : type(t), data(d), dataLength(s == 0 ? d.size() : s) {}
};

using EventCallback = std::function<void(const Event &)>;

class EventDispatcher {
public:
    void registerCallback(EventType type, EventCallback callback);

    void dispatchEvent(const Event &event);

private:
    struct CallbackEntry {
        EventType type;
        EventCallback callback;
    };

    std::vector<CallbackEntry> callbacks;
    std::bitset<MAX_EVENTS> registeredEvents;
};

enum Events : EventType {
    WS_CONNECTED = 0,
    CMD_TG_AUDIO,
    CMD_ESP_AUDIO,
    FINGERPRINT_MATCHED,
    FINGERPRINT_NO_MATCH,
    AUDIO_DATA_RECEIVED,
    ESPNOW_DATA_RECEIVED,
    MOTION_DETECTED,
    VISITOR_ENTERED,
    CMD_CHANGE_STATE,
    GATE_OPENED,
    GATE_CLOSED,
    LED_TURNED_ON,
    PERSON_DETECTED,
    LED_TURNED_OFF,
    PASSWORD_VALID,
    PASSWORD_INVALID,
    CMD_GRANT_ACCESS,
    CMD_DENY_ACCESS
};

#endif // EVENTS_H
#ifndef EVENTS_H
#define EVENTS_H

#include <functional>
#include <unordered_map>
#include <queue>

enum EventType {
    EVENT_WIFI_CONNECTED,
    EVENT_WIFI_DISCONNECTED,
    EVENT_WEBSOCKET_CONNECTED,
    EVENT_WEBSOCKET_DISCONNECTED,
    EVENT_RECORD_START,
    EVENT_RECORD_STOP,
    EVENT_PLAYBACK_START,
    EVENT_PLAYBACK_STOP,
    EVENT_KEYPAD_PRESS,
    EVENT_FINGERPRINT_MATCH,
    EVENT_FINGERPRINT_NO_MATCH,
    EVENT_AUDIO_DATA_RECEIVED,
    EVENT_AUDIO_CHUNK_READ,
};

struct Event {
    EventType type;
    std::string data;
    size_t dataLength;
};

using EventCallback = std::function<void(const Event &)>;

class EventDispatcher {
public:
    void registerCallback(EventType type, EventCallback callback);

    void dispatchEvent(const Event &event);

private:
    std::unordered_map<EventType, std::vector<EventCallback>> callbacks;
};

#endif // EVENTS_H

#ifndef EVENTS_H
#define EVENTS_H

#include <functional>
#include <unordered_map>
#include <queue>

enum EventType {
    WIFI_CONNECTED,
    WIFI_DISCONNECTED,
    WEBSOCKET_CONNECTED,
    WEBSOCKET_DISCONNECTED,
    RECORD_START,
    RECORD_STOP,
    PLAYBACK_START,
    PLAYBACK_STOP,
    KEYPAD_PRESS,
    FINGERPRINT_MATCH,
    FINGERPRINT_NO_MATCH,
    AUDIO_DATA_RECEIVED,
    AUDIO_CHUNK_READ,
    ESPNOW_DATA_RECEIVED,
    SEND_CAPTURE_IMAGE_COMMAND
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

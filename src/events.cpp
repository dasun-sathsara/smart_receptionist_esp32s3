#include "events.h"

void EventDispatcher::registerCallback(EventType type, const EventCallback& callback) {
    callbacks[type].push_back(callback);
}

void EventDispatcher::dispatchEvent(const Event &event) {
    for (auto &callback: callbacks[event.type]) {
        callback(event);
    }
}
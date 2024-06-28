#include "events.h"
#include "logger.h"

static const char *TAG = "EventDispatcher";

void EventDispatcher::registerCallback(EventType type, EventCallback callback) {
    if (type >= MAX_EVENTS) {
        LOG_E(TAG, "Event type out of range: %d", type);
        return;
    }

    callbacks.push_back({type, std::move(callback)});
    registeredEvents.set(type);
    LOG_D(TAG, "Registered callback for event type: %d", type);
}

void EventDispatcher::dispatchEvent(const Event &event) {
    if (event.type >= MAX_EVENTS) {
        LOG_E(TAG, "Invalid event type: %d", event.type);
        return;
    }

    if (!registeredEvents.test(event.type)) {
        LOG_W(TAG, "No callbacks registered for event type: %d", event.type);
        return;
    }

    LOG_D(TAG, "Dispatching event: %d, data size: %zu", event.type, event.dataLength);
    for (const auto &entry: callbacks) {
        if (entry.type == event.type) {
            entry.callback(event);
        }
    }
}
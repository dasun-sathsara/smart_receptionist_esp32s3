#ifndef WIFI_H
#define WIFI_H

#include "WebSocketsClient.h"
#include "events.h"

class NetworkManager {
public:
    void begin(EventDispatcher &dispatcher);

    static void loop();

    static void sendInitMessage();

    static void sendAudioChunk(const uint8_t *data, size_t len);

private:
    static void webSocketEvent(WStype_t type, uint8_t *payload, size_t length);

    static EventDispatcher *eventDispatcher;
    static WebSocketsClient webSocket;

};

#endif // WIFI_H
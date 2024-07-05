#ifndef WIFI_H
#define WIFI_H

#include "WebSocketsClient.h"
#include "events.h"
#include "ArduinoJson.h"

class NetworkManager {
public:
    void begin(EventDispatcher &dispatcher);

    [[noreturn]] static void loop(void *pvParameters);

    static void sendInitMessage();

    static void sendAudioChunk(const uint8_t *data, size_t len);

    static void sendEvent(const char *eventType, const JsonObject &data);

    [[noreturn]] static void reconnectTask(void *pvParameters);

private:
    static void webSocketEvent(WStype_t type, uint8_t *payload, size_t length);

    static EventDispatcher *eventDispatcher;
    static WebSocketsClient webSocket;

};

#endif // WIFI_H

#include "network_manager.h"
#include "config.h"
#include "audio.h"
#include <ArduinoJson.h>
#include <WiFi.h>
#include "logger.h"

static const char *TAG = "NetworkManager";

EventDispatcher *NetworkManager::eventDispatcher = nullptr;
WebSocketsClient NetworkManager::webSocket;

void NetworkManager::begin(EventDispatcher &dispatcher) {
    eventDispatcher = &dispatcher;

    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    int connectionAttempts = 0;
    while (WiFiClass::status() != WL_CONNECTED && connectionAttempts < 10) {
        delay(500);
        connectionAttempts++;
    }

    if (WiFiClass::status() == WL_CONNECTED) {
        LOG_I(TAG, "Connected to WiFi network");
        eventDispatcher->dispatchEvent({WIFI_CONNECTED, ""});
    } else {
        LOG_E(TAG, "Failed to connect to WiFi network");
        eventDispatcher->dispatchEvent({WIFI_DISCONNECTED, ""});
        return;
    }

    webSocket.begin(WS_SERVER, WS_PORT);
    webSocket.onEvent(webSocketEvent);

    xTaskCreate([](void *param) {
        while (true) {
            NetworkManager::loop();
            vTaskDelay(10);
        }
    }, "WiFi Task", 4096, this, 2, nullptr);
}


void NetworkManager::loop() {
    webSocket.loop();
}

void NetworkManager::webSocketEvent(WStype_t type, uint8_t *payload, size_t length) {
    StaticJsonDocument<200> doc;

    switch (type) {
        case WStype_DISCONNECTED:
            LOG_I(TAG, "WebSocket disconnected");
            eventDispatcher->dispatchEvent({WEBSOCKET_DISCONNECTED, ""});
            // Handle WebSocket disconnection (e.g., reattempt connection)
            break;
        case WStype_CONNECTED:
            LOG_I(TAG, "WebSocket connected");
            eventDispatcher->dispatchEvent({WEBSOCKET_CONNECTED, ""});
            break;
        case WStype_TEXT: {

            DeserializationError error = deserializeJson(doc, payload, length);

            if (error) {
                LOG_E(TAG, "Failed to parse JSON: %s", error.c_str());
                return;
            }

            const char *event_type = doc["event_type"];
            LOG_I(TAG, "Received event: %s", event_type);

            if (strcmp(event_type, "start_recording") == 0) {
                eventDispatcher->dispatchEvent({RECORD_START, "", 0});
            } else if (strcmp(event_type, "stop_recording") == 0) {
                eventDispatcher->dispatchEvent({RECORD_STOP, "", 0});
            } else if (strcmp(event_type, "start_playing") == 0) {
                eventDispatcher->dispatchEvent({PLAYBACK_START, "", 0});
            } else if (strcmp(event_type, "stop_playing") == 0) {
                eventDispatcher->dispatchEvent({PLAYBACK_STOP, "", 0});
            }
            break;
        }
        case WStype_BIN: {
        }
            Event event = {AUDIO_DATA_RECEIVED, std::string(reinterpret_cast<char *>(payload), length), length};
            eventDispatcher->dispatchEvent(event);
            break;
    }
}

void NetworkManager::sendInitMessage() {
    LOG_I(TAG, "Sent init message");
    webSocket.sendTXT(R"({"event_type":"init","data":{"device":"esp_s3"}})");
}

void NetworkManager::sendAudioChunk(const uint8_t *data, size_t len) {
    webSocket.sendBIN(data, len);
    LOG_I(TAG, "Sent audio chunk");
}

void NetworkManager::sendEvent(const char *eventType, const JsonObject &data) {
    StaticJsonDocument<256> doc;
    doc["event_type"] = eventType;
    doc["data"] = data;

    char buffer[256];
    size_t length = serializeJson(doc, buffer);

    webSocket.sendTXT(buffer, length);
    LOG_I(TAG, "Sent event: %s", eventType);
}
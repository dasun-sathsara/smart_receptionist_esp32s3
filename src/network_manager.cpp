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
    } else {
        LOG_E(TAG, "Failed to connect to WiFi network");
        return;
    }

    webSocket.begin(WS_SERVER, WS_PORT);
    webSocket.onEvent(webSocketEvent);

    xTaskCreate(NetworkManager::loop, "WiFi Task", 8192, this, 2, nullptr);
}

[[noreturn]] void NetworkManager::loop(void *pvParameters) {
    while (true) {
        webSocket.loop();
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

void NetworkManager::webSocketEvent(WStype_t type, uint8_t *payload, size_t length) {
    switch (type) {
        case WStype_DISCONNECTED:
            LOG_I(TAG, "WebSocket disconnected");
            break;
        case WStype_CONNECTED:
            LOG_I(TAG, "WebSocket connected");
            eventDispatcher->dispatchEvent({WS_CONNECTED, ""});
            break;
        case WStype_TEXT: {
            StaticJsonDocument<256> doc;
            DeserializationError error = deserializeJson(doc, payload, length);

            if (error) {
                LOG_E(TAG, "Failed to parse JSON: %s", error.c_str());
                return;
            }

            const char *event_type = doc["event_type"];
            if (event_type == nullptr) {
                LOG_E(TAG, "Invalid JSON: missing event_type");
                return;
            }

            LOG_I(TAG, "Received event: %s", event_type);

            if (strcmp(event_type, "audio") == 0) {
                JsonObject data = doc["data"];
                const char *action = data["action"];

                eventDispatcher->dispatchEvent({CMD_TG_AUDIO, action});

            } else if (strcmp(event_type, "change_state") == 0) {
                JsonObject data = doc["data"];
                if (!data.isNull()) {
                    String dataString;
                    serializeJson(data, dataString);
                    eventDispatcher->dispatchEvent({CMD_CHANGE_STATE, dataString.c_str()});
                } else {
                    LOG_E(TAG, "Invalid change_state event: missing data");
                }
            } else if (strcmp(event_type, "grant_access") == 0) {
                eventDispatcher->dispatchEvent({CMD_GRANT_ACCESS, ""});
            } else if (strcmp(event_type, "deny_access") == 0) {
                eventDispatcher->dispatchEvent({CMD_DENY_ACCESS, ""});
            } else {
                LOG_W(TAG, "Unknown event type: %s", event_type);
            }
            break;
        }
        case WStype_BIN: {
            Event event = {AUDIO_DATA_RECEIVED, std::string(reinterpret_cast<char *>(payload), length), length};
            eventDispatcher->dispatchEvent(event);
            break;
        }
        default:
            LOG_W(TAG, "Unhandled WebSocket event type: %d", type);
            break;
    }
}

void NetworkManager::sendInitMessage() {
    LOG_I(TAG, "Sent init message");
    webSocket.sendTXT(R"({"event_type":"init","data":{"device":"esp_s3"}})");
}

void NetworkManager::sendAudioChunk(const uint8_t *data, size_t len) {
    webSocket.sendBIN(data, len);
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
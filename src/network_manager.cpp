#include "network_manager.h"
#include "config.h"
#include "audio.h"
#include <ArduinoJson.h>
#include <WiFi.h>
#include "logger.h"
#include <esp_system.h>

static const char *TAG = "NetworkManager";


EventDispatcher *NetworkManager::eventDispatcher = nullptr;
WebSocketsClient NetworkManager::webSocket;

const char *WS_SERVER = "192.168.17.218";


void NetworkManager::begin(EventDispatcher &dispatcher) {
    eventDispatcher = &dispatcher;
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    int connectionAttempts = 0;
    while (WiFiClass::status() != WL_CONNECTED && connectionAttempts < 3) {
        vTaskDelay(pdMS_TO_TICKS(500));
        connectionAttempts++;
    }
    if (WiFiClass::status() == WL_CONNECTED) {
        LOG_I(TAG, "Connected to WiFi network");
    } else {
        LOG_E(TAG, "Failed to connect to WiFi network");
    }
    webSocket.begin(WS_SERVER, WS_PORT);
    webSocket.onEvent(webSocketEvent);
    webSocket.enableHeartbeat(15000, 3000, 2);
    xTaskCreate(NetworkManager::loop, "WiFi Task", 8192, this, 2, nullptr);
    xTaskCreate(NetworkManager::reconnectTask, "WiFi Reconnect Task", 2048, this, 1, nullptr);
}

void NetworkManager::changeWebSocketServer(const char *newServer) {
    webSocket.disconnect();
    WS_SERVER = newServer;
    webSocket.begin(WS_SERVER, WS_PORT);
    LOG_I(TAG, "WebSocket server changed to: %s", WS_SERVER);
}

[[noreturn]] void NetworkManager::reconnectTask(void *pvParameters) {
    while (true) {
        if (WiFiClass::status() != WL_CONNECTED) {
            LOG_I(TAG, "Reconnecting to WiFi network...");
            WiFi.reconnect();
            int connectionAttempts = 0;
            while (WiFiClass::status() != WL_CONNECTED && connectionAttempts < 3) {
                vTaskDelay(pdMS_TO_TICKS(500));
                connectionAttempts++;
            }
            if (WiFiClass::status() == WL_CONNECTED) {
                LOG_I(TAG, "Reconnected to WiFi network");
            } else {
                LOG_E(TAG, "Failed to reconnect to WiFi network");
            }
        }
        vTaskDelay(pdMS_TO_TICKS(30000)); // Check every 30 seconds
    }
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
            vTaskDelay(2000);
            webSocket.sendTXT(R"({"event_type":"init","data":{"device":"esp_s3"}})");
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
                LOG_I(TAG, "Received grant access command");
                eventDispatcher->dispatchEvent({CMD_GRANT_ACCESS, ""});
            } else if (strcmp(event_type, "deny_access") == 0) {
                LOG_I(TAG, "Received deny access command");
                eventDispatcher->dispatchEvent({CMD_DENY_ACCESS, ""});
            } else if (strcmp(event_type, "reset_device") == 0) {
                LOG_I(TAG, "Received reset command. Restarting ESP32...");
                vTaskDelay(pdMS_TO_TICKS(1000));
                esp_restart();
            } else if (strcmp(event_type, "motion_enable") == 0) {
                eventDispatcher->dispatchEvent({MOTION_ENABLE, ""});
            } else if (strcmp(event_type, "enroll_fingerprint") == 0) {
                JsonObject data = doc["data"];
                if (!data.isNull()) {
                    String dataString;
                    serializeJson(data, dataString);
                    eventDispatcher->dispatchEvent({CMD_ENROLL_FINGERPRINT, dataString.c_str()});
                } else {
                    LOG_E(TAG, "Invalid enroll_fingerprint event: missing data");
                }
            } else if (strcmp(event_type, "change_server") == 0) {
                const char *newServer = doc["data"]["server"];
                if (newServer) {
                    changeWebSocketServer(newServer);
                } else {
                    LOG_E(TAG, "Invalid change_server event: missing server address");
                }
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
        case WStype_PING:
            LOG_I(TAG, "Received PING");
            break;

        case WStype_PONG:
            LOG_I(TAG, "Received PONG");
            break;
        default:
            LOG_W(TAG, "Unhandled WebSocket event type: %d", type);
            break;
    }
}

void NetworkManager::sendAudioChunk(const uint8_t *data, size_t len) {
    if (webSocket.isConnected()) {
        // Allocate a new buffer with space for the prefix
        size_t totalLen = len + 6; // 6 is the length of "AUDIO:"
        auto *buffer = new uint8_t[totalLen];

        // Copy the prefix and the audio data
        memcpy(buffer, "AUDIO:", 6);
        memcpy(buffer + 6, data, len);

        // Send the prefixed data
        webSocket.sendBIN(buffer, totalLen);

        delete[] buffer;
    } else {
        LOG_W(TAG, "WebSocket not connected. Cannot send audio chunk.");
    }
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

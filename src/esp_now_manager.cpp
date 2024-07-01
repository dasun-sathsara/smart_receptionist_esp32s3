#include "esp_now_manager.h"
#include "logger.h"

static const char *TAG = "ESPNow";

esp_now_peer_info_t ESPNow::peerInfo;
EventDispatcher *ESPNow::eventDispatcher = nullptr;
const uint8_t ESPNow::broadcastAddress[] = {0x34, 0x98, 0x7A, 0xB6, 0x8E, 0x88};

void ESPNow::begin(EventDispatcher &dispatcher) {
    eventDispatcher = &dispatcher;

    if (esp_now_init() != ESP_OK) {
        LOG_E(TAG, "Error initializing ESP-NOW");
        return;
    }

    esp_now_register_recv_cb(onDataReceived);
    esp_now_register_send_cb(onDataSent);

    memcpy(peerInfo.peer_addr, broadcastAddress, 6);
    peerInfo.channel = ESPNOW_CHANNEL;
    peerInfo.encrypt = false;

    if (esp_now_add_peer(&peerInfo) != ESP_OK) {
        LOG_E(TAG, "Failed to add peer");
        return;
    }

    LOG_I(TAG, "ESP-NOW initialized successfully");
}

void ESPNow::sendCommand(const char *command) {
    esp_err_t result = esp_now_send(broadcastAddress, (uint8_t *) command, strlen(command));
    if (result != ESP_OK) {
        LOG_E(TAG, "Failed to send data: %d", result);
    } else {
        LOG_I(TAG, "Command sent successfully: %s", command);
    }
}

void ESPNow::onDataReceived(const uint8_t *mac, const uint8_t *data, int len) {
    char *receivedData = (char *) malloc(len + 1);
    memcpy(receivedData, data, len);
    receivedData[len] = '\0';

    LOG_I(TAG, "Received data: %s", receivedData);
    eventDispatcher->dispatchEvent({ESPNOW_DATA_RECEIVED, std::string(receivedData), static_cast<size_t>(len)});

    free(receivedData);
}

void ESPNow::onDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
    LOG_I(TAG, "Last Packet Send Status: %s", status == ESP_NOW_SEND_SUCCESS ? "Delivery Success" : "Delivery Fail");
}
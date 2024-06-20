#include "fingerprint.h"
#include "config.h"
#include "logger.h"

static const char *TAG = "FINGERPRINT";

FingerprintHandler::FingerprintHandler() : fingerprint(Adafruit_Fingerprint(&Serial2)) {
    Serial2.begin(FINGERPRINT_BAUDRATE_57600, SERIAL_8N1, FINGERPRINT_RX, FINGERPRINT_TX);
}

void FingerprintHandler::begin(EventDispatcher &dispatcher) {
    eventDispatcher = &dispatcher;

    if (fingerprint.verifyPassword()) {
        LOG_I(TAG, "Found fingerprint sensor!");
    } else {
        LOG_E(TAG, "Did not find fingerprint sensor :(");
    }

    fingerprint.getTemplateCount();

    if (fingerprint.templateCount == 0) {
        LOG_E(TAG, "No fingerprint templates found. Please enroll a fingerprint first.");
    } else {
        LOG_I(TAG, "Fingerprint templates found: %d", fingerprint.templateCount);
    }

    xTaskCreate(fingerprintTask, "Fingerprint Task", 4096, this, 1, nullptr);
}

void FingerprintHandler::fingerprintTask(void *parameter) {
    auto *handler = static_cast<FingerprintHandler *>(parameter);

    while (true) {
        uint8_t p = handler->fingerprint.getImage();
        if (p == FINGERPRINT_OK) {
            LOG_I(TAG, "Image taken");
            p = handler->fingerprint.image2Tz();
            if (p == FINGERPRINT_OK) {
                LOG_I(TAG, "Image converted to template");
                p = handler->fingerprint.fingerFastSearch();
                if (p == FINGERPRINT_OK) {
                    LOG_I(TAG, "Finger found!");
                    handler->eventDispatcher->dispatchEvent({EVENT_FINGERPRINT_MATCH, ""});
                } else if (p == FINGERPRINT_NOTFOUND) {
                    LOG_I(TAG, "No match found");
                    handler->eventDispatcher->dispatchEvent({EVENT_FINGERPRINT_NO_MATCH, ""});
                } else {
                    LOG_E(TAG, "Finger search error: %d", p);
                }
            } else {
                LOG_E(TAG, "Image conversion error: %d", p);
            }
        } else {
            LOG_E(TAG, "Image capture error: %d", p);
        }
        vTaskDelay(100);
    }
}


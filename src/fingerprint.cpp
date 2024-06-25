#include "fingerprint.h"
#include "config.h"
#include "logger.h"

static const char *TAG = "FINGERPRINT";

FingerprintHandler::FingerprintHandler(const HardwareSerial &serial) : mySerial(serial), fingerprint(&mySerial) {
    mySerial.begin(57600, SERIAL_8N1, FINGERPRINT_RX, FINGERPRINT_TX);
    fingerprint.begin(57600);
}

void FingerprintHandler::begin(EventDispatcher &dispatcher) {
    eventDispatcher = &dispatcher;

    if (fingerprint.verifyPassword()) {
        LOG_I(TAG, "Found fingerprint sensor!");
        fingerprint.getTemplateCount();
        if (fingerprint.templateCount == 0) {
            LOG_E(TAG, "No fingerprint templates found. Please enroll a fingerprint first.");
        } else {
            LOG_I(TAG, "Fingerprint templates found: %d", fingerprint.templateCount);
        }
    } else {
        LOG_E(TAG, "Did not find fingerprint sensor :(");
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
                    handler->eventDispatcher->dispatchEvent({FINGERPRINT_MATCHED, ""});
                } else if (p == FINGERPRINT_NOTFOUND) {
                    LOG_I(TAG, "No match found");
                    handler->eventDispatcher->dispatchEvent({FINGERPRINT_NO_MATCH, ""});
                } else {
                    LOG_E(TAG, "Finger search error: %d", p);
                }
            } else {
                continue;
            }
        } else {
            continue;
        }
        vTaskDelay(1000);
    }
}





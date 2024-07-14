#include "fingerprint.h"
#include "config.h"
#include "logger.h"

static const char *TAG = "FINGERPRINT";

EventDispatcher *FingerprintHandler::eventDispatcher = nullptr;

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

        if (!handler->sensorEnabled) {
            vTaskDelay(pdMS_TO_TICKS(3000)); // Check every three second if sensor should be enabled
            continue;
        }

        if (handler->isEnrolling) {
            uint8_t result = handler->getFingerprintEnroll();
            if (result == FINGERPRINT_OK) {
                handler->isEnrolling = false;
            } else {
                handler->eventDispatcher->dispatchEvent({FINGERPRINT_ENROLL_FAILED, ""});
                handler->isEnrolling = false;
            }
            continue;
        }

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

void FingerprintHandler::startEnrollment(uint8_t id) {
    isEnrolling = true;
    enrollId = id;
    LOG_I(TAG, "Starting fingerprint enrollment for ID %d", id);
    eventDispatcher->dispatchEvent({PLACE_FINGER, ""});
}

uint8_t FingerprintHandler::getFingerprintEnroll() {
    int p = -1;
    LOG_I(TAG, "Waiting for valid finger to enroll as #%d", enrollId);
    while (p != FINGERPRINT_OK) {
        p = fingerprint.getImage();
        switch (p) {
            case FINGERPRINT_OK:
                LOG_I(TAG, "Image taken");
                break;
            case FINGERPRINT_NOFINGER:
                LOG_D(TAG, ".");
                break;
            case FINGERPRINT_PACKETRECIEVEERR:
                LOG_E(TAG, "Communication error");
                break;
            case FINGERPRINT_IMAGEFAIL:
                LOG_E(TAG, "Imaging error");
                break;
            default:
                LOG_E(TAG, "Unknown error");
                break;
        }
        vTaskDelay(pdMS_TO_TICKS(100));
    }

    p = fingerprint.image2Tz(1);
    switch (p) {
        case FINGERPRINT_OK:
            LOG_I(TAG, "Image converted");
            break;
        case FINGERPRINT_IMAGEMESS:
            LOG_E(TAG, "Image too messy");
            return p;
        case FINGERPRINT_PACKETRECIEVEERR:
            LOG_E(TAG, "Communication error");
            return p;
        case FINGERPRINT_FEATUREFAIL:
        case FINGERPRINT_INVALIDIMAGE:
            LOG_E(TAG, "Could not find fingerprint features");
            return p;
        default:
            LOG_E(TAG, "Unknown error");
            return p;
    }

    eventDispatcher->dispatchEvent({REMOVE_FINGER, ""});
    vTaskDelay(pdMS_TO_TICKS(2000));

    p = 0;
    while (p != FINGERPRINT_NOFINGER) {
        p = fingerprint.getImage();
        vTaskDelay(pdMS_TO_TICKS(100));
    }

    p = -1;
    eventDispatcher->dispatchEvent({PLACE_FINGER_AGAIN, ""});
    while (p != FINGERPRINT_OK) {
        p = fingerprint.getImage();
        switch (p) {
            case FINGERPRINT_OK:
                LOG_I(TAG, "Image taken");
                break;
            case FINGERPRINT_NOFINGER:
                LOG_D(TAG, ".");
                break;
            case FINGERPRINT_PACKETRECIEVEERR:
                LOG_E(TAG, "Communication error");
                break;
            case FINGERPRINT_IMAGEFAIL:
                LOG_E(TAG, "Imaging error");
                break;
            default:
                LOG_E(TAG, "Unknown error");
                break;
        }
        vTaskDelay(pdMS_TO_TICKS(100));
    }

    p = fingerprint.image2Tz(2);
    switch (p) {
        case FINGERPRINT_OK:
            LOG_I(TAG, "Image converted");
            break;
        case FINGERPRINT_IMAGEMESS:
            LOG_E(TAG, "Image too messy");
            return p;
        case FINGERPRINT_PACKETRECIEVEERR:
            LOG_E(TAG, "Communication error");
            return p;
        case FINGERPRINT_FEATUREFAIL:
        case FINGERPRINT_INVALIDIMAGE:
            LOG_E(TAG, "Could not find fingerprint features");
            return p;
        default:
            LOG_E(TAG, "Unknown error");
            return p;
    }

    LOG_I(TAG, "Creating model for #%d", enrollId);
    p = fingerprint.createModel();
    if (p == FINGERPRINT_OK) {
        LOG_I(TAG, "Prints matched!");
    } else if (p == FINGERPRINT_PACKETRECIEVEERR) {
        LOG_E(TAG, "Communication error");
        return p;
    } else if (p == FINGERPRINT_ENROLLMISMATCH) {
        LOG_E(TAG, "Fingerprints did not match");
        return p;
    } else {
        LOG_E(TAG, "Unknown error");
        return p;
    }

    p = fingerprint.storeModel(enrollId);
    if (p == FINGERPRINT_OK) {
        LOG_I(TAG, "Stored!");
        eventDispatcher->dispatchEvent({FINGERPRINT_ENROLLED, ""});
    } else if (p == FINGERPRINT_PACKETRECIEVEERR) {
        LOG_E(TAG, "Communication error");
        return p;
    } else if (p == FINGERPRINT_BADLOCATION) {
        LOG_E(TAG, "Could not store in that location");
        return p;
    } else if (p == FINGERPRINT_FLASHERR) {
        LOG_E(TAG, "Error writing to flash");
        return p;
    } else {
        LOG_E(TAG, "Unknown error");
        return p;
    }

    return FINGERPRINT_OK;
}

void FingerprintHandler::enableSensor() {
    sensorEnabled = true;
    LOG_I(TAG, "Fingerprint sensor enabled");
}

void FingerprintHandler::disableSensor() {
    sensorEnabled = false;
    LOG_I(TAG, "Fingerprint sensor disabled");
}


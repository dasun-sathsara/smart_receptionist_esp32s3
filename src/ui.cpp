#include "ui.h"
#include "config.h"
#include "logger.h"
#include <Wire.h>

static const char *TAG = "UI";

// Define keypad size
const byte ROWS = 4;
const byte COLS = 3;

char hexaKeys[ROWS][COLS] = {
        {'1', '2', '3',},
        {'4', '5', '6',},
        {'7', '8', '9',},
        {'*', '0', '#',}
};

byte rowPins[ROWS] = {4, 5, 6, 7};
byte colPins[COLS] = {15, 16, 17};

const char *UI::menuItems[] = {"NOTIFY OWNER", "ENTER PASSWORD", "RECORD AUDIO", "PLAY RECEIVED AUDIO"};
const int UI::menuItemCount = sizeof(menuItems) / sizeof(menuItems[0]);
const char UI::correctPassword[] = "1234";

EventDispatcher *UI::eventDispatcher = nullptr;

UI::UI() : u8g2(U8G2_R0, U8X8_PIN_NONE),
           keypad(makeKeymap(hexaKeys), rowPins, colPins, ROWS, COLS),
           currentMenuItem(0),
           notificationDisplayed(false),
           enteringPassword(false),
           passwordCorrect(false),
           passwordChecked(false),
           recordingAudio(false),
           playingAudio(false),
           passwordIndex(0) {

    memset(enteredPassword, 0, sizeof(enteredPassword));
}

void UI::begin(EventDispatcher &dispatcher) {
    eventDispatcher = &dispatcher;
    Wire.begin(SDA_PIN, SCL_PIN);
    u8g2.setBusClock(400000);  // Set I2C clock speed to 400kHz
    u8g2.setI2CAddress(I2C_ADDRESS * 2);
    u8g2.begin();
    xTaskCreate(uiTask, "UI Task", 4096, this, 1, nullptr);
    LOG_I(TAG, "UI initialized");
}

void UI::uiTask(void *parameter) {
    UI *ui = static_cast<UI *>(parameter);
    while (true) {
        ui->update();
        vTaskDelay(pdMS_TO_TICKS(500)); // Update every 500ms
    }
}

void UI::update() {
    char key = keypad.getKey();
    if (key) {
        LOG_I(TAG, "Key pressed: %c", key);
        handleKeyPress(key);
    }
    displayMenu();
}

void UI::handleKeyPress(char key) {
    if (enteringPassword) {
        if (key >= '0' && key <= '9' && passwordIndex < 4) {
            enteredPassword[passwordIndex++] = key;
            enteredPassword[passwordIndex] = '\0';
        } else if (key == '*') {
            passwordIndex = 0;
            enteredPassword[0] = '\0';
        } else if (key == '#') {
            passwordCorrect = (strcmp(enteredPassword, correctPassword) == 0);
            passwordChecked = true;
            enteringPassword = false;
            displayPasswordResult(passwordCorrect);
            vTaskDelay(pdMS_TO_TICKS(2000));
            passwordChecked = false;
            passwordIndex = 0;
            enteredPassword[0] = '\0';
        }
    } else if (recordingAudio || playingAudio) {
        if (key == '1') {
            recordingAudio = false;
            playingAudio = false;
        }
    } else {
        switch (key) {
            case '2':
                currentMenuItem = (currentMenuItem - 1 + menuItemCount) % menuItemCount;
                notificationDisplayed = false;
                break;
            case '8':
                currentMenuItem = (currentMenuItem + 1) % menuItemCount;
                notificationDisplayed = false;
                break;
            case '5':
                if (currentMenuItem == 0) {
                    notificationDisplayed = true;
//                    eventDispatcher->dispatchEvent({EN_CMD_CAPTURE_IMAGE, ""});
                } else if (currentMenuItem == 1) {
                    enteringPassword = true;
                } else if (currentMenuItem == 2) {
                    recordingAudio = true;
//                    eventDispatcher->dispatchEvent({RECORD_START, ""});
                } else if (currentMenuItem == 3) {
                    playingAudio = true;
//                    eventDispatcher->dispatchEvent({PLAYBACK_START, ""});
                }
                break;
        }
    }
}

void UI::displayMenu() {
    u8g2.clearBuffer();

    if (notificationDisplayed && currentMenuItem == 0) {
        u8g2.setFont(u8g2_font_profont17_tr);
        u8g2.drawStr(15, 32, "OWNER HAS BEEN");
        u8g2.drawStr(35, 47, "NOTIFIED");
    } else if (enteringPassword && currentMenuItem == 1) {
        u8g2.setFont(u8g2_font_profont17_tr);
        u8g2.drawStr(15, 32, "ENTER PASSWORD");
        u8g2.drawStr(35, 47, enteredPassword);
    } else if (recordingAudio && currentMenuItem == 2) {
        displayRecordingMessage();
    } else if (playingAudio && currentMenuItem == 3) {
        displayPlayingMessage();
    } else {
        switch (currentMenuItem) {
            case 0:
                u8g2.setFont(u8g2_font_profont17_tr);
                u8g2.drawXBMP(7, 19, 16, 16, image_notification_bell_bits);
                u8g2.drawStr(33, 25, "NOTIFY");
                u8g2.drawStr(33, 40, "OWNER");
                u8g2.drawLine(120, 8, 120, 54);
                u8g2.drawEllipse(120, 12, 1, 3);
                break;
            case 1:
                u8g2.setFont(u8g2_font_profont17_tr);
                u8g2.drawXBMP(7, 19, 16, 16, image_device_lock_bits);
                u8g2.drawStr(33, 25, "ENTER");
                u8g2.drawStr(33, 40, "PASSWORD");
                u8g2.drawLine(120, 8, 120, 54);
                u8g2.drawEllipse(120, 21, 1, 3);
                break;
            case 2:
                u8g2.setFont(u8g2_font_profont17_tr);
                u8g2.drawXBMP(7, 19, 16, 16, image_microphone_bits);
                u8g2.drawStr(33, 25, "RECORD");
                u8g2.drawStr(33, 40, "AUDIO");
                u8g2.drawLine(120, 8, 120, 54);
                u8g2.drawEllipse(120, 34, 1, 3);
                break;
            case 3:
                u8g2.setFont(u8g2_font_profont17_tr);
                u8g2.drawXBMP(7, 19, 16, 16, image_volume_loud_bits);
                u8g2.drawStr(33, 20, "PLAY");
                u8g2.drawStr(33, 36, "RECEIVED");
                u8g2.drawLine(120, 8, 120, 54);
                u8g2.drawEllipse(120, 48, 1, 3);
                u8g2.drawStr(33, 52, "AUDIO");
                break;
        }
    }

    u8g2.sendBuffer();
}

void UI::displayPasswordResult(bool correct) {
    u8g2.clearBuffer();
    u8g2.setFont(u8g2_font_profont17_tr);
    if (correct) {
        u8g2.drawStr(40, 32, "PASSWORD");
        u8g2.drawStr(50, 47, "CORRECT");
        eventDispatcher->dispatchEvent({PASSWORD_VALIDATED, ""});
    } else {
        u8g2.drawStr(40, 32, "PASSWORD");
        u8g2.drawStr(50, 47, "WRONG");
    }
    u8g2.sendBuffer();
}

void UI::displayRecordingMessage() {
    u8g2.clearBuffer();
    u8g2.setFont(u8g2_font_profont17_tr);
    u8g2.drawStr(2, 32, "RECORDING...");
    u8g2.drawStr(2, 47, "PRESS 1 TO STOP");
    u8g2.sendBuffer();
}

void UI::displayPlayingMessage() {
    u8g2.clearBuffer();
    u8g2.setFont(u8g2_font_profont17_tr);
    u8g2.drawStr(2, 32, "PLAYING...");
    u8g2.drawStr(2, 47, "PRESS 1 TO STOP");
    u8g2.sendBuffer();
}

void UI::displayAccessGranted() {
    u8g2.clearBuffer();
    u8g2.setFont(u8g2_font_profont17_tr);
    u8g2.drawStr(40, 32, "ACCESS");
    u8g2.drawStr(50, 47, "GRANTED");
    u8g2.sendBuffer();
}

void UI::displayAccessDenied() {
    u8g2.clearBuffer();
    u8g2.setFont(u8g2_font_profont17_tr);
    u8g2.drawStr(40, 32, "ACCESS");
    u8g2.drawStr(50, 47, "DENIED");
    u8g2.sendBuffer();
}

void UI::displayMotionDetected() {
    u8g2.clearBuffer();
    u8g2.setFont(u8g2_font_profont17_tr);
    u8g2.drawStr(40, 32, "MOTION");
    u8g2.drawStr(50, 47, "DETECTED");
    u8g2.sendBuffer();
}

void UI::displayFingerprintMatched() {
    u8g2.clearBuffer();
    u8g2.setFont(u8g2_font_profont17_tr);
    u8g2.drawStr(40, 32, "FINGERPRINT");
    u8g2.drawStr(50, 47, "MATCHED");
    u8g2.sendBuffer();
}

void UI::displayFingerprintNoMatch() {
    u8g2.clearBuffer();
    u8g2.setFont(u8g2_font_profont17_tr);
    u8g2.drawStr(40, 32, "FINGERPRINT");
    u8g2.drawStr(50, 47, "NO MATCH");
    u8g2.sendBuffer();
}


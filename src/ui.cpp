#include "ui.h"
#include "config.h"
#include "logger.h"
#include <Wire.h>
#include "icons.h"

static const char *TAG = "UI";

const int UI::menuItemCount = 4;
const char UI::correctPassword[] = "1234";
EventDispatcher *UI::eventDispatcher = nullptr;

const byte ROWS = 4;
const byte COLS = 3;
char hexaKeys[ROWS][COLS] = {
        {'1', '2', '3'},
        {'4', '5', '6'},
        {'7', '8', '9'},
        {'*', '0', '#'}
};

byte rowPins[ROWS] = {ROW1, ROW2, ROW3, ROW4};
byte colPins[COLS] = {COL1, COL2, COL3};

UI::UI() : u8g2(U8G2_R0, U8X8_PIN_NONE),
           keypad(makeKeymap(hexaKeys), rowPins, colPins, ROWS, COLS),
           currentState(UIState::WELCOME),
           currentMenuItem(0),
           passwordIndex(0),
           lastStateChangeTime(0),
           enteringPassword(false) {
    memset(enteredPassword, 0, sizeof(enteredPassword));
}

void UI::begin(EventDispatcher &dispatcher) {
    eventDispatcher = &dispatcher;
    Wire.begin(SDA_PIN, SCL_PIN);
    u8g2.setBusClock(400000);
    u8g2.setI2CAddress(I2C_ADDRESS * 2);
    u8g2.begin();


    xTaskCreate(uiTask, "UI Task", 4096, this, 1, nullptr);
    LOG_I(TAG, "UI initialized");
}

[[noreturn]] void UI::uiTask(void *parameter) {
    UI *ui = static_cast<UI *>(parameter);
    while (true) {
        ui->update();
        vTaskDelay(pdMS_TO_TICKS(40));
    }
}

void UI::enableDisplay() {
    displayEnabled = true;
    u8g2.setPowerSave(0); // Turn on display
    LOG_I(TAG, "Display enabled");
}

void UI::disableDisplay() {
    displayEnabled = false;
    u8g2.setPowerSave(1); // Turn off display
    LOG_I(TAG, "Display disabled");
}

void UI::update() {
    if (!displayEnabled) return;

    char key = keypad.getKey();
    if (key) {
        LOG_I(TAG, "Key pressed: %c", key);
        handleKeyPress(key);
    }

    unsigned long currentTime = millis();

    // Check for temporary state expiration
    if (temporaryState && currentTime >= stateEndTime) {
        setState(originalState);
        temporaryState = false;
        LOG_I(TAG, "UI state changed back to %d", static_cast<int>(originalState));
    }

    // Check for timeout to return to welcome screen
    if (currentTime - lastStateChangeTime > STATE_TIMEOUT &&
        currentState != UIState::WELCOME &&
        currentState != UIState::RECORDING_AUDIO &&
        currentState != UIState::PLAYING_AUDIO) {
        setState(UIState::WELCOME);
    }

    if (!enteringPassword) {
        displayCurrentState();
    }
}

void UI::setState(UIState newState) {
    currentState = newState;
    lastStateChangeTime = millis();
    LOG_I(TAG, "UI state changed to: %d", static_cast<int>(newState));
}

void UI::setStateFor(int seconds, UIState newState) {
    setState(newState);
    stateEndTime = millis() + (seconds * 1000);
    temporaryState = true;
    originalState = currentState;
    LOG_I(TAG, "UI state changed to %d for %d seconds", static_cast<int>(newState), seconds);
}

void UI::handleKeyPress(char key) {
    switch (currentState) {
        case UIState::WELCOME:
            // Pressing any key will take us to the menu
            setState(UIState::MENU_NOTIFY_OWNER);
            break;
        case UIState::MENU_NOTIFY_OWNER:
        case UIState::MENU_ENTER_PASSWORD:
        case UIState::MENU_RECORD_AUDIO:
        case UIState::MENU_PLAY_AUDIO:
            handleMenuKeyPress(key);
            break;
        case UIState::ENTER_PASSWORD:
            enteringPassword = true;
            handlePasswordKeyPress(key);
            break;
        case UIState::RECORDING_AUDIO:
            if (key == '1') {
                eventDispatcher->dispatchEvent({CMD_ESP_AUDIO, "stop_recording"});
                vTaskDelay(pdMS_TO_TICKS(1000)); // Wait for one second
                setState(UIState::MENU_NOTIFY_OWNER); // Go back to the menu
            }
            break;
        case UIState::PLAYING_AUDIO:
            if (key == '1') {
                eventDispatcher->dispatchEvent({CMD_ESP_AUDIO, "stop_playing"});
                vTaskDelay(pdMS_TO_TICKS(1000)); // Wait for one second
                setState(UIState::MENU_NOTIFY_OWNER); // Go back to the menu
            }
            break;
        case UIState::PASSWORD_CORRECT:
        case UIState::PASSWORD_INCORRECT:
        case UIState::OWNER_NOTIFIED:
        case UIState::ACCESS_GRANTED:
        case UIState::ACCESS_DENIED:
        case UIState::MOTION_DETECTED:
        case UIState::FINGERPRINT_MATCHED:
        case UIState::FINGERPRINT_NO_MATCH:
        case UIState::SAY_CHEESE:
            // Pressing any key will take us back to the menu
            setState(UIState::MENU_NOTIFY_OWNER);
            break;
        default:
            break;
    }
}

void UI::handleMenuKeyPress(char key) {
    switch (key) {
        case '2':
            currentMenuItem = (currentMenuItem - 1 + menuItemCount) % menuItemCount;
            setState(static_cast<UIState>(static_cast<int>(UIState::MENU_NOTIFY_OWNER) + currentMenuItem));
            break;
        case '8':
            currentMenuItem = (currentMenuItem + 1) % menuItemCount;
            setState(static_cast<UIState>(static_cast<int>(UIState::MENU_NOTIFY_OWNER) + currentMenuItem));
            break;
        case '5':
            switch (currentState) {
                case UIState::MENU_NOTIFY_OWNER:
                    setState(UIState::OWNER_NOTIFIED);
                    eventDispatcher->dispatchEvent({PERSON_DETECTED, ""});
                    break;
                case UIState::MENU_ENTER_PASSWORD:
                    setState(UIState::ENTER_PASSWORD);
                    break;
                case UIState::MENU_RECORD_AUDIO:
                    setState(UIState::RECORDING_AUDIO);
                    eventDispatcher->dispatchEvent({CMD_ESP_AUDIO, "start_recording"});
                    break;
                case UIState::MENU_PLAY_AUDIO:
                    setState(UIState::PLAYING_AUDIO);
                    eventDispatcher->dispatchEvent({CMD_ESP_AUDIO, "start_playing"});
                    break;
            }
            break;
        default:
            LOG_I(TAG, "Invalid key: %c", key);
            break;
    }
}

void UI::handlePasswordKeyPress(char key) {
    if (key >= '0' && key <= '6' && passwordIndex < 4) {
        enteredPassword[passwordIndex++] = key;
        enteredPassword[passwordIndex] = '\0';
        displayPasswordAsAsterisks(enteredPassword);
    } else if (key == '7') {
        if (passwordIndex > 0) {
            enteredPassword[--passwordIndex] = '\0';
            displayPasswordAsAsterisks(enteredPassword);
        }
    } else if (key == '9') {
        enteringPassword = false;
        bool passwordCorrect = (strcmp(enteredPassword, correctPassword) == 0);
        eventDispatcher->dispatchEvent({passwordCorrect ? PASSWORD_VALID : PASSWORD_INVALID, ""});
        setState(passwordCorrect ? UIState::PASSWORD_CORRECT : UIState::PASSWORD_INCORRECT);
        passwordIndex = 0;
        memset(enteredPassword, 0, sizeof(enteredPassword));
    }
}

void UI::displayCurrentState() {
    u8g2.clearBuffer();
    u8g2.setFontMode(1);
    u8g2.setBitmapMode(1);

    switch (currentState) {
        case UIState::MENU_NOTIFY_OWNER:
            u8g2.drawLine(116, 7, 116, 55);
            u8g2.drawBox(113, 9, 7, 11);
            u8g2.drawFrame(1, 1, 125, 61);
            u8g2.setFont(u8g2_font_profont17_tr);
            u8g2.drawStr(42, 45, "OWNER");
            u8g2.drawStr(42, 29, "NOTIFY");
            u8g2.setDrawColor(2);
            u8g2.drawXBMP(14, 24, 16, 16, image_notification_bell_bits);
            break;
        case UIState::MENU_ENTER_PASSWORD:
            u8g2.drawXBMP(11, 24, 16, 16, image_device_lock_bits);
            u8g2.drawLine(116, 7, 116, 55);
            u8g2.drawBox(113, 18, 7, 11);
            u8g2.drawFrame(1, 1, 125, 61);
            u8g2.setFont(u8g2_font_profont17_tr);
            u8g2.drawStr(34, 28, "ENTER");
            u8g2.drawStr(34, 45, "PASSWORD");
            break;
        case UIState::MENU_RECORD_AUDIO:
            u8g2.drawXBMP(11, 24, 16, 16, image_microphone_bits);
            u8g2.drawLine(116, 7, 116, 55);
            u8g2.drawBox(113, 32, 7, 11);
            u8g2.drawFrame(1, 1, 125, 61);
            u8g2.setFont(u8g2_font_profont17_tr);
            u8g2.drawStr(39, 28, "RECORD");
            u8g2.drawStr(39, 45, "AUDIO");
            break;
        case UIState::MENU_PLAY_AUDIO:
            u8g2.drawLine(116, 7, 116, 55);
            u8g2.drawBox(113, 42, 7, 11);
            u8g2.drawFrame(1, 1, 125, 61);
            u8g2.setFont(u8g2_font_profont17_tr);
            u8g2.drawStr(39, 28, "PLAY");
            u8g2.drawStr(39, 45, "AUDIO");
            u8g2.drawXBMP(9, 24, 20, 16, image_volume_loud_bits);
            break;
        case UIState::OWNER_NOTIFIED:
            u8g2.setFont(u8g2_font_profont17_tr);
            u8g2.drawStr(22, 21, "OWNER HAS");
            u8g2.drawStr(28, 53, "NOTIFIED");
            u8g2.drawStr(46, 37, "BEEN");
            u8g2.drawFrame(1, 1, 125, 61);
            break;
        case UIState::ENTER_PASSWORD:
            u8g2.setFont(u8g2_font_t0_16b_tr);
            u8g2.drawStr(8, 26, "ENTER PASSWORD");
            u8g2.setFont(u8g2_font_profont17_tr);
            u8g2.drawStr(45, 47, "");
            u8g2.drawFrame(1, 1, 125, 61);
            break;
        case UIState::RECORDING_AUDIO:
            u8g2.setFont(u8g2_font_profont17_tr);
            u8g2.drawStr(10, 26, "RECORDING...");
            u8g2.setFont(u8g2_font_profont15_tr);
            u8g2.drawStr(11, 55, "PRESS 1 TO STOP");
            u8g2.drawFrame(1, 1, 126, 37);
            break;
        case UIState::PLAYING_AUDIO:
            u8g2.setFont(u8g2_font_profont17_tr);
            u8g2.drawStr(20, 26, "PLAYING...");
            u8g2.setFont(u8g2_font_profont15_tr);
            u8g2.drawStr(11, 55, "PRESS 1 TO STOP");
            u8g2.drawFrame(1, 1, 126, 37);
            break;
        case UIState::ACCESS_GRANTED:
            u8g2.setFont(u8g2_font_profont17_tr);
            u8g2.drawStr(34, 29, "ACCESS");
            u8g2.drawStr(31, 45, "GRANTED");
            u8g2.drawFrame(1, 1, 125, 61);
            break;
        case UIState::ACCESS_DENIED:
            u8g2.setFont(u8g2_font_profont17_tr);
            u8g2.drawStr(34, 29, "ACCESS");
            u8g2.drawStr(34, 46, "DENIED");
            u8g2.drawFrame(1, 1, 125, 61);
            break;
        case UIState::MOTION_DETECTED:
            u8g2.setFont(u8g2_font_profont17_tr);
            u8g2.drawStr(35, 30, "MOTION");
            u8g2.drawStr(28, 45, "DETECTED");
            u8g2.drawFrame(1, 1, 125, 61);
            break;
        case UIState::FINGERPRINT_MATCHED:
            u8g2.setFont(u8g2_font_profont17_tr);
            u8g2.drawStr(14, 29, "FINGERPRINT");
            u8g2.drawStr(32, 45, "MATCHED");
            u8g2.drawFrame(1, 1, 125, 61);
            break;
        case UIState::FINGERPRINT_NO_MATCH:
            u8g2.setFont(u8g2_font_profont17_tr);
            u8g2.drawStr(14, 29, "FINGERPRINT");
            u8g2.drawStr(27, 45, "NO MATCH");
            u8g2.drawFrame(1, 1, 125, 61);
            break;
        case UIState::PASSWORD_CORRECT:
            u8g2.setDrawColor(2);
            u8g2.setFont(u8g2_font_profont17_tr);
            u8g2.drawStr(31, 34, "CORRECT");
            u8g2.setDrawColor(1);
            u8g2.drawStr(27, 19, "PASSWORD");
            u8g2.drawFrame(1, 1, 125, 61);
            u8g2.drawXBMP(48, 41, 29, 14, image_FaceNormal_bits);
            break;
        case UIState::NO_AUDIO_DATA:
            u8g2.setFontMode(1);
            u8g2.setBitmapMode(1);
            u8g2.setFont(u8g2_font_profont17_tr);
            u8g2.drawStr(28, 29, "NO AUDIO");
            u8g2.drawStr(45, 46, "DATA");
            u8g2.drawFrame(1, 1, 125, 61);
            break;
        case UIState::PASSWORD_INCORRECT:
            u8g2.setDrawColor(2);
            u8g2.setFont(u8g2_font_profont17_tr);
            u8g2.drawStr(23, 36, "INCORRECT");
            u8g2.setDrawColor(1);
            u8g2.drawStr(27, 20, "PASSWORD");
            u8g2.drawFrame(1, 1, 125, 61);
            u8g2.drawXBMP(49, 42, 29, 14, image_FaceNopower_bits);
            break;
        case UIState::SAY_CHEESE:
            u8g2.setDrawColor(2);
            u8g2.setFont(u8g2_font_profont17_tr);
            u8g2.drawStr(17, 46, "CHEESE");
            u8g2.setDrawColor(1);
            u8g2.drawStr(18, 29, "SAY");
            u8g2.drawFrame(1, 1, 125, 61);
            u8g2.drawXBM(88, 23, 18, 18, image_Smile_bits);
            break;
        case UIState::WELCOME:
            u8g2.setDrawColor(2);
            u8g2.setFont(u8g2_font_profont17_tr);
            u8g2.drawStr(10, 40, "RECEPTIONIST");
            u8g2.setDrawColor(1);
            u8g2.drawStr(42, 23, "SMART");
            u8g2.drawFrame(1, 1, 125, 61);
            u8g2.drawEllipse(63, 49, 2, 2);
            u8g2.drawEllipse(55, 49, 2, 2);
            u8g2.drawEllipse(71, 49, 2, 2);
            break;
    }
    u8g2.sendBuffer();
}

void UI::displayPasswordAsAsterisks(char *password) {
    u8g2.clearBuffer();
    u8g2.setFontMode(1);
    u8g2.setBitmapMode(1);
    u8g2.setFont(u8g2_font_t0_16b_tr);
    u8g2.drawStr(8, 26, "ENTER PASSWORD");
    u8g2.setFont(u8g2_font_profont17_tr);
    u8g2.drawStr(45, 47, password);
    u8g2.drawFrame(1, 1, 125, 61);
    u8g2.sendBuffer();
}
#include "ui.h"
#include "config.h"
#include <Keypad.h>
#include <Arduino.h>

// Keypad configuration
// Define keypad size
const byte rows = 4; // Four rows
const byte cols = 3; // Three columns

// Define the key map
char keys[rows][cols] = {
        {'1', '2', '3',},
        {'4', '5', '6',},
        {'7', '8', '9',},
        {'*', '0', '#',}
};

// Connect keypad rows and columns to Arduino pins
byte rowPins[rows] = {12, 14, 27, 26};
byte colPins[cols] = {2, 0, 4};

UI::UI() : keypad(Keypad(makeKeymap(keys), rowPins, colPins, rows, cols)),
           u8g2(U8G2_SSD1306_128X64_NONAME_F_HW_I2C(U8G2_R0, /* reset=*/U8X8_PIN_NONE)) {}

void UI::begin(EventDispatcher &dispatcher) {
    eventDispatcher = &dispatcher;

    u8g2.begin();
    displayMenu();

    keypad.setDebounceTime(50); // Set debounce time (optional)

    xTaskCreate(loopTask, "UI Task", 4096, this, 1, nullptr);
}


void UI::loopTask(void *parameter) {
    while (true) {
        ((UI *) parameter)->loop();
        vTaskDelay(10);
    }
}

void UI::loop() {
    char key = keypad.getKey();
    if (key) {
        if (enteringPassword) {
            if (key >= '0' && key <= '9') {
                if (passwordIndex < 4) {
                    enteredPassword[passwordIndex++] = key;
                    displayMenu();
                }
            } else if (key == '*') {
                // Clear entered password
                passwordIndex = 0;
                enteredPassword[0] = '\0';
                displayMenu();
            } else if (key == '#') {
                // Exit password entry mode
                enteringPassword = false;
                passwordIndex = 0;
                enteredPassword[0] = '\0';
                displayMenu();
            }
        } else {
            switch (key) {
                case '2': // Up
                    currentMenuItem = (currentMenuItem - 1 + 4) % 4;
                    notificationDisplayed = false;
                    displayMenu();
                    break;
                case '8': // Down
                    currentMenuItem = (currentMenuItem + 1) % 4;
                    notificationDisplayed = false;
                    displayMenu();
                    break;
                case '5': // Select
                    if (currentMenuItem == 0) {
                        notificationDisplayed = true;
                    } else if (currentMenuItem == 1) {
                        enteringPassword = true;
                    }
                    displayMenu();
                    break;
            }
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
        u8g2.drawStr(35, 47, "ENTER PASSWORD: ");
        u8g2.drawStr(110, 47, enteredPassword);
    } else {
        switch (currentMenuItem) {
            case 0:
                u8g2.setFont(u8g2_font_profont17_tr);
                u8g2.drawStr(33, 25, "NOTIFY");
                u8g2.drawStr(34, 40, "OWNER");
                u8g2.drawLine(120, 8, 120, 54);
                u8g2.drawXBMP(7, 19, 16, 16, image_notification_bell_bits);
                u8g2.drawEllipse(120, 12, 1, 3);
                break;
            case 1:
                u8g2.drawXBMP(5, 18, 13, 16, image_device_lock_bits);
                u8g2.setFont(u8g2_font_profont17_tr);
                u8g2.drawStr(31, 26, "ENTER");
                u8g2.drawStr(30, 41, "PASSWORD");
                u8g2.drawLine(120, 8, 120, 54);
                u8g2.drawEllipse(120, 21, 1, 3);
                break;
            case 2:
                u8g2.setFont(u8g2_font_profont17_tr);
                u8g2.drawStr(33, 25, "RECORD");
                u8g2.drawStr(34, 40, "AUDIO");
                u8g2.drawLine(120, 8, 120, 54);
                u8g2.drawXBMP(9, 18, 15, 16, image_microphone_bits);
                u8g2.drawEllipse(120, 34, 1, 3);
                break;
            case 3:
                u8g2.setFont(u8g2_font_profont17_tr);
                u8g2.drawStr(33, 20, "PLAY");
                u8g2.drawStr(31, 36, "RECEIVED");
                u8g2.drawLine(120, 8, 120, 54);
                u8g2.drawEllipse(120, 48, 1, 3);
                u8g2.drawXBMP(6, 20, 20, 16, image_volume_loud_bits);
                u8g2.drawStr(31, 52, "AUDIO");
                break;
        }
    }

    u8g2.sendBuffer();
}

void UI::keypadEvent() {
    // This function is required by the Keypad library but is unused in this context.
}


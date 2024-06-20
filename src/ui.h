#ifndef UI_H
#define UI_H

#include "U8g2lib.h"
#include "Keypad.h"
#include "events.h"


class UI {
public:
    UI();

    void begin(EventDispatcher &dispatcher);

    void loop();

private:
    void displayMenu();

    static void loopTask(void *parameter);

    static void keypadEvent();

    U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2;
    Keypad keypad;
    EventDispatcher *eventDispatcher{};

    const char *menuItems[4] = {"NOTIFY OWNER", "ENTER PASSWORD", "RECORD AUDIO", "PLAY RECEIVED AUDIO"};
    int currentMenuItem = 0;
    bool notificationDisplayed = false;
    bool enteringPassword = false;
    char enteredPassword[5] = {0}; // Initialize with null terminator
    int passwordIndex = 0;
};

#endif // UI_H
#ifndef UI_H
#define UI_H

#include <U8g2lib.h>
#include <Keypad.h>
#include "events.h"

enum class UIState {
    MENU_NOTIFY_OWNER,
    MENU_ENTER_PASSWORD,
    MENU_RECORD_AUDIO,
    MENU_PLAY_AUDIO,
    OWNER_NOTIFIED,
    ENTER_PASSWORD,
    RECORDING_AUDIO,
    PLAYING_AUDIO,
    ACCESS_GRANTED,
    ACCESS_DENIED,
    MOTION_DETECTED,
    FINGERPRINT_MATCHED,
    FINGERPRINT_NO_MATCH,
    PASSWORD_CORRECT,
    PASSWORD_INCORRECT,
    SAY_CHEESE,
    WELCOME,
    NO_AUDIO_DATA,
    PLACE_FINGER,
    PLACE_FINGER_AGAIN,
    REMOVE_FINGER,
    FINGERPRINT_ENROLLED,
    FINGERPRINT_ENROLL_FAILED,
    AUDIO_MESSAGE_RECEIVED,
};

class UI {
public:
    UI();

    void begin(EventDispatcher &dispatcher);

    void update();

    void setState(UIState newState);

    void setStateFor(int seconds, UIState newState);

    void enableDisplay();

    void disableDisplay();

private:
    [[noreturn]] static void uiTask(void *parameter);

    void handleKeyPress(char key);

    void displayCurrentState();

    void handleMenuKeyPress(char key);

    void handlePasswordKeyPress(char key);

    U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2;
    bool displayEnabled;
    Keypad keypad;
    UIState currentState;
    char enteredPassword[5]{};
    bool enteringPassword;
    int passwordIndex;
    static const int menuItemCount;
    int currentMenuItem;
    static const char correctPassword[];
    static EventDispatcher *eventDispatcher;
    unsigned long lastStateChangeTime;
    static const unsigned long STATE_TIMEOUT = 30000; // 30 seconds timeout
    void displayPasswordAsAsterisks(char *password);

    TimerHandle_t stateTimer;

    static void stateTimerCallback(TimerHandle_t xTimer);

    UIState scheduledState;
};

#endif // UI_H
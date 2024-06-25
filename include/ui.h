#ifndef UI_H
#define UI_H

#include <U8g2lib.h>
#include <Keypad.h>
#include "events.h"

class UI {
public:
    UI();

    void begin(EventDispatcher &dispatcher);

    void update();

    void displayAccessGranted();

    void displayAccessDenied();

private:
    static void uiTask(void *parameter);

    void displayMenu();

    void handleKeyPress(char key);

    void displayPasswordResult(bool correct);

    void displayRecordingMessage();

    void displayPlayingMessage();

    void displayFingerprintNoMatch();

    void displayMotionDetected();

    void displayFingerprintMatched();


    U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2;
    Keypad keypad;

    static const char *menuItems[];
    static const int menuItemCount;
    int currentMenuItem;
    bool notificationDisplayed;
    bool enteringPassword;
    bool passwordCorrect;
    bool passwordChecked;
    bool recordingAudio;
    bool playingAudio;
    char enteredPassword[5]{};
    int passwordIndex;
    static const char correctPassword[];

    static EventDispatcher *eventDispatcher;

};

#endif // UI_H
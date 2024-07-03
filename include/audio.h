#ifndef AUDIO_H
#define AUDIO_H

#include <Arduino.h>
#include <driver/i2s.h>
#include "freertos/ringbuf.h"
#include "events.h"

class Audio {
public:
    void begin(EventDispatcher &dispatcher);
    static void startRecording();
    static void stopRecording();
    static void startPlayback();
    static void stopPlayback();

private:

    static void audioTransmissionTask(void *parameter);
    static EventDispatcher *eventDispatcher;


    // I2S configuration
    static const i2s_config_t i2sConfigRx;
    static const i2s_pin_config_t i2sPinConfigRx;
    static const i2s_config_t i2sConfigTx;
    static const i2s_pin_config_t i2sPinConfigTx;

    static volatile bool isRecording;
    static volatile bool isPlaying;

};

#endif // AUDIO_H
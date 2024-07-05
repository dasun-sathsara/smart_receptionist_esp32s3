#ifndef AUDIO_H
#define AUDIO_H

#include <Arduino.h>
#include <driver/i2s.h>
#include "freertos/ringbuf.h"
#include "events.h"

class Audio {
public:
    static void begin(EventDispatcher &dispatcher);

    static void startRecording();

    static void stopRecording();

    static void startPlayback();

    static void stopPlayback();

    static void addPrefetchData(const uint8_t *data, size_t length);

    static void startPrefetch();

    static void stopPrefetch();

private:
    static void audioTask(void *parameter);

    static EventDispatcher *eventDispatcher;

    // I2S configuration
    static const i2s_config_t i2sConfigRx;
    static const i2s_pin_config_t i2sPinConfigRx;
    static const i2s_config_t i2sConfigTx;
    static const i2s_pin_config_t i2sPinConfigTx;

    static volatile bool isRecording;
    static volatile bool isPlaying;
    static volatile bool isPrefetching;

    static uint8_t *audioBuffer;
    static size_t audioBufferIndex;
    static size_t audioBufferSize;

    static void clearAudioBuffer();

    static void sendAudioData();

    static size_t min(size_t a, size_t b);
};

#endif // AUDIO_H
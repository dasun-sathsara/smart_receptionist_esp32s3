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

    static void addDataToBuffer(const uint8_t *data, size_t length);

private:
    static void i2sReaderTask(void *parameter);

    static void i2sWriterTask(void *parameter);

    static void audioProcessingTask(void *parameter);

    static EventDispatcher *eventDispatcher;
    static RingbufHandle_t recordBuffer;
    static RingbufHandle_t playBuffer;


    static StaticRingbuffer_t recordBufferStatic;
    static StaticRingbuffer_t playBufferStatic;

    static TaskHandle_t i2sReaderTaskHandle;
    static TaskHandle_t i2sWriterTaskHandle;
    static TaskHandle_t audioProcessingTaskHandle;

    // I2S configuration
    static const i2s_config_t i2sConfigRx;
    static const i2s_pin_config_t i2sPinConfigRx;
    static const i2s_config_t i2sConfigTx;
    static const i2s_pin_config_t i2sPinConfigTx;

    static volatile bool isRecording;
    static volatile bool isPlaying;

    // Audio batching configuration
    static const size_t BATCH_SIZE;
    static const TickType_t BATCH_TIMEOUT;
};

#endif // AUDIO_H
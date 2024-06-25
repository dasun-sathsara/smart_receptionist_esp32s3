// audio.h
#ifndef AUDIO_H
#define AUDIO_H

#include <Arduino.h>
#include <driver/i2s.h>
#include "events.h"

class Audio {
public:
     void begin();

    static void startRecording();

    static void stopRecording();

    static void startPlayback();

    static void stopPlayback();

    static void addDataToBuffer(const uint8_t *data, size_t length);

private:
    [[noreturn]] static void i2sReaderTask(void *parameter);

    [[noreturn]] static void i2sWriterTask(void *parameter);

    static SemaphoreHandle_t i2sBufferMutex;
    static uint8_t *i2sBuffer;
    static size_t i2sBufferSize;
    static volatile size_t i2sBufferHead;
    static volatile size_t i2sBufferTail;
    static TaskHandle_t i2sReaderTaskHandle;
    static TaskHandle_t i2sWriterTaskHandle;

    // I2S configuration
    static const i2s_config_t i2sConfigRx;
    static const i2s_pin_config_t i2sPinConfigRx;
    static const i2s_config_t i2sConfigTx;
    static const i2s_pin_config_t i2sPinConfigTx;
};

#endif // AUDIO_H
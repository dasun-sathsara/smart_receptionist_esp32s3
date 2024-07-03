#include "audio.h"
#include "config.h"
#include "network_manager.h"
#include <esp_log.h>
#include <esp_heap_caps.h>
#include "logger.h"

static const char *TAG = "AUDIO";

EventDispatcher *Audio::eventDispatcher = nullptr;

volatile bool Audio::isRecording = false;
volatile bool Audio::isPlaying = false;

const i2s_config_t Audio::i2sConfigRx = {
        .mode = (i2s_mode_t) (I2S_MODE_MASTER | I2S_MODE_RX),
        .sample_rate = SAMPLE_RATE,
        .bits_per_sample = static_cast<i2s_bits_per_sample_t>(BITS_PER_SAMPLE),
        .channel_format = I2S_CHANNEL_FMT_ONLY_LEFT,
        .communication_format = I2S_COMM_FORMAT_STAND_I2S,
        .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
        .dma_buf_count = DMA_BUF_COUNT,
        .dma_buf_len = DMA_BUF_LEN,
        .use_apll = false,
        .tx_desc_auto_clear = false,
        .fixed_mclk = 0
};

const i2s_pin_config_t Audio::i2sPinConfigRx = {
        .bck_io_num = I2S_MIC_SERIAL_CLOCK,
        .ws_io_num = I2S_MIC_LEFT_RIGHT_CLOCK,
        .data_out_num = I2S_PIN_NO_CHANGE,
        .data_in_num = I2S_MIC_SERIAL_DATA
};

const i2s_config_t Audio::i2sConfigTx = {
        .mode = (i2s_mode_t) (I2S_MODE_MASTER | I2S_MODE_TX),
        .sample_rate = SAMPLE_RATE,
        .bits_per_sample = static_cast<i2s_bits_per_sample_t>(BITS_PER_SAMPLE),
        .channel_format = I2S_CHANNEL_FMT_ONLY_LEFT,
        .communication_format = I2S_COMM_FORMAT_STAND_I2S,
        .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
        .dma_buf_count = DMA_BUF_COUNT,
        .dma_buf_len = DMA_BUF_LEN,
        .use_apll = false,
        .tx_desc_auto_clear = true,
        .fixed_mclk = 0
};

const i2s_pin_config_t Audio::i2sPinConfigTx = {
        .bck_io_num = I2S_SPEAKER_SERIAL_CLOCK,
        .ws_io_num = I2S_SPEAKER_LEFT_RIGHT_CLOCK,
        .data_out_num = I2S_SPEAKER_SERIAL_DATA,
        .data_in_num = I2S_PIN_NO_CHANGE
};

void Audio::begin(EventDispatcher &dispatcher) {
    eventDispatcher = &dispatcher;

    // Install and configure I2S drivers
    ESP_ERROR_CHECK(i2s_driver_install(I2S_NUM_0, &i2sConfigRx, 0, nullptr));
    ESP_ERROR_CHECK(i2s_set_pin(I2S_NUM_0, &i2sPinConfigRx));
    ESP_ERROR_CHECK(i2s_driver_install(I2S_NUM_1, &i2sConfigTx, 0, nullptr));
    ESP_ERROR_CHECK(i2s_set_pin(I2S_NUM_1, &i2sPinConfigTx));
}

void Audio::startRecording() {
    isRecording = true;
    LOG_I(TAG, "Recording started");
}

void Audio::stopRecording() {
    isRecording = false;
    LOG_I(TAG, "Recording stopped");
}

void Audio::startPlayback() {
    isPlaying = true;
    LOG_I(TAG, "Playback started");
}

void Audio::stopPlayback() {
    isPlaying = false;
    LOG_I(TAG, "Stopping playback...");
}

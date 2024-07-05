#include "audio.h"
#include "config.h"
#include "logger.h"

static const char *TAG = "AUDIO";

EventDispatcher *Audio::eventDispatcher = nullptr;
volatile bool Audio::isRecording = false;
volatile bool Audio::isPlaying = false;
volatile bool Audio::isPrefetching = false;
uint8_t *Audio::audioBuffer = nullptr;
size_t Audio::audioBufferIndex = 0;
size_t Audio::audioBufferSize = AUDIO_BUFFER_SIZE;

const i2s_config_t Audio::i2sConfigRx = {
        .mode = (i2s_mode_t) (I2S_MODE_MASTER | I2S_MODE_RX),
        .sample_rate = SAMPLE_RATE,
        .bits_per_sample = static_cast<i2s_bits_per_sample_t>(BITS_PER_SAMPLE),
        .channel_format = I2S_CHANNEL_FMT_ONLY_LEFT,
        .communication_format = I2S_COMM_FORMAT_STAND_I2S,
        .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
        .dma_buf_count = DMA_BUF_COUNT,
        .dma_buf_len = DMA_BUF_LEN,
        .use_apll = true,
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
        .use_apll = true,
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

    // Allocate audio buffer in PSRAM
    audioBuffer = (uint8_t *) ps_malloc(AUDIO_BUFFER_SIZE);
    if (audioBuffer == nullptr) {
        LOG_E(TAG, "Failed to allocate audio buffer in PSRAM");
        return;
    }

    // Install and configure I2S drivers
    ESP_ERROR_CHECK(i2s_driver_install(I2S_NUM_0, &i2sConfigRx, 0, nullptr));
    ESP_ERROR_CHECK(i2s_set_pin(I2S_NUM_0, &i2sPinConfigRx));
    ESP_ERROR_CHECK(i2s_driver_install(I2S_NUM_1, &i2sConfigTx, 0, nullptr));
    ESP_ERROR_CHECK(i2s_set_pin(I2S_NUM_1, &i2sPinConfigTx));

    xTaskCreatePinnedToCore(audioTask, "AudioTask", 8192, nullptr, 5, nullptr, 1);
}

size_t Audio::min(size_t a, size_t b) {
    return (a < b) ? a : b;
}

void Audio::startRecording() {
    isRecording = true;
    audioBufferIndex = 0;
    LOG_I(TAG, "Recording started");
}

void Audio::stopRecording() {
    LOG_I(TAG, "Recording stopped. Recorded %d bytes", audioBufferIndex);
    isRecording = false;
    vTaskDelay(pdMS_TO_TICKS(100)); // Wait for last audio chunk to be read
    sendAudioData();
}

void Audio::startPlayback() {
    if (audioBufferIndex > 0) {
        isPlaying = true;
        LOG_I(TAG, "Playback started with %d bytes", audioBufferIndex);
    } else {
        LOG_W(TAG, "Playback not started. No audio data available.");
        eventDispatcher->dispatchEvent({NO_AUDIO_DATA, ""});
    }
}

void Audio::stopPlayback() {
    isPlaying = false;
    vTaskDelay(100); // Wait for last audio chunk to be sent
    LOG_I(TAG, "Playback stopped");
    clearAudioBuffer();
}

void Audio::startPrefetch() {
    isPrefetching = true;
    audioBufferIndex = 0;
    LOG_I(TAG, "Prefetching started");
}

void Audio::stopPrefetch() {
    isPrefetching = false;
    LOG_I(TAG, "Prefetching stopped. Collected %d bytes", audioBufferIndex);
}

void Audio::addPrefetchData(const uint8_t *data, size_t length) {
    if (isPrefetching && (audioBufferIndex + length) <= audioBufferSize) {
        memcpy(audioBuffer + audioBufferIndex, data, length);
        audioBufferIndex += length;
    }
}

void Audio::clearAudioBuffer() {
    memset(audioBuffer, 0, audioBufferSize);
    audioBufferIndex = 0;
}

void Audio::sendAudioData() {
    const size_t chunkSize = 1024 * 8; // Send 8KB at a time
    size_t remainingBytes = audioBufferIndex;
    size_t offset = 0;

    while (remainingBytes > 0) {
        size_t bytesToSend = min(chunkSize, remainingBytes);
        eventDispatcher->dispatchEvent({AUDIO_DATA_READY, std::string(reinterpret_cast<char *>(audioBuffer + offset), bytesToSend)});
        remainingBytes -= bytesToSend;
        offset += bytesToSend;
        vTaskDelay(pdMS_TO_TICKS(10));
    }

    LOG_I(TAG, "Dispatched %u bytes of audio data in chunks", audioBufferIndex);

    eventDispatcher->dispatchEvent({RECORDING_SENT, ""});
}

void Audio::audioTask(void *parameter) {
    size_t bytesRead = 0;
    size_t bytesWritten = 0;
    while (true) {
        if (isRecording) {
            if (audioBufferIndex + DMA_BUF_LEN <= audioBufferSize) {
                esp_err_t result = i2s_read(I2S_NUM_0, audioBuffer + audioBufferIndex, DMA_BUF_LEN, &bytesRead, portMAX_DELAY);
                if (result == ESP_OK) {
                    audioBufferIndex += bytesRead;
                } else {
                    LOG_E(TAG, "Error reading from I2S: %d", result);
                }
            } else {
                LOG_W(TAG, "Audio buffer full. Stopping recording.");
                stopRecording();
            }
        } else if (isPlaying && audioBufferIndex > 0) {
            size_t bytesToWrite = min(DMA_BUF_LEN, audioBufferIndex);
            esp_err_t result = i2s_write(I2S_NUM_1, audioBuffer, bytesToWrite, &bytesWritten, portMAX_DELAY);

            if (result == ESP_OK) {
                // Shift the remaining audio data to the beginning of the buffer
                memmove(audioBuffer, audioBuffer + bytesWritten, audioBufferIndex - bytesWritten);
                audioBufferIndex -= bytesWritten;
            } else {
                LOG_E(TAG, "Error writing to I2S: %d", result);
            }
            if (audioBufferIndex == 0) {
                LOG_I(TAG, "Playback completed.");
                stopPlayback();
            }

        }
        vTaskDelay(pdMS_TO_TICKS(10)); // Small delay to prevent task from hogging CPU
    }
}
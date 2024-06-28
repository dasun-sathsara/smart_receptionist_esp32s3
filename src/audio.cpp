#include "audio.h"
#include "config.h"
#include "network_manager.h"
#include <esp_log.h>
#include <esp_heap_caps.h>

static const char *TAG = "AUDIO";

EventDispatcher *Audio::eventDispatcher = nullptr;
RingbufHandle_t Audio::recordBuffer = nullptr;
RingbufHandle_t Audio::playBuffer = nullptr;
TaskHandle_t Audio::i2sReaderTaskHandle = nullptr;
TaskHandle_t Audio::i2sWriterTaskHandle = nullptr;
TaskHandle_t Audio::audioProcessingTaskHandle = nullptr;
volatile bool Audio::isRecording = false;
volatile bool Audio::isPlaying = false;
StaticRingbuffer_t Audio::recordBufferStatic;
StaticRingbuffer_t Audio::playBufferStatic;

// Audio batching configuration
const size_t Audio::BATCH_SIZE = 4096; // Adjust this value based on your needs
const TickType_t Audio::BATCH_TIMEOUT = pdMS_TO_TICKS(100); // 100ms timeout


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

    recordBuffer = xRingbufferCreateStatic(RING_BUF_SIZE, RINGBUF_TYPE_BYTEBUF,
                                           (uint8_t *) ps_malloc(RING_BUF_SIZE),
                                           &recordBufferStatic);

    playBuffer = xRingbufferCreateStatic(RING_BUF_SIZE, RINGBUF_TYPE_BYTEBUF,
                                         (uint8_t *) ps_malloc(RING_BUF_SIZE),
                                         &playBufferStatic);

    if (recordBuffer == nullptr || playBuffer == nullptr) {
        ESP_LOGE(TAG, "Failed to create ring buffers");
        return;
    }

    ESP_ERROR_CHECK(i2s_driver_install(I2S_NUM_0, &i2sConfigRx, 0, nullptr));
    ESP_ERROR_CHECK(i2s_set_pin(I2S_NUM_0, &i2sPinConfigRx));

    ESP_ERROR_CHECK(i2s_driver_install(I2S_NUM_1, &i2sConfigTx, 0, nullptr));
    ESP_ERROR_CHECK(i2s_set_pin(I2S_NUM_1, &i2sPinConfigTx));

    xTaskCreatePinnedToCore(i2sReaderTask, "I2S Reader Task", 8192, nullptr, 5, &i2sReaderTaskHandle, 0);
    xTaskCreatePinnedToCore(i2sWriterTask, "I2S Writer Task", 8192, nullptr, 5, &i2sWriterTaskHandle, 1);
    xTaskCreatePinnedToCore(audioProcessingTask, "Audio Processing Task", 8192, nullptr, 4, &audioProcessingTaskHandle, 1);

    // Initially suspend all tasks
    vTaskSuspend(i2sReaderTaskHandle);
    vTaskSuspend(i2sWriterTaskHandle);
    vTaskSuspend(audioProcessingTaskHandle);

    ESP_LOGI(TAG, "Audio system initialized");
}

void Audio::startRecording() {
    isRecording = true;
    vTaskResume(i2sReaderTaskHandle);
    vTaskResume(audioProcessingTaskHandle);
    ESP_LOGI(TAG, "Recording started");
}

void Audio::stopRecording() {
    isRecording = false;
    vTaskSuspend(i2sReaderTaskHandle);
    vTaskSuspend(audioProcessingTaskHandle);
    ESP_LOGI(TAG, "Recording stopped");
}

void Audio::startPlayback() {
    isPlaying = true;
    vTaskResume(i2sWriterTaskHandle);
    ESP_LOGI(TAG, "Playback started");
}

void Audio::stopPlayback() {
    isPlaying = false;
    vTaskSuspend(i2sWriterTaskHandle);

    // Clear the I2S DMA buffer
    i2s_zero_dma_buffer(I2S_NUM_1);

    // Clear the play buffer
    size_t itemSize;
    void *item;
    while ((item = xRingbufferReceive(playBuffer, &itemSize, 0)) != nullptr) {
        vRingbufferReturnItem(playBuffer, item);
    }

    ESP_LOGI(TAG, "Playback stopped and buffers cleared");
}

void Audio::addDataToBuffer(const uint8_t *data, size_t length) {
    if (xRingbufferSend(playBuffer, data, length, pdMS_TO_TICKS(100)) != pdTRUE) {
        ESP_LOGW(TAG, "Failed to add data to play buffer");
    }
}

void Audio::i2sReaderTask(void *parameter) {
    size_t bytesRead = 0;
    const size_t bufferSize = DMA_BUF_LEN * 2;
    uint8_t *buffer = (uint8_t *) heap_caps_malloc(bufferSize, MALLOC_CAP_DMA);

    if (!buffer) {
        ESP_LOGE(TAG, "Failed to allocate memory for audio buffer");
        vTaskDelete(nullptr);
        return;
    }

    while (true) {
        if (isRecording) {
            ESP_ERROR_CHECK(i2s_read(I2S_NUM_0, buffer, bufferSize, &bytesRead, portMAX_DELAY));
            if (bytesRead > 0) {
                if (xRingbufferSend(recordBuffer, buffer, bytesRead, pdMS_TO_TICKS(100)) != pdTRUE) {
                    ESP_LOGW(TAG, "Failed to add data to record buffer");
                }
            }
        }
        vTaskDelay(pdMS_TO_TICKS(10));
    }

    heap_caps_free(buffer);
}

void Audio::i2sWriterTask(void *parameter) {
    size_t bytesWritten = 0;
    size_t itemSize = 0;
    uint8_t *item = nullptr;

    while (true) {
        if (isPlaying) {
            item = (uint8_t *) xRingbufferReceive(playBuffer, &itemSize, pdMS_TO_TICKS(10));
            if (item != nullptr) {
                ESP_ERROR_CHECK(i2s_write(I2S_NUM_1, item, itemSize, &bytesWritten, portMAX_DELAY));
                vRingbufferReturnItem(playBuffer, item);
            }
        }
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

void Audio::audioProcessingTask(void *parameter) {
    size_t itemSize = 0;
    uint8_t *item = nullptr;
    auto *batchBuffer = (uint8_t *) heap_caps_malloc(BATCH_SIZE + 6, MALLOC_CAP_DMA);
    size_t batchedSize = 0;
    TickType_t lastSendTime = xTaskGetTickCount();

    if (!batchBuffer) {
        ESP_LOGE(TAG, "Failed to allocate memory for batch buffer");
        vTaskDelete(nullptr);
        return;
    }

    memcpy(batchBuffer, "AUDIO:", 6);

    while (true) {
        if (isRecording) {
            item = (uint8_t *) xRingbufferReceive(recordBuffer, &itemSize, pdMS_TO_TICKS(10));
            if (item != nullptr) {
                size_t remainingSpace = BATCH_SIZE - batchedSize;
                size_t copySize = (itemSize < remainingSpace) ? itemSize : remainingSpace;

                memcpy(batchBuffer + 6 + batchedSize, item, copySize);
                batchedSize += copySize;

                vRingbufferReturnItem(recordBuffer, item);

                if (batchedSize >= BATCH_SIZE || (xTaskGetTickCount() - lastSendTime) >= BATCH_TIMEOUT) {
                    NetworkManager::sendAudioChunk(batchBuffer, batchedSize + 6);
                    batchedSize = 0;
                    lastSendTime = xTaskGetTickCount();
                }
            }
        } else {
            // If not recording, reset the batched size and last send time
            batchedSize = 0;
            lastSendTime = xTaskGetTickCount();
        }
//        vTaskDelay(pdMS_TO_TICKS(10));
        vTaskDelay(pdMS_TO_TICKS(100)); // Change it back to 10 if it doesn't work
    }

    heap_caps_free(batchBuffer);
}
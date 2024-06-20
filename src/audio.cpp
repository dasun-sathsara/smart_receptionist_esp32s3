#include "audio.h"
#include "config.h"
#include "network_manager.h"
#include <esp_log.h>

static const char *TAG = "AUDIO";

const i2s_config_t Audio::i2sConfigRx = {
        .mode = (i2s_mode_t) (I2S_MODE_MASTER | I2S_MODE_RX),
        .sample_rate = SAMPLE_RATE,
        .bits_per_sample = (i2s_bits_per_sample_t) BITS_PER_SAMPLE,
        .channel_format = I2S_CHANNEL_FMT_ONLY_LEFT,
        .communication_format = (i2s_comm_format_t) (I2S_COMM_FORMAT_STAND_I2S),
        .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
        .dma_buf_count = DMA_BUF_COUNT,
        .dma_buf_len = DMA_BUF_LEN,
        .use_apll = false,
        .tx_desc_auto_clear = true,
        .fixed_mclk = 0
};

const i2s_pin_config_t Audio::i2sPinConfigRx = {
        .bck_io_num = I2S_PIN_INMP441_SCK,
        .ws_io_num = I2S_PIN_INMP441_WS,
        .data_out_num = I2S_PIN_NO_CHANGE,
        .data_in_num = I2S_PIN_INMP441_SD
};

const i2s_config_t Audio::i2sConfigTx = {
        .mode = (i2s_mode_t) (I2S_MODE_MASTER | I2S_MODE_TX),
        .sample_rate = SAMPLE_RATE,
        .bits_per_sample = (i2s_bits_per_sample_t) BITS_PER_SAMPLE,
        .channel_format = I2S_CHANNEL_FMT_ONLY_LEFT,
        .communication_format = (i2s_comm_format_t) (I2S_COMM_FORMAT_STAND_I2S),
        .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
        .dma_buf_count = DMA_BUF_COUNT,
        .dma_buf_len = DMA_BUF_LEN,
        .use_apll = false
};

const i2s_pin_config_t Audio::i2sPinConfigTx = {
        .bck_io_num = I2S_PIN_BCLK,
        .ws_io_num = I2S_PIN_LRCK,
        .data_out_num = I2S_PIN_DATA,
        .data_in_num = I2S_PIN_NO_CHANGE
};

SemaphoreHandle_t Audio::i2sBufferMutex = nullptr;
uint8_t *Audio::i2sBuffer = nullptr;
size_t Audio::i2sBufferSize = I2S_BUFFER_SIZE;
volatile size_t Audio::i2sBufferHead = 0;
volatile size_t Audio::i2sBufferTail = 0;
TaskHandle_t Audio::i2sReaderTaskHandle = nullptr;
TaskHandle_t Audio::i2sWriterTaskHandle = nullptr;

void Audio::begin() {
    i2sBufferMutex = xSemaphoreCreateMutex();

    // Allocate memory for the I2S buffer dynamically
    i2sBuffer = (uint8_t *) malloc(i2sBufferSize * sizeof(uint8_t));

    if (i2sBuffer == nullptr) {
        ESP_LOGE(TAG, "Error allocating memory for I2S buffer!");
    }

    // Initialize I2S RX
    if (i2s_driver_install(I2S_NUM_1, &i2sConfigRx, 0, nullptr) != ESP_OK) {
        ESP_LOGE(TAG, "Error installing I2S RX driver!");
    }

    i2s_set_pin(I2S_NUM_1, &i2sPinConfigRx);
    i2s_zero_dma_buffer(I2S_NUM_1);

    // Initialize I2S TX
    if (i2s_driver_install(I2S_NUM_0, &i2sConfigTx, 0, nullptr) != ESP_OK) {
        ESP_LOGE(TAG, "Error installing I2S TX driver!");
    }
    i2s_set_pin(I2S_NUM_0, &i2sPinConfigTx);
    i2s_zero_dma_buffer(I2S_NUM_0);

    xTaskCreatePinnedToCore(i2sReaderTask, "I2S Reader Task", 8192, nullptr, 2, &i2sReaderTaskHandle, 1);
    xTaskCreatePinnedToCore(i2sWriterTask, "I2S Writer Task", 8192, nullptr, 3, &i2sWriterTaskHandle, 1);

    vTaskSuspend(i2sReaderTaskHandle); // Initially suspend the reader task
    vTaskSuspend(i2sWriterTaskHandle); // Initially suspend the writer task
}

void Audio::startRecording() {
    vTaskResume(i2sReaderTaskHandle);
    ESP_LOGI(TAG, "Recording started");
}

void Audio::stopRecording() {
    vTaskSuspend(i2sReaderTaskHandle);
    ESP_LOGI(TAG, "Recording stopped");
}

void Audio::startPlayback() {
    vTaskResume(i2sWriterTaskHandle);
    ESP_LOGI(TAG, "Playback started");
}

void Audio::stopPlayback() {
    vTaskSuspend(i2sWriterTaskHandle);
    ESP_LOGI(TAG, "Playback stopped");

    // Clear the I2S DMA buffer
    i2s_zero_dma_buffer(I2S_NUM_0);

    // Clear the buffer when playback stops
    if (xSemaphoreTake(i2sBufferMutex, portMAX_DELAY) == pdTRUE) {
        i2sBufferHead = 0;
        i2sBufferTail = 0;
        xSemaphoreGive(i2sBufferMutex);
    }
}

void Audio::addDataToBuffer(const uint8_t *data, size_t length) {
    if (xSemaphoreTake(i2sBufferMutex, portMAX_DELAY) == pdTRUE) {
        for (size_t i = 0; i < length; i++) {
            i2sBuffer[i2sBufferHead] = data[i];
            i2sBufferHead = (i2sBufferHead + 1) % I2S_BUFFER_SIZE;
            if (i2sBufferHead == i2sBufferTail) // Buffer overflow, discard oldest data
            {
                i2sBufferTail = (i2sBufferTail + 1) % I2S_BUFFER_SIZE;
            }
        }
        xSemaphoreGive(i2sBufferMutex);
    }
}

[[noreturn]] void Audio::i2sReaderTask(void *parameter) {
    size_t bytesRead = 0;

    // Dynamically allocate i2SMicBuffer
    auto *i2SMicBuffer = new uint8_t[DMA_BUF_LEN * 2];
    auto *prependedBuffer = new uint8_t[DMA_BUF_LEN * 2 + 6];

    while (true) {
        i2s_read(I2S_NUM_1, i2SMicBuffer, DMA_BUF_LEN * 2, &bytesRead, portMAX_DELAY); // Read from Microphone
        if (bytesRead > 0) {
            memcpy(prependedBuffer, "AUDIO:", 6);
            memcpy(prependedBuffer + 6, i2SMicBuffer, bytesRead);

            NetworkManager::sendAudioChunk(prependedBuffer, bytesRead + 6);


//            Event event = {EVENT_AUDIO_CHUNK_READ, std::string(reinterpret_cast<char *>(prependedBuffer)),
//                           bytesRead + 6};
//            eventDispatcher->dispatchEvent(event);
        }

    }

    // Free allocated memory
    delete[] i2SMicBuffer;
    delete[] prependedBuffer;
}

[[noreturn]] void Audio::i2sWriterTask(void *parameter) {
    size_t bytesWritten = 0;
    for (;;) {
        if (xSemaphoreTake(i2sBufferMutex, portMAX_DELAY) == pdTRUE) {
            if (i2sBufferTail != i2sBufferHead) {
                size_t bytesToWrite = (i2sBufferTail < i2sBufferHead) ? (i2sBufferHead - i2sBufferTail) : (
                        I2S_BUFFER_SIZE - i2sBufferTail);
                i2s_write(I2S_NUM_0, &i2sBuffer[i2sBufferTail], bytesToWrite, &bytesWritten, portMAX_DELAY);
                i2sBufferTail = (i2sBufferTail + bytesWritten) % I2S_BUFFER_SIZE;
            }
            xSemaphoreGive(i2sBufferMutex);
        }
        vTaskDelay(10);
    }
}
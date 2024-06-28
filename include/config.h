#ifndef CONFIG_H
#define CONFIG_H

#include <Arduino.h>
#include "U8g2lib.h"

// WiFi credentials
#define WIFI_SSID "Xperia 1 II"
#define WIFI_PASSWORD "12345678"

// WebSocket server details
#define WS_SERVER "192.168.8.35"
#define WS_PORT 8765

// L298N motor driver configuration
#define MOTOR_PIN1 8
#define MOTOR_PIN2 9
#define MOTOR_ENABLE 10

// PIR sensor configuration
#define PIR_PIN 21

// Break beam sensor configuration
#define BREAK_BEAM_PIN 2

// LED strip configuration
#define LED_STRIP_PIN 38

// I2S configuration for INMP441 microphone
#define I2S_MIC_SERIAL_CLOCK 40
#define I2S_MIC_LEFT_RIGHT_CLOCK 41
#define I2S_MIC_SERIAL_DATA 39

// I2S configuration for MAX98357A amplifier
#define I2S_SPEAKER_SERIAL_CLOCK 13
#define I2S_SPEAKER_LEFT_RIGHT_CLOCK 14
#define I2S_SPEAKER_SERIAL_DATA 21

// Audio settings
#define SAMPLE_RATE 48000
#define BITS_PER_SAMPLE 16
#define CHANNELS 1

// DMA buffer settings
#define DMA_BUF_COUNT 8
#define DMA_BUF_LEN 1024

// Ring buffer size (1MB)
#define RING_BUF_SIZE (1024 * 1024)

// Fingerprint sensor configuration
#define FINGERPRINT_TX 17
#define FINGERPRINT_RX 18

// OLED I2C address
#define SDA_PIN 8
#define SCL_PIN 18
#define I2C_ADDRESS 0x3C

#endif // CONFIG_H
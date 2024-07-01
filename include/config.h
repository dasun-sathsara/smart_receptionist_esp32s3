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
#define MOTOR_PIN1 39
#define MOTOR_PIN2 38
#define MOTOR_ENABLE 47

// PIR sensor configuration
#define PIR_PIN 42

// Break beam sensor configuration
#define BREAK_BEAM_PIN 41

// LED strip configuration
#define LED_STRIP_PIN 40

// I2S configuration for INMP441 microphone
#define I2S_MIC_SERIAL_CLOCK 3
#define I2S_MIC_LEFT_RIGHT_CLOCK 9
#define I2S_MIC_SERIAL_DATA 10

// I2S configuration for MAX98357A amplifier
#define I2S_SPEAKER_SERIAL_CLOCK 11
#define I2S_SPEAKER_LEFT_RIGHT_CLOCK 12
#define I2S_SPEAKER_SERIAL_DATA 13

// Audio settings
#define SAMPLE_RATE 48000
#define BITS_PER_SAMPLE 16

// DMA buffer settings
#define DMA_BUF_COUNT 8
#define DMA_BUF_LEN 1024

// Ring buffer size (1MB)
#define RING_BUF_SIZE (1024 * 1024)

// Fingerprint sensor configuration
#define FINGERPRINT_TX 1
#define FINGERPRINT_RX 2

// OLED I2C address
#define SDA_PIN 18
#define SCL_PIN 8
#define I2C_ADDRESS 0x3C

// Keypad configuration

#define ROW1 4
#define ROW2 5
#define ROW3 6
#define ROW4 7
#define COL1 15
#define COL2 16
#define COL3 17

#endif // CONFIG_H
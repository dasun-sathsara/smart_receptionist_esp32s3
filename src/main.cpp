#include <Arduino.h>
#include "events.h"
#include "audio.h"
#include "network_manager.h"
#include "ui.h"
#include "fingerprint.h"
#include "logger.h"
#include "esp_now_manager.h"
#include "gate.h"
#include "pir.h"
#include "led.h"
#include "event_handler.h"
#include "soc/soc.h"
#include "soc/rtc_cntl_reg.h"
#include <PubSubClient.h>

static const char *TAG = "MAIN";

EventDispatcher eventDispatcher;
NetworkManager network;
UI ui;
HardwareSerial fingerprintSerial(1);
FingerprintHandler fingerprintHandler(fingerprintSerial);
PIRSensor pirSensor;
Gate gate;
LED led;
Audio audio;
ESPNow espNow;
EventHandler eventHandler(audio, network, gate, led, ui, espNow, fingerprintHandler, pirSensor);

WiFiClient espClient;
PubSubClient mqttClient(espClient);

void setup() {
    Serial.begin(115200);
    network.begin(eventDispatcher);
    vTaskDelay(2000);

    // Initialize MQTT client
    mqttClient.setServer("192.168.17.218", 1883);
    logger.begin(mqttClient);
    mqttClient.connect("SmartReceptionist");

    // Disable brownout detector
    WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0);
    LOG_I(TAG, "Brownout detector disabled");

    eventHandler.registerCallbacks(eventDispatcher);
    gate.begin(eventDispatcher);
    ui.begin(eventDispatcher);
    audio.begin(eventDispatcher);
    fingerprintHandler.begin(eventDispatcher);
    pirSensor.begin(eventDispatcher);
    led.begin(eventDispatcher);
    espNow.begin(eventDispatcher);


    LOG_I(TAG, "System initialization complete");
}

void loop() {
}
#include "sensor_task.h"
#include "logger.h"

static const char *TAG = "SensorTask";

void sensorTask(void *pvParameters) {
    int lastBreakBeamState = -1;
    int lastDoorState = -1;

    pinMode(BREAK_BEAM_PIN, INPUT_PULLUP);
    pinMode(REED_SWITCH_PIN, INPUT_PULLUP);

    while (true) {
        int currentBreakBeamState = digitalRead(BREAK_BEAM_PIN);
        int currentDoorState = digitalRead(REED_SWITCH_PIN);

        if (currentBreakBeamState != lastBreakBeamState) {
            LOG_I(TAG, "Break beam sensor state changed: %s",
                  currentBreakBeamState == LOW ? "Beam broken" : "Beam intact");
            lastBreakBeamState = currentBreakBeamState;
        }

        if (currentDoorState != lastDoorState) {
            LOG_I(TAG, "Door sensor state changed: %s",
                  currentDoorState == LOW ? "Door closed" : "Door open");
            lastDoorState = currentDoorState;
        }

        vTaskDelay(pdMS_TO_TICKS(100)); // Check every 100ms
    }
}
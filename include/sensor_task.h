#ifndef SENSOR_TASK_H
#define SENSOR_TASK_H

#include <Arduino.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "config.h"

void sensorTask(void *pvParameters);

#endif // SENSOR_TASK_H
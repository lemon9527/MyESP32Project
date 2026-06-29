/*
 * SPDX-FileCopyrightText: 2010-2022 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: CC0-1.0
 */

#include <stdio.h>
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"

#define LED_GPIO_PIN 2

static const char *TAG = "main";

void Task_1(void *pvParameters)
{
    while (1) {
        ESP_LOGI(TAG, "Task 1 is running");
        vTaskDelay(pdMS_TO_TICKS(1000)); // 延时 1 秒
    }
    vTaskDelete(NULL);
}


void Task_2(void *pvParameters)
{
    while (1) {
        ESP_LOGI(TAG, "Task 2 is running");
        vTaskDelay(pdMS_TO_TICKS(1000)); // 延时 1 秒
    }
    vTaskDelete(NULL);
}


// 任务2: 控制任务1的挂起与恢复

void app_main(void)
{
    TaskHandle_t taskHandle_1 = NULL;
    TaskHandle_t taskHandle_2 = NULL;

    xTaskCreate(Task_1, "Task_1", 2048, NULL, 5, &taskHandle_1);
    xTaskCreate(Task_2, "Task_2", 2048, NULL, 5, &taskHandle_2);

    // 输出任务列表
    static char cBuffer[512] = {0};
    vTaskList(cBuffer);
    ESP_LOGI(TAG, "任务列表:\n%s", cBuffer);

    while(1)
    {
        int istack = uxTaskGetStackHighWaterMark(taskHandle_1);
        ESP_LOGI(TAG, "任务1栈水位: %d", istack);
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}

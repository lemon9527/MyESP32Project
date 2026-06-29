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

TaskHandle_t taskHandle_1 = NULL;

// 任务1: 定时打印日志
void Task_1(void *pvParameters)
{
    while (1) {
        ESP_LOGI(TAG, "Task 1 is running");
        vTaskDelay(pdMS_TO_TICKS(1000)); // 延时 1 秒
    }
}

// 任务2: 控制任务1的挂起与恢复
void Task_2(void *pvParameters)
{
    while (1) {
        ESP_LOGI(TAG, "挂起任务1...");
        vTaskSuspend(taskHandle_1); // 挂起任务1
        vTaskDelay(pdMS_TO_TICKS(5000)); // 延时 5 秒

        ESP_LOGI(TAG, "恢复任务1...");
        vTaskResume(taskHandle_1); // 恢复任务1
        vTaskDelay(pdMS_TO_TICKS(5000)); // 延时 5 秒
    }
}


// 任务2: 控制任务1的挂起与恢复

void app_main(void)
{
    // Create a FreeRTOS task
    // parameters description:
    // 1. 任务入口函数: myTask
    // 2. 任务名称: myTask
    // 3. 任务栈大小: 2048 words (8192 bytes)
    // 4. 任务参数: void * 类型的指针，可以传递任意类型的数据（整型、数组、结构体或字符串等）
    // 5. 任务优先级: 1（优先级较低，空闲任务优先级为 0）
    // 6. 任务句柄: NULL

    // 创建任务1
    xTaskCreate(Task_1, "Task_1", 2048, NULL, 5, &taskHandle_1);
    // 创建任务2
    xTaskCreate(Task_2, "Task_2", 2048, NULL, 5, NULL);
}

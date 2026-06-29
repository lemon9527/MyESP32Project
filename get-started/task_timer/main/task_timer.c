/*
 * SPDX-FileCopyrightText: 2010-2022 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: CC0-1.0
 */

#include <stdio.h>
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/timers.h"
#include "driver/gpio.h"

#define LED_GPIO_PIN 2

static const char *TAG = "main";

// 定时器回调函数
void TimerCallback_1(TimerHandle_t xTimer)
{
    if (pvTimerGetTimerID(xTimer) == (void*)0) {
        ESP_LOGI(TAG, "Timer_1 is running");
    } else if (pvTimerGetTimerID(xTimer) == (void*)1) {
        ESP_LOGI(TAG, "Timer_2 is running");
    }
}



void app_main(void)
{
    //创建定时器句柄
    TimerHandle_t xTimer_1;
    TimerHandle_t xTimer_2;
    xTimer_1 = xTimerCreate("Timer_1", pdMS_TO_TICKS(1000), pdTRUE, (void*)0, TimerCallback_1);
    xTimer_2 = xTimerCreate("Timer_2", pdMS_TO_TICKS(2000), pdTRUE, (void*)1, TimerCallback_1);

    // 启动定时器
    xTimerStart(xTimer_1, pdTRUE);
    xTimerStart(xTimer_2, pdTRUE);

    vTaskDelay(pdMS_TO_TICKS(5000));

    // 停止定时器
    xTimerStop(xTimer_1, 0);
    xTimerStop(xTimer_2, 0);
}

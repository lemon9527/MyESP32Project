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

static const char *TAG = "task_create_main";

void myTask(void *pvParameter)
{
    while (1) {
        vTaskDelay(1000 / portTICK_PERIOD_MS);
        ESP_LOGI(TAG, "myTask: 1");
        vTaskDelay(1000 / portTICK_PERIOD_MS);
        ESP_LOGI(TAG, "myTask: 2");
        vTaskDelay(1000 / portTICK_PERIOD_MS);
        ESP_LOGI(TAG, "myTask: 3");

        //delete task myTask after 3 seconds
        vTaskDelete(NULL);
    }
}

void myTask2(void *pvParameter)
{
    while (1) {
        ESP_LOGI(TAG, "Hello from myTask2!");
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}

void ledTask(void *pvParameter)
{
    gpio_reset_pin(LED_GPIO_PIN);
    gpio_set_direction(LED_GPIO_PIN, GPIO_MODE_OUTPUT);
    while (1) {
        gpio_set_level(LED_GPIO_PIN, 0);
        ESP_LOGI(TAG, "Toggle LED from ledTask!");
        vTaskDelay(500 / portTICK_PERIOD_MS);
        gpio_set_level(LED_GPIO_PIN, 1);
        ESP_LOGI(TAG, "Toggle LED from ledTask!");
        vTaskDelay(500 / portTICK_PERIOD_MS);
    }
}



void app_main(void)
{
    // Create a FreeRTOS task
    // parameters description:
    // 1. 任务入口函数: myTask
    // 2. 任务名称: myTask
    // 3. 任务栈大小: 2048 words (8192 bytes)
    // 4. 任务参数: NULL
    // 5. 任务优先级: 1（优先级较低，空闲任务优先级为 0）
    // 6. 任务句柄: NULL

    TaskHandle_t taskHandle = NULL;
    TaskHandle_t taskHandle2 = NULL;
    TaskHandle_t taskHandle3 = NULL;

    xTaskCreate(myTask, "myTask", 2048, NULL, 1, &taskHandle);
    xTaskCreate(myTask2, "myTask2", 2048, NULL, 1, &taskHandle2);
    xTaskCreate(ledTask, "ledTask", 2048, NULL, 1, &taskHandle3);

    //vTaskDelay(2000 / portTICK_PERIOD_MS);

    // delete task myTask
    /*
    为什么要判断 taskHandle != NULL？
    这是一种防御性编程。虽然你的代码中 xTaskCreate 成功后 taskHandle 一定非 NULL，但如果：

    xTaskCreate 失败了（比如内存不足），taskHandle 就还是 NULL
    传入 NULL 给 vTaskDelete 会变成删除自己，这显然不是你想要的
    所以先判断再删除是一个好习惯，确保不会意外删除 app_main 自身。
    */
    // if(taskHandle != NULL)
    // {
    //     vTaskDelete(taskHandle);
    //     taskHandle = NULL;
    // }

}

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

typedef struct {
    int Int;
    int Array[3];
} MyStruct;

// 任务函数1:接受整型参数
void Task_1(void *pvParameter)
{
    int *pInt = (int *)pvParameter;
    ESP_LOGI(TAG, "Task_1 取得整型参数为: %d", *pInt);
    vTaskDelete(NULL);
}

// 任务函数2:接受数组参数
void Task_2(void *pvParameter)
{
    int *pArray = (int *)pvParameter;
    ESP_LOGI(TAG, "Task_2 取得数组参数为: %d, %d, %d", *pArray, *(pArray + 1), *(pArray + 2));
    vTaskDelete(NULL);
}

// 任务函数3:接受结构体参数
void Task_3(void *pvParameter)
{
    MyStruct *pStruct = (MyStruct *)pvParameter;
    ESP_LOGI(TAG, "Task_3 取得struct参数为: Int = %d, Array[0] = %d, Array[1] = %d, Array[2] = %d", pStruct->Int, pStruct->Array[0], pStruct->Array[1], pStruct->Array[2]);
    vTaskDelete(NULL);
}

// 任务函数4:接受字符串参数
void Task_4(void *pvParameter)
{
    const char *pStr = (const char *)pvParameter;
    ESP_LOGI(TAG, "Task_4 取得str参数为: %s", pStr);
    vTaskDelete(NULL);
}

int Parameters_1 = 1;
int Parameters_2[3] = {1, 2, 3};
MyStruct Parameters_3 = {1, {1, 2, 3}};
static const char *Parameters_4 = "Hello, World!";

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

    UBaseType_t taskPriority_1 = 0;
    UBaseType_t taskPriority_2 = 0;
    TaskHandle_t taskHandle_1 = NULL;
    TaskHandle_t taskHandle_2 = NULL;

    // 传递整型参数
    xTaskCreate(Task_1, "Task_1", 2048, (void *)&Parameters_1, 1, &taskHandle_1);
    taskPriority_1 = uxTaskPriorityGet(taskHandle_1);
    ESP_LOGI(TAG, "Task_1 优先级为: %d", taskPriority_1);

    // 传递数组参数
    xTaskCreate(Task_2, "Task_2", 2048, (void *)&Parameters_2, 2, &taskHandle_2);
    taskPriority_2 = uxTaskPriorityGet(taskHandle_2);
    ESP_LOGI(TAG, "Task_2 优先级为: %d", taskPriority_2);

    // 传递结构体参数
    xTaskCreate(Task_3, "Task_3", 2048, (void *)&Parameters_3, 1, NULL);

    // 传递字符串参数(注意这里没有取地址符号&,因为字符串是常量字符串,不能被修改)
    xTaskCreate(Task_4, "Task_4", 2048, (void *)Parameters_4, 1, NULL);
}

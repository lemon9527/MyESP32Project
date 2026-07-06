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

#define LED_GPIO    GPIO_NUM_2
#define BUTTON_GPIO GPIO_NUM_0

//static const char *TAG = "task_create_main";

void app_main(void)
{
    //初始化LED_GPIO为输出模式
    gpio_set_direction(LED_GPIO, GPIO_MODE_OUTPUT);

    //初始化BUTTON_GPIO为输入模式
    gpio_set_direction(BUTTON_GPIO, GPIO_MODE_INPUT);
    gpio_set_pull_mode(BUTTON_GPIO, GPIO_PULLUP_ONLY);

    while(1)
    {
        //读取BUTTON_GPIO状态
        int button_state = gpio_get_level(BUTTON_GPIO);
        if(button_state == 0)
        {
            //按钮被按下
            gpio_set_level(LED_GPIO, 1);
        }
        else
        {
            //按钮未被按下
            gpio_set_level(LED_GPIO, 0);
        }
        vTaskDelay(100 / portTICK_PERIOD_MS);
    }
}

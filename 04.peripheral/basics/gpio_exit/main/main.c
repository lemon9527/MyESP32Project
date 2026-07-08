#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"

#define LED_GPIO GPIO_NUM_2
#define BUTTON_GPIO GPIO_NUM_0

// 声明一个用于处理GPIO中断的函数
static void IRAM_ATTR gpio_isr_handler(void *arg)
{
    gpio_set_level(LED_GPIO, !gpio_get_level(BUTTON_GPIO));
}

void app_main(void)
{
    // 初始化LED GPIO为输出模式//
    gpio_reset_pin(LED_GPIO);
    gpio_set_direction(LED_GPIO, GPIO_MODE_OUTPUT);

    // 初始化按钮 GPIO为输入模式, 并设置为上拉
    gpio_reset_pin(BUTTON_GPIO);
    gpio_set_direction(BUTTON_GPIO, GPIO_MODE_INPUT);
    gpio_set_pull_mode(BUTTON_GPIO, GPIO_PULLUP_ENABLE);

    // 注册GPIO中断服务
    //gpio_isr_register(BUTTON_GPIO, gpio_isr_handler, NULL, GPIO_INTR_TYPE_EDGE_ANYEDGE, 0);

    // 安装GPIO服务
    gpio_install_isr_service(0);
    // 配置GPIO中断类型，并注册中断服务函数
    gpio_set_intr_type(BUTTON_GPIO, GPIO_INTR_ANYEDGE);
    gpio_isr_handler_add(BUTTON_GPIO, gpio_isr_handler, NULL);

    while (1)
    {
        //在中断模式下，主循环可以执行其他任务或进入睡眠状态
        vTaskDelay(portMAX_DELAY);// 使用portMAX_DELAY使任务永久挂起，直到中断唤醒
    }
    
}
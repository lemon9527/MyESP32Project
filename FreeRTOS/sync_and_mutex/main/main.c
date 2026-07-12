#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"


#define TAG "app_main"

static char sum_and_finish_flag = 0;
static int sum_total = 0;

void task1(void *pvParameters)
{
    for(int i = 0; i < 200; i++)
    {
        sum_total += i;
        esp_rom_delay_us(10*1000);
    }
    sum_and_finish_flag = 1;
    ESP_LOGI(TAG, "sum finished.");
    while(1)
    {
        vTaskDelay(1000);
    }
}

void task2(void *pvParameters)
{
    while(!sum_and_finish_flag);
    ESP_LOGI(TAG, "sum_total = %d", sum_total);
    sum_and_finish_flag = 0;
    while(1)
    {
        vTaskDelay(1000);
    }
}

void app_main(void)
{
    xTaskCreatePinnedToCore(task1, "task1", 2048, NULL, 3, NULL, 1);
    xTaskCreatePinnedToCore(task2, "task2", 2048, NULL, 3, NULL, 1);
}

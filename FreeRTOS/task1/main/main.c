#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"

#define TAG "task"

static TaskHandle_t task1_handle = NULL;
static TaskHandle_t task2_handle = NULL;


void task1(void *pvParameters)
{
    int cnt = 0;
    while (cnt++ < 5)
    {
        esp_rom_delay_us(200*1000);
        ESP_LOGI(TAG, "task1");
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
    ESP_LOGI(TAG, "task1 delete self");
    vTaskDelete(NULL);
}

void task2(void *pvParameters)
{
    int cnt = 0;
    TickType_t xLastWakeTime = xTaskGetTickCount();
    while (1)
    {
        esp_rom_delay_us(200*1000);
        ESP_LOGI(TAG, "task2");
        vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(1000));
        /*if(cnt++ == 5)
        {
            ESP_LOGI(TAG, "task2 delete task1");
            vTaskDelete(task1_handle);
        }*/
    }
}

void app_main(void)
{
    if(pdPASS != xTaskCreatePinnedToCore(task1, "task1", 2048, NULL, 3, &task1_handle, 0))
    {
        ESP_LOGE(TAG, "task1 create failed");
        return;
    }
    if(pdPASS != xTaskCreatePinnedToCore(task2, "task2", 2048, NULL, 3, &task2_handle, 1))
    {
        ESP_LOGE(TAG, "task2 create failed");
        return;
    }
}

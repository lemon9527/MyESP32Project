#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "list.h"

static list_t test_list;

static portMUX_TYPE spin_lock = portMUX_INITIALIZER_UNLOCKED;

#define TAG "critical"

void task1(void* param)
{
    while(1)
    {
        taskENTER_CRITICAL(&spin_lock);
        for(int i = 0;i<500;i++)
        {
            list_push_back(&test_list,i);
        }
        taskENTER_CRITICAL(&spin_lock);
        for(int i = 0;i<500;i++)
        {
            list_pop_back(&test_list);
        }
        taskEXIT_CRITICAL(&spin_lock);
        taskEXIT_CRITICAL(&spin_lock);
        vTaskDelay(pdMS_TO_TICKS(500));
        ESP_LOGI(TAG,"task1");
    }
}

void task2(void* param)
{
    while(1)
    {
        taskENTER_CRITICAL(&spin_lock);
        for(int i = 0;i<500;i++)
        {
            list_push_back(&test_list,i);
        }
        taskENTER_CRITICAL(&spin_lock);
        for(int i = 0;i<500;i++)
        {
            list_pop_back(&test_list);
        }
        taskEXIT_CRITICAL(&spin_lock);
        taskEXIT_CRITICAL(&spin_lock);
        vTaskDelay(pdMS_TO_TICKS(500));
        ESP_LOGI(TAG,"task2");
    }
}

void app_main(void)
{
    xTaskCreatePinnedToCore(task1,"task1",2048,NULL,3,NULL,0);
    xTaskCreatePinnedToCore(task2,"task2",2048,NULL,3,NULL,1);
}


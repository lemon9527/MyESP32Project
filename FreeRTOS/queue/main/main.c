#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "freertos/queue.h"

static QueueHandle_t test_queue = NULL;
#define TAG "Queue"

void task1(void* param)
{
    int send_num = 0;
    while(1)
    {
        if(pdTRUE != xQueueSend(test_queue,&send_num,pdMS_TO_TICKS(50)))
        {
            ESP_LOGE(TAG,"xQueueSend failed");
        }
        else
        {
            ESP_LOGI(TAG,"send_num = %d",send_num);
            send_num++;
        }
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

void task2(void* param)
{
    int receive_num = 0;
    while(1)
    {
        if(pdTRUE != xQueueReceive(test_queue,&receive_num,pdMS_TO_TICKS(50)))
        {
            ESP_LOGE(TAG,"xQueueReceive failed");
        }
        else
        {
            ESP_LOGI(TAG,"receive_num = %d",receive_num);
        }
        vTaskDelay(pdMS_TO_TICKS(200));
    }
}

void app_main(void)
{
    test_queue = xQueueCreate(3,sizeof(int));
    if(test_queue == NULL)
    {
        ESP_LOGE(TAG,"xQueueCreate failed");
        return;
    }
    xTaskCreatePinnedToCore(task1,"task1",2048,NULL,3,NULL,0);
    xTaskCreatePinnedToCore(task2,"task2",2048,NULL,3,NULL,1);
}


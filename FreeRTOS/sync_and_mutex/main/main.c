#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "list.h"
#include "freertos/queue.h"

#include "esp_timer.h"

#define TAG     "main"

static char sum_add_flag = 0;
static int sum_total = 0;
#if 0
void task1(void* param)
{
    for(int i = 0;i<200;i++)
    {
        sum_total += i;
        esp_rom_delay_us(10*1000);
    }
    sum_add_flag = 1;
    ESP_LOGI(TAG,"sum finish");
    while(1)
    {
        vTaskDelay(1000);
    }
}

void task2(void* param)
{
    while(!sum_add_flag);
    ESP_LOGI(TAG,"sum_total:%d",sum_total);
    sum_add_flag = 0;
    while(1)
    {
        vTaskDelay(1000);
    }
}
#endif

static list_t test_list;
static char list_flag = 0;

void task1(void* param)
{
    while(1)
    {
        if(!list_flag)
        {
            //esp_rom_delay_us(100*1000);
            list_flag = 1;
            for(int i = 0;i<500;i++)
            {
                list_push_back(&test_list,i);
            }
            for(int i = 0;i<500;i++)
            {
                list_pop_back(&test_list);
            }
            list_flag = 0;
        }

    }
}

void task2(void* param)
{
    while(1)
    {
        if(!list_flag)
        {
            //esp_rom_delay_us(100*1000);
            list_flag = 1;
            for(int i = 0;i<500;i++)
            {
                list_push_back(&test_list,i);
            }
            for(int i = 0;i<500;i++)
            {
                list_pop_back(&test_list);
            }
            list_flag = 0;
        }

    }
}

void app_main(void)
{
    xTaskCreatePinnedToCore(task1,"task1",2048,NULL,3,NULL,1);
    xTaskCreatePinnedToCore(task2,"task2",2048,NULL,3,NULL,1);
}


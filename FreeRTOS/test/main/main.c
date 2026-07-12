#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"


#define TAG "main"

int fun2(int b)
{
    unsigned char ch[100];
    int c = 0;
    for(int i = 0; i < 100; i++)
    {
        ch[i] = i;
        c += ch[i];
    }
    return c + b;
}

int fun1(int a)
{
    int a2 = 0;
    a2 = a + 2;
    return fun2(a2);
}
void app_main(void)
{
    int num1 = 5;
    int num2 = 0;
    num2 = fun1(num1);
    printf("num2 = %d\n", num2);

    unsigned int stack_value = uxTaskGetStackHighWaterMark(NULL);
    ESP_LOGI(TAG, "stack_value = %d", stack_value);
}

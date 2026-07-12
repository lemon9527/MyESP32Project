#include <stdio.h>
#include "driver/gptimer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "esp_err.h"
#include "esp_log.h"

static const char *TAG = "gptimer";

typedef struct{
    uint64_t event_count;
} example_queue_element_t;


void app_main(void)
{
    // 定义一个example_queue_element_t类型的变量 ele
    example_queue_element_t ele;
    // 创建了一个长度为10， 每个元素大小为 example_queue_element_t 的队列, 并将其赋值给queue变量
    QueueHandle_t queue = xQueueCreate(10, sizeof(example_queue_element_t));
    if(!queue){
        ESP_LOGE(TAG, "Queue create failed");
        return;
    }

    // 初始化定时器
    ESP_LOGW(TAG, "创建分辨率为1MHz的通用定时器句柄");
    gptimer_handle_t gptimer = NULL;
    gptimer_config_t timer_config = {
        .clk_src = GPTIMER_CLK_SRC_DEFAULT,
        .direction = GPTIMER_COUNT_UP,
        .resolution_hz = 1000000, // 1MHx, 1 tick = 1us
    };
    ESP_ERROR_CHECK(gptimer_new_timer(&timer_config, &gptimer));

    // 注册定时器事件回调函数(警报事件的回调函数)
    gptimer_event_callbacks_t cbs = {
        .on_alarm = example_timer_on_alarm_cb_v1,
    };
    ESP_ERROR_CHECK(gptimer_register_event_callbacks(gptimer, &cbs, NULL));



    //使能定时器
    ESP_LOGI(TAG, "Enable timer");
    ESP_ERROR_CHECK(gptimer_enable(gptimer));

    // 设置定时器周期为1S(触发警报事件的目标计数值)
    ESP_LOGI(TAG, "Set timer period to 1s");
    gptimer_alarm_config_t alarm_config1 = {
        .alarm_count = 1000000, //period = 1S

    };
    // 设置定时器警报事件
    ESP_ERROR_CHECK(gptimer_set_alarm_action(gptimer, &alarm_config1));
    // 启动定时器
    ESP_ERROR_CHECK(gptimer_start(gptimer));
    
}
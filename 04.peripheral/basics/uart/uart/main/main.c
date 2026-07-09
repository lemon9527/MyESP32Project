#include <stdio.h>
#include "driver/uart.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "esp_err.h"
#include "esp_log.h"
#include "string.h"

#define TX_GPIO_NUM 17
#define RX_GPIO_NUM 16

//配置串口
void uart_config(void)
{
    const uart_port_t uart_num = UART_NUM_2;

    uart_config_t uart_config = {
        .baud_rate = 115200,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .rx_flow_ctrl_thresh = 122,
    };
    
    // 配置 UART 参数
    ESP_ERROR_CHECK(uart_param_config(uart_num, &uart_config));

    // 配置 UART 引脚
    ESP_ERROR_CHECK(uart_set_pin(uart_num, TX_GPIO_NUM, RX_GPIO_NUM, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE));

    // 安装 UART 驱动程序
    const int uart_buffer_size = (1024 * 2);
    ESP_ERROR_CHECK(uart_driver_install(uart_num, uart_buffer_size, 0, 0, NULL, 0));
}

// 串口发送接收数据任务
void uart_send_receive_task(void *pvParameters)
{
    const uart_port_t uart_num = UART_NUM_2;
    char* test_str = "This is a UART test string!\n";
    uint8_t data[128];

    while(1){
        // 发送数据
        uart_write_bytes(uart_num, test_str, strlen(test_str));

        // 检查并接收数据
        int length = 0;
        ESP_ERROR_CHECK(uart_get_buffered_data_len(uart_num, (size_t*)&length));
        if(length > 0){
            int read_len = uart_read_bytes(uart_num, data, length, pdMS_TO_TICKS(1000));
            if(read_len > 0){
                printf("Received: %s", data);
            }
        }

        // 延时 1 秒
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

void app_main(void)
{
    uart_config();

    xTaskCreate(uart_send_receive_task, "uart_task", 2048, NULL, 5, NULL);
}
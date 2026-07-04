#include <stdio.h>
#include "esp_log.h"
#include "nvs_flash.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/i2c_master.h"
#include "sen68.h"
#include "wifi.h"
#include "cloud.h"

#define I2C_MASTER_SCL_IO           22
#define I2C_MASTER_SDA_IO           21
#define I2C_MASTER_FREQ_HZ          100000
#define I2C_MASTER_PORT             I2C_NUM_0

static const char *TAG = "aq_sensor";

static void aq_measurement_task(void *pvParameters)
{
    i2c_master_dev_handle_t dev_handle = (i2c_master_dev_handle_t)pvParameters;

    while (1) {
        bool ready = false;
        sen68_read_data_ready(dev_handle, &ready);
        if (ready) {
            sen68_data_t data;
            sen68_read_measurement(dev_handle, &data);
            /* TODO: cloud_publish for SEN68 */
        }
        vTaskDelay(pdMS_TO_TICKS(10000));
    }
}

void app_main(void)
{
    ESP_ERROR_CHECK(nvs_flash_init());
    wifi_init_sta();
    cloud_init();

    ESP_LOGI(TAG, "Initializing I2C bus...");

    i2c_master_bus_config_t i2c_mst_config = {
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .i2c_port = I2C_MASTER_PORT,
        .scl_io_num = I2C_MASTER_SCL_IO,
        .sda_io_num = I2C_MASTER_SDA_IO,
        .glitch_ignore_cnt = 7,
        .flags.enable_internal_pullup = true,
    };

    i2c_master_bus_handle_t bus_handle;
    ESP_ERROR_CHECK(i2c_new_master_bus(&i2c_mst_config, &bus_handle));
    ESP_LOGI(TAG, "I2C bus initialized successfully.");

    ESP_LOGI(TAG, "Probing for SEN68 sensor at address 0x%02X...", SEN68_SLAVE_ADDR);

    esp_err_t err = i2c_master_probe(bus_handle, SEN68_SLAVE_ADDR, -1);
    if (err == ESP_OK) {
        ESP_LOGI(TAG, "SEN68 sensor detected at address 0x%02X !", SEN68_SLAVE_ADDR);
    } else {
        ESP_LOGE(TAG, "SEN68 sensor NOT detected at address 0x%02X. Error: %s",
                 SEN68_SLAVE_ADDR, esp_err_to_name(err));
        ESP_ERROR_CHECK(i2c_del_master_bus(bus_handle));
        return;
    }

    i2c_device_config_t dev_cfg = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = SEN68_SLAVE_ADDR,
        .scl_speed_hz = I2C_MASTER_FREQ_HZ,
    };
    i2c_master_dev_handle_t dev_handle;
    ESP_ERROR_CHECK(i2c_master_bus_add_device(bus_handle, &dev_cfg, &dev_handle));

    sen68_read_product_name(dev_handle);
    sen68_read_serial_number(dev_handle);
    sen68_start_measurement(dev_handle);

    xTaskCreate(aq_measurement_task, "aq_meas", 4096, dev_handle, 5, NULL);
}
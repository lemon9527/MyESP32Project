#include <stdio.h>
#include "esp_log.h"
#include "nvs_flash.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/i2c_master.h"
#include "am2020.h"
#include "sen68.h"
#include "wifi.h"
#include "cloud.h"

#define I2C_MASTER_SCL_IO           22
#define I2C_MASTER_SDA_IO           21
#define I2C_MASTER_FREQ_HZ          100000
#define I2C_MASTER_PORT             I2C_NUM_0

static const char *TAG = "aq_sensor";

typedef struct {
    i2c_master_dev_handle_t am2020_handle;
    i2c_master_dev_handle_t sen68_handle;
    bool has_am2020;
    bool has_sen68;
} sensor_handles_t;

static void aq_measurement_task(void *pvParameters)
{
    sensor_handles_t *handles = (sensor_handles_t *)pvParameters;

    while (1) {
        if (handles->has_am2020) {
            am2020_data_t data;
            if (am2020_read_measurement(handles->am2020_handle, &data) == ESP_OK) {
                cloud_publish_am2020_measurement(&data);
            }
        }
        if (handles->has_sen68) {
            bool ready = false;
            sen68_read_data_ready(handles->sen68_handle, &ready);
            if (ready) {
                sen68_data_t sdata;
                if (sen68_read_measurement(handles->sen68_handle, &sdata) == ESP_OK) {
                    cloud_publish_sen68_measurement(&sdata);
                }
            }
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

    static sensor_handles_t handles = {0};

    ESP_LOGI(TAG, "Probing for AM2020 at 0x%02X...", AM2020_SLAVE_ADDR);
    if (i2c_master_probe(bus_handle, AM2020_SLAVE_ADDR, -1) == ESP_OK) {
        ESP_LOGI(TAG, "AM2020 detected !");
        handles.has_am2020 = true;
        i2c_device_config_t dev_cfg = {
            .dev_addr_length = I2C_ADDR_BIT_LEN_7,
            .device_address = AM2020_SLAVE_ADDR,
            .scl_speed_hz = I2C_MASTER_FREQ_HZ,
        };
        ESP_ERROR_CHECK(i2c_master_bus_add_device(bus_handle, &dev_cfg, &handles.am2020_handle));
        am2020_read_software_version(handles.am2020_handle);
        am2020_read_serial_number(handles.am2020_handle);
        am2020_read_product_name(handles.am2020_handle);
    } else {
        ESP_LOGI(TAG, "AM2020 not detected.");
    }

    ESP_LOGI(TAG, "Probing for SEN68 at 0x%02X...", SEN68_SLAVE_ADDR);
    if (i2c_master_probe(bus_handle, SEN68_SLAVE_ADDR, -1) == ESP_OK) {
        ESP_LOGI(TAG, "SEN68 detected !");
        handles.has_sen68 = true;
        i2c_device_config_t dev_cfg = {
            .dev_addr_length = I2C_ADDR_BIT_LEN_7,
            .device_address = SEN68_SLAVE_ADDR,
            .scl_speed_hz = I2C_MASTER_FREQ_HZ,
        };
        ESP_ERROR_CHECK(i2c_master_bus_add_device(bus_handle, &dev_cfg, &handles.sen68_handle));
        sen68_read_product_name(handles.sen68_handle);
        sen68_read_serial_number(handles.sen68_handle);
        sen68_set_voc_tuning_params(handles.sen68_handle);
        sen68_start_measurement(handles.sen68_handle);
    } else {
        ESP_LOGI(TAG, "SEN68 not detected.");
    }

    if (!handles.has_am2020 && !handles.has_sen68) {
        ESP_LOGE(TAG, "No sensor detected. Exiting.");
        ESP_ERROR_CHECK(i2c_del_master_bus(bus_handle));
        return;
    }

    xTaskCreate(aq_measurement_task, "aq_meas", 4096, &handles, 5, NULL);
}
#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include "esp_log.h"
#include "nvs_flash.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/i2c_master.h"
#include "am2020.h"
#include "sen6x.h"
#include "wifi.h"
#include "cloud.h"
#include "mcuc_uart.h"

#define I2C_MASTER_SCL_IO           22
#define I2C_MASTER_SDA_IO           21
#define I2C_MASTER_FREQ_HZ          100000
#define I2C_MASTER_PORT             I2C_NUM_0

static const char *TAG = "aq_sensor";

typedef struct {
    i2c_master_dev_handle_t am2020_handle;
    i2c_master_dev_handle_t sen6x_handle;
    bool has_am2020;
    bool has_sen6x;
    sen6x_type_t sen6x_type;
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
        if (handles->has_sen6x) {
            bool ready = false;
            sen6x_read_data_ready(handles->sen6x_handle, &ready);
            if (ready) {
                sen6x_data_t sdata = {0};
                if (sen6x_read_measurement(handles->sen6x_handle, &sdata, handles->sen6x_type) == ESP_OK) {
                    cloud_publish_sen6x_measurement(&sdata, handles->sen6x_type);
                }
            }
        }

        mcuc_data_t mcuc;
        if (mcuc_uart_read_frame(&mcuc) == ESP_OK) {
            cloud_publish_mcuc_measurement(&mcuc);
        }

        vTaskDelay(pdMS_TO_TICKS(20000));
    }
}
static void aq_dummy_task(void *pvParameters)
{
    ESP_LOGI(TAG, "Dummy data mode: simulating AM2020 + SEN66");

    int tick = 0;
    while (1) {
        tick++;
        float drift = 0.5f * sinf(tick * 0.3f) + ((float)(rand() % 100) / 100.0f - 0.5f);

        am2020_data_t a = {
            .tvoc = 120 + (int)(drift * 15),
            .no2 = 15 + (int)(drift * 3),
            .hcho = 28 + (int)(drift * 4),
            .pm1_0 = 8 + (int)(drift * 2),
            .pm2_5 = 12 + (int)(drift * 3),
            .pm10 = 14 + (int)(drift * 2),
            .temperature = 25.5f + drift * 1.5f,
            .humidity = 60.0f + drift * 5.0f,
        };
        cloud_publish_am2020_measurement(&a);

        vTaskDelay(pdMS_TO_TICKS(7000));

        drift = 0.5f * sinf(tick * 0.35f) + ((float)(rand() % 100) / 100.0f - 0.5f);

        sen6x_data_t s = {
            .pm1_0 = 10.5f + drift * 1.0f,
            .pm2_5 = 11.2f + drift * 1.5f,
            .pm4_0 = 11.0f + drift * 1.0f,
            .pm10 = 11.0f + drift * 1.0f,
            .temperature = 26.0f + drift * 1.0f,
            .humidity = 55.0f + drift * 4.0f,
            .voc_index = 80.0f + drift * 10.0f,
            .tvoc_ppb = 60.0f + drift * 8.0f,
            .nox_index = 1.0f + drift * 0.3f,
            .hcho = 0,
            .co2 = 620.0f + drift * 40.0f,
        };
        cloud_publish_sen6x_measurement(&s, SEN6X_TYPE_66);

        vTaskDelay(pdMS_TO_TICKS(13000));
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

    ESP_LOGI(TAG, "Probing for SEN6x at 0x%02X...", SEN6X_SLAVE_ADDR);
    if (i2c_master_probe(bus_handle, SEN6X_SLAVE_ADDR, -1) == ESP_OK) {
        handles.has_sen6x = true;
        i2c_device_config_t dev_cfg = {
            .dev_addr_length = I2C_ADDR_BIT_LEN_7,
            .device_address = SEN6X_SLAVE_ADDR,
            .scl_speed_hz = I2C_MASTER_FREQ_HZ,
        };
        ESP_ERROR_CHECK(i2c_master_bus_add_device(bus_handle, &dev_cfg, &handles.sen6x_handle));
        sen6x_init(handles.sen6x_handle, &handles.sen6x_type);
        sen6x_start_measurement(handles.sen6x_handle);
    } else {
        ESP_LOGI(TAG, "SEN6x not detected.");
    }

    if (!handles.has_am2020 && !handles.has_sen6x) {
        ESP_LOGW(TAG, "No sensor detected. Switching to dummy data mode.");
        ESP_ERROR_CHECK(i2c_del_master_bus(bus_handle));
        xTaskCreate(aq_dummy_task, "aq_dummy", 4096, NULL, 5, NULL);
        return;
    }

    mcuc_uart_init();
    xTaskCreate(aq_measurement_task, "aq_meas", 4096, &handles, 5, NULL);
}
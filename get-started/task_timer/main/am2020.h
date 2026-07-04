#pragma once

#include "esp_err.h"
#include "driver/i2c_master.h"

#define AM2020_SLAVE_ADDR               0x28

#define AM2020_CMD_READ_MEASUREMENT     0x16
#define AM2020_CMD_READ_SW_VERSION      0x1E
#define AM2020_CMD_READ_SERIAL          0x1F
#define AM2020_CMD_READ_PRODUCT_NAME    0x2E
#define AM2020_CMD_FAN_LASER            0x1C
#define AM2020_CMD_ZERO_CALIB           0x360B

#define AM2020_FRAME_HEAD               0x11

typedef struct {
    uint16_t tvoc;
    uint16_t no2;
    uint16_t pm1_0;
    uint16_t pm2_5;
    uint16_t pm10;
    uint16_t hcho;
    float temperature;
    float humidity;
} am2020_data_t;

esp_err_t am2020_read_software_version(i2c_master_dev_handle_t dev_handle);
esp_err_t am2020_read_serial_number(i2c_master_dev_handle_t dev_handle);
esp_err_t am2020_read_product_name(i2c_master_dev_handle_t dev_handle);
esp_err_t am2020_read_measurement(i2c_master_dev_handle_t dev_handle, am2020_data_t *out_data);
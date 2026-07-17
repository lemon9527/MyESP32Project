#pragma once

#include "esp_err.h"
#include "driver/i2c_master.h"

#define SEN6X_SLAVE_ADDR               0x6B

#define SEN6X_CMD_START_MEASUREMENT    0x0021
#define SEN6X_CMD_STOP_MEASUREMENT     0x0104
#define SEN6X_CMD_READ_DATA_READY      0x0202
#define SEN6X_CMD_READ_MEASUREMENT_68  0x0467
#define SEN6X_CMD_READ_MEASUREMENT_66  0x0300
#define SEN6X_CMD_READ_PRODUCT_NAME    0xD014
#define SEN6X_CMD_READ_SERIAL_NUMBER   0xD033
#define SEN6X_CMD_SET_VOC_TUNING       0x60D0

typedef enum {
    SEN6X_TYPE_UNKNOWN = 0,
    SEN6X_TYPE_68,
    SEN6X_TYPE_66,
} sen6x_type_t;

typedef struct {
    float pm1_0;
    float pm2_5;
    float pm4_0;
    float pm10;
    float humidity;
    float temperature;
    float voc_index;
    float tvoc_ppb;
    float nox_index;
    float hcho;
    float co2;
} sen6x_data_t;

esp_err_t sen6x_init(i2c_master_dev_handle_t dev_handle, sen6x_type_t *out_type);
esp_err_t sen6x_start_measurement(i2c_master_dev_handle_t dev_handle);
esp_err_t sen6x_read_data_ready(i2c_master_dev_handle_t dev_handle, bool *ready);
esp_err_t sen6x_read_measurement(i2c_master_dev_handle_t dev_handle, sen6x_data_t *out_data, sen6x_type_t type);
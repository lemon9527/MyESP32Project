#include <stdio.h>
#include <string.h>
#include <math.h>
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/i2c_master.h"
#include "sen68.h"

static const char *TAG = "sen68";

static uint8_t sen68_crc8(const uint8_t *data, size_t len)
{
    uint8_t crc = 0xFF;
    for (size_t i = 0; i < len; i++) {
        crc ^= data[i];
        for (int j = 0; j < 8; j++) {
            if (crc & 0x80) {
                crc = (uint8_t)((crc << 1) ^ 0x31);
            } else {
                crc <<= 1;
            }
        }
    }
    return crc;
}

static esp_err_t sen68_send_command(i2c_master_dev_handle_t dev_handle, uint16_t cmd)
{
    uint8_t tx_buf[2];
    tx_buf[0] = (cmd >> 8) & 0xFF;
    tx_buf[1] = cmd & 0xFF;
    return i2c_master_transmit(dev_handle, tx_buf, sizeof(tx_buf), -1);
}

esp_err_t sen68_read_product_name(i2c_master_dev_handle_t dev_handle)
{
    esp_err_t err = sen68_send_command(dev_handle, SEN68_CMD_READ_PRODUCT_NAME);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to send read product name command: %s", esp_err_to_name(err));
        return err;
    }

    vTaskDelay(pdMS_TO_TICKS(20));

    uint8_t rx_buf[48] = {0};
    err = i2c_master_receive(dev_handle, rx_buf, sizeof(rx_buf), -1);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to read product name: %s", esp_err_to_name(err));
        return err;
    }

    char name[33];
    int pos = 0;
    for (int i = 0; i < 16 && pos < 32; i++) {
        uint8_t crc_calc = sen68_crc8(&rx_buf[i * 3], 2);
        if (crc_calc != rx_buf[i * 3 + 2]) {
            ESP_LOGW(TAG, "Product name CRC mismatch at word %d", i);
        }
        name[pos++] = (char)rx_buf[i * 3];
        name[pos++] = (char)rx_buf[i * 3 + 1];
    }
    name[pos] = '\0';

    ESP_LOGI(TAG, "Product name: %s", name);
    return ESP_OK;
}

esp_err_t sen68_read_serial_number(i2c_master_dev_handle_t dev_handle)
{
    esp_err_t err = sen68_send_command(dev_handle, SEN68_CMD_READ_SERIAL_NUMBER);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to send read serial number command: %s", esp_err_to_name(err));
        return err;
    }

    vTaskDelay(pdMS_TO_TICKS(20));

    uint8_t rx_buf[48] = {0};
    err = i2c_master_receive(dev_handle, rx_buf, sizeof(rx_buf), -1);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to read serial number: %s", esp_err_to_name(err));
        return err;
    }

    char serial[33];
    int pos = 0;
    for (int i = 0; i < 16 && pos < 32; i++) {
        uint8_t crc_calc = sen68_crc8(&rx_buf[i * 3], 2);
        if (crc_calc != rx_buf[i * 3 + 2]) {
            ESP_LOGW(TAG, "Serial number CRC mismatch at word %d", i);
        }
        serial[pos++] = (char)rx_buf[i * 3];
        serial[pos++] = (char)rx_buf[i * 3 + 1];
    }
    serial[pos] = '\0';

    ESP_LOGI(TAG, "Serial number: %s", serial);
    return ESP_OK;
}

esp_err_t sen68_start_measurement(i2c_master_dev_handle_t dev_handle)
{
    esp_err_t err = sen68_send_command(dev_handle, SEN68_CMD_START_MEASUREMENT);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to start measurement: %s", esp_err_to_name(err));
        return err;
    }

    ESP_LOGI(TAG, "Measurement started, waiting for first data...");
    vTaskDelay(pdMS_TO_TICKS(1100));
    return ESP_OK;
}

esp_err_t sen68_stop_measurement(i2c_master_dev_handle_t dev_handle)
{
    esp_err_t err = sen68_send_command(dev_handle, SEN68_CMD_STOP_MEASUREMENT);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to stop measurement: %s", esp_err_to_name(err));
        return err;
    }

    vTaskDelay(pdMS_TO_TICKS(1400));
    return ESP_OK;
}

esp_err_t sen68_read_data_ready(i2c_master_dev_handle_t dev_handle, bool *ready)
{
    esp_err_t err = sen68_send_command(dev_handle, SEN68_CMD_READ_DATA_READY);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to send data ready command: %s", esp_err_to_name(err));
        return err;
    }

    vTaskDelay(pdMS_TO_TICKS(20));

    uint8_t rx_buf[3] = {0};
    err = i2c_master_receive(dev_handle, rx_buf, sizeof(rx_buf), -1);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to read data ready: %s", esp_err_to_name(err));
        return err;
    }

    uint8_t crc_calc = sen68_crc8(rx_buf, 2);
    if (crc_calc != rx_buf[2]) {
        ESP_LOGW(TAG, "Data ready CRC mismatch: calc=0x%02X, recv=0x%02X", crc_calc, rx_buf[2]);
    }

    *ready = (rx_buf[1] == 0x01);
    return ESP_OK;
}

esp_err_t sen68_read_measurement(i2c_master_dev_handle_t dev_handle, sen68_data_t *out_data)
{
    esp_err_t err = sen68_send_command(dev_handle, SEN68_CMD_READ_MEASUREMENT);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to send measurement command: %s", esp_err_to_name(err));
        return err;
    }

    vTaskDelay(pdMS_TO_TICKS(20));

    uint8_t rx_buf[27] = {0};
    err = i2c_master_receive(dev_handle, rx_buf, sizeof(rx_buf), -1);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to read measurement: %s", esp_err_to_name(err));
        return err;
    }

    ESP_LOG_BUFFER_HEX_LEVEL(TAG, rx_buf, sizeof(rx_buf), ESP_LOG_DEBUG);

    uint16_t raw[9];
    for (int i = 0; i < 9; i++) {
        uint8_t crc_calc = sen68_crc8(&rx_buf[i * 3], 2);
        if (crc_calc != rx_buf[i * 3 + 2]) {
            ESP_LOGW(TAG, "Measurement CRC mismatch at word %d: calc=0x%02X, recv=0x%02X",
                     i, crc_calc, rx_buf[i * 3 + 2]);
        }
        raw[i] = ((uint16_t)rx_buf[i * 3] << 8) | rx_buf[i * 3 + 1];
    }

    float pm1_0  = raw[0] / 10.0f;
    float pm2_5  = raw[1] / 10.0f;
    float pm4_0  = raw[2] / 10.0f;
    float pm10   = raw[3] / 10.0f;
    float hum    = (int16_t)raw[4] / 100.0f;
    float temp   = (int16_t)raw[5] / 200.0f;
    float voc    = (int16_t)raw[6] / 10.0f;
    float nox    = (int16_t)raw[7] / 10.0f;
    float hcho   = (int16_t)raw[8] / 10.0f;

    ESP_LOGI(TAG, "--- Measurement ---");
    ESP_LOGI(TAG, "PM1.0: %.1f μg/m³,  PM2.5: %.1f μg/m³,  PM4.0: %.1f μg/m³,  PM10: %.1f μg/m³",
             pm1_0, pm2_5, pm4_0, pm10);
    ESP_LOGI(TAG, "Temperature: %.1f °C,  Humidity: %.1f %%", temp, hum);
    float voc_clamped = (voc > 500.0f) ? 500.0f : voc;
    float tvoc_ppb = (logf(501.0f - voc_clamped) - 6.24f) * (-313.6f);

    ESP_LOGI(TAG, "VOC Index: %.1f -> TVOC: %.1f ppb,  NOx Index: %.1f,  HCHO: %.1f ppb", voc, tvoc_ppb, nox, hcho);

    if (out_data) {
        out_data->pm1_0       = pm1_0;
        out_data->pm2_5       = pm2_5;
        out_data->pm4_0       = pm4_0;
        out_data->pm10        = pm10;
        out_data->humidity    = hum;
        out_data->temperature = temp;
        out_data->voc_index   = voc;
        out_data->tvoc_ppb    = tvoc_ppb;
        out_data->nox_index   = nox;
        out_data->hcho        = hcho;
    }
    return ESP_OK;
}
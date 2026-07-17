#include <stdio.h>
#include <string.h>
#include <math.h>
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/i2c_master.h"
#include "sen6x.h"

static const char *TAG = "sen6x";

static uint8_t sen6x_crc8(const uint8_t *data, size_t len)
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

static esp_err_t sen6x_send_command(i2c_master_dev_handle_t dev_handle, uint16_t cmd)
{
    uint8_t tx_buf[2];
    tx_buf[0] = (cmd >> 8) & 0xFF;
    tx_buf[1] = cmd & 0xFF;
    return i2c_master_transmit(dev_handle, tx_buf, sizeof(tx_buf), -1);
}

static esp_err_t sen6x_read_string(i2c_master_dev_handle_t dev_handle, uint16_t cmd, char *out, size_t max_len)
{
    esp_err_t err = sen6x_send_command(dev_handle, cmd);
    if (err != ESP_OK) return err;

    vTaskDelay(pdMS_TO_TICKS(20));

    uint8_t rx_buf[48] = {0};
    err = i2c_master_receive(dev_handle, rx_buf, sizeof(rx_buf), -1);
    if (err != ESP_OK) return err;

    int pos = 0;
    for (int i = 0; i < 16 && pos < (int)(max_len - 1); i++) {
        uint8_t crc_calc = sen6x_crc8(&rx_buf[i * 3], 2);
        if (crc_calc != rx_buf[i * 3 + 2]) {
            ESP_LOGW(TAG, "CRC mismatch at word %d", i);
        }
        out[pos++] = (char)rx_buf[i * 3];
        out[pos++] = (char)rx_buf[i * 3 + 1];
    }
    out[pos] = '\0';
    return ESP_OK;
}

esp_err_t sen6x_init(i2c_master_dev_handle_t dev_handle, sen6x_type_t *out_type)
{
    char name[33] = {0};
    esp_err_t err = sen6x_read_string(dev_handle, SEN6X_CMD_READ_PRODUCT_NAME, name, sizeof(name));
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to read product name");
        return err;
    }

    ESP_LOGI(TAG, "Product name: %s", name);

    if (strstr(name, "SEN66")) {
        *out_type = SEN6X_TYPE_66;
        ESP_LOGI(TAG, "Detected SEN66");
    } else if (strstr(name, "SEN68")) {
        *out_type = SEN6X_TYPE_68;
        ESP_LOGI(TAG, "Detected SEN68");
    } else {
        ESP_LOGW(TAG, "Unknown sensor: %s, assuming SEN68", name);
        *out_type = SEN6X_TYPE_68;
    }

    char serial[33] = {0};
    sen6x_read_string(dev_handle, SEN6X_CMD_READ_SERIAL_NUMBER, serial, sizeof(serial));
    ESP_LOGI(TAG, "Serial number: %s", serial);

    int16_t params[6] = {100, 72, 72, 180, 50, 230};
    uint8_t tx_buf[20];
    tx_buf[0] = (SEN6X_CMD_SET_VOC_TUNING >> 8) & 0xFF;
    tx_buf[1] = SEN6X_CMD_SET_VOC_TUNING & 0xFF;
    for (int i = 0; i < 6; i++) {
        int idx = 2 + i * 3;
        tx_buf[idx]     = (params[i] >> 8) & 0xFF;
        tx_buf[idx + 1] = params[i] & 0xFF;
        tx_buf[idx + 2] = sen6x_crc8(&tx_buf[idx], 2);
    }
    err = i2c_master_transmit(dev_handle, tx_buf, sizeof(tx_buf), -1);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to set VOC tuning params");
        return err;
    }
    ESP_LOGI(TAG, "VOC tuning parameters set");

    return ESP_OK;
}

esp_err_t sen6x_start_measurement(i2c_master_dev_handle_t dev_handle)
{
    esp_err_t err = sen6x_send_command(dev_handle, SEN6X_CMD_START_MEASUREMENT);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to start measurement: %s", esp_err_to_name(err));
        return err;
    }
    ESP_LOGI(TAG, "Measurement started, waiting for first data...");
    vTaskDelay(pdMS_TO_TICKS(1100));
    return ESP_OK;
}

esp_err_t sen6x_read_data_ready(i2c_master_dev_handle_t dev_handle, bool *ready)
{
    esp_err_t err = sen6x_send_command(dev_handle, SEN6X_CMD_READ_DATA_READY);
    if (err != ESP_OK) return err;

    vTaskDelay(pdMS_TO_TICKS(20));

    uint8_t rx_buf[3] = {0};
    err = i2c_master_receive(dev_handle, rx_buf, sizeof(rx_buf), -1);
    if (err != ESP_OK) return err;

    uint8_t crc_calc = sen6x_crc8(rx_buf, 2);
    if (crc_calc != rx_buf[2]) {
        ESP_LOGW(TAG, "Data ready CRC mismatch");
    }

    *ready = (rx_buf[1] == 0x01);
    return ESP_OK;
}

esp_err_t sen6x_read_measurement(i2c_master_dev_handle_t dev_handle, sen6x_data_t *out_data, sen6x_type_t type)
{
    uint16_t cmd = (type == SEN6X_TYPE_66) ? SEN6X_CMD_READ_MEASUREMENT_66 : SEN6X_CMD_READ_MEASUREMENT_68;

    esp_err_t err = sen6x_send_command(dev_handle, cmd);
    if (err != ESP_OK) return err;

    vTaskDelay(pdMS_TO_TICKS(20));

    uint8_t rx_buf[27] = {0};
    err = i2c_master_receive(dev_handle, rx_buf, sizeof(rx_buf), -1);
    if (err != ESP_OK) return err;

    uint16_t raw[9];
    for (int i = 0; i < 9; i++) {
        uint8_t crc_calc = sen6x_crc8(&rx_buf[i * 3], 2);
        if (crc_calc != rx_buf[i * 3 + 2]) {
            ESP_LOGW(TAG, "CRC mismatch at word %d: calc=0x%02X, recv=0x%02X",
                     i, crc_calc, rx_buf[i * 3 + 2]);
        }
        raw[i] = ((uint16_t)rx_buf[i * 3] << 8) | rx_buf[i * 3 + 1];
    }

    float pm1_0 = raw[0] / 10.0f;
    float pm2_5 = raw[1] / 10.0f;
    float pm4_0 = raw[2] / 10.0f;
    float pm10  = raw[3] / 10.0f;
    float hum   = (int16_t)raw[4] / 100.0f;
    float temp  = (int16_t)raw[5] / 200.0f;
    float voc   = (int16_t)raw[6] / 10.0f;
    float nox   = (int16_t)raw[7] / 10.0f;
    float hcho  = (int16_t)raw[8] / 10.0f;
    float co2   = (uint16_t)raw[8];

    float voc_clamped = (voc > 500.0f) ? 500.0f : voc;
    float tvoc_ppb = (logf(501.0f - voc_clamped) - 6.24f) * (-313.6f);

    ESP_LOGI(TAG, "--- Measurement (%s) ---", (type == SEN6X_TYPE_66) ? "SEN66" : "SEN68");
    ESP_LOGI(TAG, "PM1.0: %.1f  PM2.5: %.1f  PM4.0: %.1f  PM10: %.1f", pm1_0, pm2_5, pm4_0, pm10);
    ESP_LOGI(TAG, "Temperature: %.1f °C  Humidity: %.1f %%", temp, hum);
    ESP_LOGI(TAG, "VOC: %.1f -> TVOC: %.1f ppb  NOx: %.1f", voc, tvoc_ppb, nox);

    if (type == SEN6X_TYPE_66) {
        ESP_LOGI(TAG, "CO2: %.1f ppm", co2);
    } else {
        ESP_LOGI(TAG, "HCHO: %.1f ppb", hcho);
    }

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
        out_data->hcho        = (type == SEN6X_TYPE_68) ? hcho : 0;
        out_data->co2         = (type == SEN6X_TYPE_66) ? co2  : 0;
    }
    return ESP_OK;
}
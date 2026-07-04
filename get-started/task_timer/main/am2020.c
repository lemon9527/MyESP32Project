#include <stdio.h>
#include <string.h>
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/i2c_master.h"
#include "am2020.h"

static const char *TAG = "am2020";

/*
  Commands for the AM2020 combo sensor. (ref SPECIFICATION of Cubic AM2020DY V0.5)

  ## Protocol detailed description
  # Send Control Command
  START+WRITE+ACK+P1+ACK+P2+ACK...... +Pn+ACK+CS+ACK+STOP
  example
  50 11 01 16 D8

  Byte   Meaning                      Description
  0x50   Sensor address + write bit   Upper 7 bits are address, LSB is read/write flag (write here)
  0x11   Frame head                   Fixed identifier marking start of command frame
  0x01   Frame length                 Indicates the following command field is 1 byte
  0x16   Command byte                 "Read measurement result" command
  0xD8   Checksum                     Verifies correctness of all preceding bytes


  # Send Reading Command
  START+READ+ACK+P1+ACK+P2+ACK...... +Pn+ACK+CS+NCK+STOP

  example
  51
  0x51 = address + read bit (LSB = 1)
*/

static uint8_t am2020_checksum(const uint8_t *data, size_t len)
{
    uint16_t sum = 0;
    for (size_t i = 0; i < len; i++) {
        sum += data[i];
    }
    return (uint8_t)(0x100 - (sum & 0xFF));
}

static esp_err_t am2020_send_command(i2c_master_dev_handle_t dev_handle, uint8_t cmd)
{
    uint8_t tx_buf[4];
    tx_buf[0] = AM2020_FRAME_HEAD;
    tx_buf[1] = 0x01;
    tx_buf[2] = cmd;
    tx_buf[3] = am2020_checksum(tx_buf, 3);

    return i2c_master_transmit(dev_handle, tx_buf, sizeof(tx_buf), -1);
}

/*
  ## 2 Read Software Version

    Send: 50 11 01 1E D0

    Response: 16 0E 1E DF1~DF13 [CS]
    Software version = "DF1~DF13" (HEX to ASCII)
    Example: 16 0E 1E 50 4D 20 56 31 2E 31 39 2E 30 2E 30 33 F3
             -> "PM V1.19.0.03"
*/
esp_err_t am2020_read_software_version(i2c_master_dev_handle_t dev_handle)
{
    esp_err_t err = am2020_send_command(dev_handle, AM2020_CMD_READ_SW_VERSION);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to send read software version command: %s", esp_err_to_name(err));
        return err;
    }

    vTaskDelay(pdMS_TO_TICKS(50));

    uint8_t rx_buf[17] = {0};
    err = i2c_master_receive(dev_handle, rx_buf, sizeof(rx_buf), -1);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to read response: %s", esp_err_to_name(err));
        return err;
    }

    ESP_LOG_BUFFER_HEX_LEVEL(TAG, rx_buf, sizeof(rx_buf), ESP_LOG_DEBUG);

    uint8_t version[14];
    memcpy(version, &rx_buf[3], 13);
    version[13] = '\0';

    ESP_LOGI(TAG, "Software version: %s", version);
    return ESP_OK;
}

/*
  ## 3 Read Serial Number

    Send: 50 11 01 1F CF

    Response: 16 0B 1F DF1 DF2 DF3 DF4 DF5 DF6 DF7 DF8 DF9 DF10 CS
    Serial number = (DF1*256+DF2), (DF3*256+DF4), (DF5*256+DF6), (DF7*256+DF8), (DF9*256+DF10)
    Example: 16 0B 1F 00 00 00 7E 09 07 07 0E 0D 72 9E
             -> 0126 2311 1806 3442
*/
esp_err_t am2020_read_serial_number(i2c_master_dev_handle_t dev_handle)
{
    esp_err_t err = am2020_send_command(dev_handle, AM2020_CMD_READ_SERIAL);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to send read serial number command: %s", esp_err_to_name(err));
        return err;
    }

    vTaskDelay(pdMS_TO_TICKS(50));

    uint8_t rx_buf[14] = {0};
    err = i2c_master_receive(dev_handle, rx_buf, sizeof(rx_buf), -1);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to read response: %s", esp_err_to_name(err));
        return err;
    }

    ESP_LOG_BUFFER_HEX_LEVEL(TAG, rx_buf, sizeof(rx_buf), ESP_LOG_DEBUG);

    uint16_t sn[5];
    for (int i = 0; i < 5; i++) {
        sn[i] = ((uint16_t)rx_buf[3 + i * 2] << 8) | rx_buf[4 + i * 2];
    }

    ESP_LOGI(TAG, "Serial number: %04X %04X %04X %04X %04X",
             sn[0], sn[1], sn[2], sn[3], sn[4]);
    return ESP_OK;
}

/*
  ## 4 Read Product Name / Sensor Model

    Send: 50 11 01 2E C0

    Response: 16 09 2E DF1~DF6 CS (10 bytes total)
    DF1~DF6 is ASCII code, e.g. 41 4D 32 30 30 39 -> "AM2009"
*/
esp_err_t am2020_read_product_name(i2c_master_dev_handle_t dev_handle)
{
    esp_err_t err = am2020_send_command(dev_handle, AM2020_CMD_READ_PRODUCT_NAME);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to send read product name command: %s", esp_err_to_name(err));
        return err;
    }

    vTaskDelay(pdMS_TO_TICKS(65));

    uint8_t rx_buf[10] = {0};
    err = i2c_master_receive(dev_handle, rx_buf, sizeof(rx_buf), -1);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to read response: %s", esp_err_to_name(err));
        return err;
    }

    ESP_LOG_BUFFER_HEX_LEVEL(TAG, rx_buf, sizeof(rx_buf), ESP_LOG_DEBUG);

    char model[7];
    memcpy(model, &rx_buf[3], 6);
    model[6] = '\0';

    ESP_LOGI(TAG, "Product name: %s", model);
    return ESP_OK;
}

/*
  ## 1 Measuring Result

    Send: 50 11 01 16 D8

    Response: 16 15 16 DF1~DF22 CS
    DF1-DF2: TVOC (ppb)        DF11-DF12: Temperature = (raw-500)/10 °C
    DF3-DF4: NO2 (ppb)         DF13-DF14: Humidity = raw/10 %
    DF5-DF6: PM1.0 (μg/m³)     DF19-DF20: HCHO (ppb)
    DF7-DF8: PM2.5 (μg/m³)     DF21-DF22: Fault detection
    DF9-DF10: PM10 (μg/m³)
*/
esp_err_t am2020_read_measurement(i2c_master_dev_handle_t dev_handle, am2020_data_t *out_data)
{
    esp_err_t err = am2020_send_command(dev_handle, AM2020_CMD_READ_MEASUREMENT);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to send measurement command: %s", esp_err_to_name(err));
        return err;
    }

    vTaskDelay(pdMS_TO_TICKS(50));

    uint8_t rx_buf[32] = {0};
    err = i2c_master_receive(dev_handle, rx_buf, sizeof(rx_buf), -1);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to read measurement: %s", esp_err_to_name(err));
        return err;
    }

    uint8_t data_len = rx_buf[1];
    uint8_t cs_pos   = 2 + data_len;

    ESP_LOG_BUFFER_HEX_LEVEL(TAG, rx_buf, cs_pos + 1, ESP_LOG_DEBUG);

    uint8_t cs_calc = am2020_checksum(rx_buf, cs_pos);
    if (cs_calc != rx_buf[cs_pos]) {
        ESP_LOGE(TAG, "Checksum mismatch: calc=0x%02X, recv=0x%02X", cs_calc, rx_buf[cs_pos]);
        return ESP_FAIL;
    }

    uint16_t tvoc     = ((uint16_t)rx_buf[3]  << 8) | rx_buf[4];
    uint16_t no2      = ((uint16_t)rx_buf[5]  << 8) | rx_buf[6];
    uint16_t pm1_0    = ((uint16_t)rx_buf[7]  << 8) | rx_buf[8];
    uint16_t pm2_5    = ((uint16_t)rx_buf[9]  << 8) | rx_buf[10];
    uint16_t pm10     = ((uint16_t)rx_buf[11] << 8) | rx_buf[12];
    int16_t  temp_raw = ((uint16_t)rx_buf[13] << 8) | rx_buf[14];
    uint16_t hum_raw  = ((uint16_t)rx_buf[15] << 8) | rx_buf[16];
    uint16_t hcho     = ((uint16_t)rx_buf[21] << 8) | rx_buf[22];
    uint8_t  fault1   = rx_buf[23];
    uint8_t  fault2   = rx_buf[24];

    if (fault1 != 0 || fault2 != 0) {
        ESP_LOGW(TAG, "Fault detected! DF21=0x%02X DF22=0x%02X", fault1, fault2);
        if (fault1 & 0x80) ESP_LOGW(TAG, "  - Fan speed faulty");
        if (fault1 & 0x40) ESP_LOGW(TAG, "  - PM signal faulty");
        if (fault1 & 0x20) ESP_LOGW(TAG, "  - VOC working temp faulty");
        if (fault1 & 0x10) ESP_LOGW(TAG, "  - VOC sensitive material faulty");
        if (fault1 & 0x08) ESP_LOGW(TAG, "  - NO2 working temp faulty");
        if (fault1 & 0x04) ESP_LOGW(TAG, "  - NO2 sensitive material faulty");
        if (fault1 & 0x02) ESP_LOGW(TAG, "  - HCHO signal channel faulty");
        if (fault1 & 0x01) ESP_LOGW(TAG, "  - HCHO reference channel faulty");
        if (fault2 & 0x80) ESP_LOGW(TAG, "  - RHT faulty (I2C/CRC)");
    }

    float temp = (temp_raw - 500) / 10.0f;
    float hum  = hum_raw / 10.0f;

    ESP_LOGI(TAG, "--- Measurement ---");
    ESP_LOGI(TAG, "TVOC: %u ppb,  NO2: %u ppb,  HCHO: %u ppb", tvoc, no2, hcho);
    ESP_LOGI(TAG, "PM1.0: %u μg/m³,  PM2.5: %u μg/m³,  PM10: %u μg/m³", pm1_0, pm2_5, pm10);
    ESP_LOGI(TAG, "Temperature: %.1f °C,  Humidity: %.1f %%", temp, hum);

    if (out_data) {
        out_data->tvoc        = tvoc;
        out_data->no2         = no2;
        out_data->pm1_0       = pm1_0;
        out_data->pm2_5       = pm2_5;
        out_data->pm10        = pm10;
        out_data->hcho        = hcho;
        out_data->temperature = temp;
        out_data->humidity    = hum;
    }
    return ESP_OK;
}
#include <stdio.h>
#include <string.h>
#include "esp_log.h"
#include "driver/uart.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "mcuc_uart.h"

static const char *TAG = "mcuc_uart";

esp_err_t mcuc_uart_init(void)
{
    uart_config_t uart_cfg = {
        .baud_rate = MCUC_UART_BAUD,
        .data_bits = UART_DATA_8_BITS,
        .parity    = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_DEFAULT,
    };

    ESP_ERROR_CHECK(uart_driver_install(MCUC_UART_PORT, 2048, 0, 0, NULL, 0));
    ESP_ERROR_CHECK(uart_param_config(MCUC_UART_PORT, &uart_cfg));
    ESP_ERROR_CHECK(uart_set_pin(MCUC_UART_PORT, MCUC_UART_TX, MCUC_UART_RX,
                                  UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE));

    ESP_LOGI(TAG, "UART%d init OK (TX=%d, RX=%d, %d baud)",
             MCUC_UART_PORT, MCUC_UART_TX, MCUC_UART_RX, MCUC_UART_BAUD);
    return ESP_OK;
}

static uint16_t read_u16_le(const uint8_t *buf)
{
    return (uint16_t)buf[0] | ((uint16_t)buf[1] << 8);
}

static uint16_t mcuc_checksum16(const uint8_t *data, size_t len)
{
    uint16_t sum = 0;
    for (size_t i = 0; i < len; i++) {
        sum += data[i];
    }
    return sum;
}

static void mcuc_parse_fields(const uint8_t *buf, mcuc_data_t *out)
{
    memset(out, 0, sizeof(*out));
    int i = 0;
    out->pms_in_pm1_0   = read_u16_le(buf + i); i += 2;
    out->pms_in_pm2_5   = read_u16_le(buf + i); i += 2;
    out->pms_in_pm10    = read_u16_le(buf + i); i += 2;
    out->pms_out_pm1_0  = read_u16_le(buf + i); i += 2;
    out->pms_out_pm2_5  = read_u16_le(buf + i); i += 2;
    out->pms_out_pm10   = read_u16_le(buf + i); i += 2;
    out->temperature    = read_u16_le(buf + i); i += 2;
    out->humidity       = read_u16_le(buf + i); i += 2;
    out->tvoc_count     = read_u16_le(buf + i); i += 2;
    out->aq_state       = read_u16_le(buf + i); i += 2;
    out->audio_state    = read_u16_le(buf + i); i += 2;
    out->pir_state      = read_u16_le(buf + i); i += 2;
    out->co2_count      = read_u16_le(buf + i); i += 2;
    out->light_state    = read_u16_le(buf + i); i += 2;
    out->pressure_count = read_u16_le(buf + i); i += 2;
    out->reserved1      = read_u16_le(buf + i); i += 2;
    out->reserved2      = read_u16_le(buf + i); i += 2;
    out->reserved3      = read_u16_le(buf + i); i += 2;
    out->ave_state_sum  = read_u16_le(buf + i); i += 2;
}

esp_err_t mcuc_uart_read_frame(mcuc_data_t *out_data)
{
    /*
     * 帧格式: HEAD(2 LE) | LEN(2 LE) | FIELDS(N×2 LE) | CS(2 LE)
     * LEN = 整帧字节数
     * CS  = sum(帧头 ~ 最后一个字段) 的 uint16_t
     */
    uint8_t buf[MCUC_MAX_FRAME];

    /* ---------- 状态机: 逐字节找帧头 2D 23 ---------- */
    int state = 0;
    uint8_t ch;

    while (1) {
        int n = uart_read_bytes(MCUC_UART_PORT, &ch, 1, pdMS_TO_TICKS(100));
        if (n <= 0) return ESP_ERR_TIMEOUT;

        if (state == 0 && ch == MCUC_HEAD_LO) state = 1;
        else if (state == 1) state = (ch == MCUC_HEAD_HI) ? 2 : (ch == MCUC_HEAD_LO ? 1 : 0);
        if (state == 2) break;
    }

    buf[0] = MCUC_HEAD_LO;
    buf[1] = MCUC_HEAD_HI;

    /* ---------- 读 length (2 bytes LE) ---------- */
    int n = uart_read_bytes(MCUC_UART_PORT, buf + 2, 2, pdMS_TO_TICKS(200));
    if (n < 2) {
        ESP_LOGW(TAG, "Timeout reading length");
        return ESP_ERR_TIMEOUT;
    }

    uint16_t frame_len = read_u16_le(buf + 2);
    if (frame_len < 8 || frame_len > MCUC_MAX_FRAME) {
        ESP_LOGW(TAG, "Invalid frame length: %u", frame_len);
        return ESP_FAIL;
    }

    /* ---------- 读剩余部分 (fields + checksum) ---------- */
    int remain = frame_len - 4; /* 减去 head + length */
    n = uart_read_bytes(MCUC_UART_PORT, buf + 4, remain, pdMS_TO_TICKS(500));
    if (n < remain) {
        ESP_LOGW(TAG, "Short frame: need %d, got %d", remain, n);
        return ESP_ERR_TIMEOUT;
    }

    /* ---------- 校验 ---------- */
    uint16_t cs_recv = read_u16_le(buf + frame_len - 2);
    uint16_t cs_calc = mcuc_checksum16(buf, frame_len - 2);
    if (cs_calc != cs_recv) {
        ESP_LOGW(TAG, "Checksum mismatch: calc=0x%04X recv=0x%04X", cs_calc, cs_recv);
        ESP_LOG_BUFFER_HEX_LEVEL(TAG, buf, frame_len, ESP_LOG_WARN);
        return ESP_FAIL;
    }

    /* ---------- 打印原始 hex ---------- */
    ESP_LOG_BUFFER_HEX_LEVEL(TAG, buf + 4, remain - 2, ESP_LOG_DEBUG);

    /* ---------- 解析 ---------- */
    mcuc_parse_fields(buf + 4, out_data);

    ESP_LOGI(TAG, "PM_in: %u/%u/%u  PM_out: %u/%u/%u",
             out_data->pms_in_pm1_0, out_data->pms_in_pm2_5, out_data->pms_in_pm10,
             out_data->pms_out_pm1_0, out_data->pms_out_pm2_5, out_data->pms_out_pm10);
    ESP_LOGI(TAG, "Temp=%u  Hum=%u  TVOC=%u  AQ=%u  CO2=%u",
             out_data->temperature, out_data->humidity,
             out_data->tvoc_count, out_data->aq_state, out_data->co2_count);
    ESP_LOGI(TAG, "Audio=%u  PIR=%u  Light=%u  Pressure=%u",
             out_data->audio_state, out_data->pir_state,
             out_data->light_state, out_data->pressure_count);

    return ESP_OK;
}
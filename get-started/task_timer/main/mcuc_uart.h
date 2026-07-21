#pragma once

#include <stdint.h>
#include "esp_err.h"

/* ---------- 引脚 ---------- */
#define MCUC_UART_PORT          UART_NUM_2
#define MCUC_UART_TX            17
#define MCUC_UART_RX            16
#define MCUC_UART_BAUD          9600

/* ---------- 帧常量 ---------- */
#define MCUC_HEAD_LO            0x2D
#define MCUC_HEAD_HI            0x23
#define MCUC_HEADER_LEN         2
#define MCUC_LEN_LEN            2
#define MCUC_CS_LEN             2
#define MCUC_FIELD_COUNT        19
#define MCUC_MAX_FRAME          128

/* ---------- 数据字段（19 × uint16_t LE）---------- */
typedef struct {
    uint16_t pms_in_pm1_0;       /* PMS_in_data_Spm1_0       */
    uint16_t pms_in_pm2_5;       /* PMS_in_data_Spm2_5       */
    uint16_t pms_in_pm10;        /* PMS_in_data_Spm10        */
    uint16_t pms_out_pm1_0;      /* PMS_out_data_Spm1_0      */
    uint16_t pms_out_pm2_5;      /* PMS_out_data_Spm2_5      */
    uint16_t pms_out_pm10;       /* PMS_out_data_Spm10       */
    uint16_t temperature;        /* G_Temperature (SHT41)    */
    uint16_t humidity;           /* G_Humidity (SHT41)       */
    uint16_t tvoc_count;         /* TVOC_Count (SGP40 VOC)   */
    uint16_t aq_state;           /* G_AQ_State (0-4)         */
    uint16_t audio_state;        /* G_AUDIO_State            */
    uint16_t pir_state;          /* G_PIR_State              */
    uint16_t co2_count;          /* G_CO2_Count (ppm)        */
    uint16_t light_state;        /* G_Light_State            */
    uint16_t pressure_count;     /* G_Pressure_Count (hPa)   */
    uint16_t reserved1;          /* Query_CM1106_ABC_status  */
    uint16_t reserved2;          /* voc_index                */
    uint16_t reserved3;          /* CM1107_calibration_state */
    uint16_t ave_state_sum;      /* averaged_state_sum       */
} mcuc_data_t;

/* ---------- 函数 ---------- */
esp_err_t mcuc_uart_init(void);
esp_err_t mcuc_uart_read_frame(mcuc_data_t *out_data);
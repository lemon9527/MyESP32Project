#define ILI9341_DRIVER
#define TFT_MOSI 23
#define TFT_MISO 19
#define TFT_SCLK 18
#define TFT_CS   14
#define TFT_DC   26
#define TFT_RST  27
#define TFT_BL   25
#define TOUCH_CS 13
#define UART_TX  17
#define UART_RX  16

#define FW_VERSION_OFFSET  17
#define FW_VERSION_LEN     10
#define FRAME_OVERHEAD      6

#define AUDIO_OFFSET  5
#define PIR_OFFSET    9
#define LIGHT_OFFSET  13

#define R16BE(buf, off) (((uint16_t)(buf)[off] << 8) | (uint16_t)(buf)[(off)+1])

static const uint8_t cmd_co2_sw_write[] = {0x2E, 0x53, 0x31, 0x00, 0x01, 0x1E, 0xD1};
static const uint8_t cmd_co2_sw_read[]  = {0x2E, 0x53, 0x31, 0x01, 0x01, 0x0C, 0xC0};
static const uint8_t cmd_co2_ppm_write[] = {0x2E, 0x53, 0x31, 0x00, 0x01, 0x01, 0xB4};
static const uint8_t cmd_co2_ppm_read[]  = {0x2E, 0x53, 0x31, 0x01, 0x01, 0x03, 0xB7};

static const uint8_t cmd_gzp_write[]    = {0x2E, 0x53, 0x78, 0x00, 0x01, 0xAC, 0xA6};
static const uint8_t cmd_gzp_read[]     = {0x2E, 0x53, 0x78, 0x01, 0x01, 0x06, 0x01};

static const uint8_t cmd_sht41_write[]  = {0x2E, 0x53, 0x44, 0x00, 0x01, 0xFD, 0xC3};
static const uint8_t cmd_sht41_read[]   = {0x2E, 0x53, 0x44, 0x01, 0x01, 0x06, 0xCD};

static const uint8_t cmd_sgp40_write[]  = {0x2E, 0x53, 0x59, 0x00, 0x08, 0x26, 0x0F, 0x80, 0x00, 0xA2, 0x66, 0x66, 0x93, 0x98};
static const uint8_t cmd_sgp40_read[]   = {0x2E, 0x53, 0x59, 0x01, 0x01, 0x03, 0xDF};

static const uint8_t cmd_pm2009x_write[] = {0x2E, 0x53, 0x51, 0x00, 0x01, 0x01, 0xD4};
static const uint8_t cmd_pm2009x_read[]  = {0x2E, 0x53, 0x51, 0x01, 0x01, 0x09, 0xDD};

// GZP6816D 压力计算常数
#define PMIN    30000.0f
#define PMAX    110000.0f
#define DMIN    1677722.0f
#define DMAX    15099494.0f

#include "sensirion_gas_index_algorithm.h"

static uint8_t  bridge_state = 0;
static uint32_t bridge_timeout = 0;

static GasIndexAlgorithmParams gas_index_params;

static bool sensor_fw_saved = false;
static bool co2_fw_saved = false;
static char sensor_fw_buf[11];
static char co2_fw_buf[11];

#include <TFT_eSPI.h>
#include <lvgl.h>

TFT_eSPI tft;

// 屏幕分辨率
#define SCR_W 240
#define SCR_H 320

// 缓冲区：双缓冲 PARTIAL 模式，渲染和传输并行，无撕裂
#define DRAW_BUF_SIZE (240 * 320 / 10 * (LV_COLOR_DEPTH / 8))
static uint32_t draw_buf1[DRAW_BUF_SIZE / 4];
static uint32_t draw_buf2[DRAW_BUF_SIZE / 4];

// 帧解析状态机
static uint8_t  rx_buffer[64];
static uint8_t  rx_index = 0;
static bool     rx_synced = false;

static uint8_t  br_buf[32];
static uint8_t  br_index = 0;
static bool     br_collecting = false;

// 全局对象指针
static lv_obj_t *label;
static lv_obj_t *label_audio;
static lv_obj_t *label_pir;
static lv_obj_t *label_light;
static lv_obj_t *label_co2;
static lv_obj_t *label_co2_ppm;
static lv_obj_t *label_pressure;
static lv_obj_t *label_sht;
static lv_obj_t *label_sgp;
static lv_obj_t *label_pm;

// LVGL tick 回调，返回 millis()
static uint32_t my_tick(void) {
  return millis();
}

// 屏幕刷新回调
void
tft_flush_cb(lv_display_t *disp, const lv_area_t *area, uint8_t *px_map) {
  int x1 = area->x1, y1 = area->y1, x2 = area->x2, y2 = area->y2;
  tft.startWrite();
  tft.setAddrWindow(x1, y1, x2-x1+1, y2-y1+1);
  tft.pushColors((uint16_t*)px_map, (x2-x1+1)*(y2-y1+1), true);
  tft.endWrite();
  lv_display_flush_ready(disp);
}

// UI 创建函数
void ui_create(void) {
  lv_obj_set_style_bg_color(lv_scr_act(), lv_color_black(), LV_PART_MAIN);
  
  label = lv_label_create(lv_scr_act());
  lv_label_set_text(label, "waiting for SensorPCB test...");
  lv_obj_set_style_text_color(label, lv_color_white(), LV_PART_MAIN);
  lv_obj_set_style_text_font(label, &lv_font_montserrat_16, LV_PART_MAIN);
  lv_obj_center(label);

  label_audio = lv_label_create(lv_scr_act());
  lv_label_set_text(label_audio, "AUDIO: ---");
  lv_obj_set_style_text_color(label_audio, lv_color_white(), LV_PART_MAIN);
  lv_obj_set_style_text_font(label_audio, &lv_font_montserrat_14, LV_PART_MAIN);
  lv_obj_add_flag(label_audio, LV_OBJ_FLAG_HIDDEN);

  label_pir = lv_label_create(lv_scr_act());
  lv_label_set_text(label_pir, "PIR: ---");
  lv_obj_set_style_text_color(label_pir, lv_color_white(), LV_PART_MAIN);
  lv_obj_set_style_text_font(label_pir, &lv_font_montserrat_14, LV_PART_MAIN);
  lv_obj_add_flag(label_pir, LV_OBJ_FLAG_HIDDEN);

  label_light = lv_label_create(lv_scr_act());
  lv_label_set_text(label_light, "LIGHT: ---");
  lv_obj_set_style_text_color(label_light, lv_color_white(), LV_PART_MAIN);
  lv_obj_set_style_text_font(label_light, &lv_font_montserrat_14, LV_PART_MAIN);
  lv_obj_add_flag(label_light, LV_OBJ_FLAG_HIDDEN);

  label_co2 = lv_label_create(lv_scr_act());
  lv_label_set_text(label_co2, "CO2 Sensor FW: ---");
  lv_obj_set_style_text_color(label_co2, lv_color_white(), LV_PART_MAIN);
  lv_obj_set_style_text_font(label_co2, &lv_font_montserrat_14, LV_PART_MAIN);
  lv_obj_add_flag(label_co2, LV_OBJ_FLAG_HIDDEN);

  label_co2_ppm = lv_label_create(lv_scr_act());
  lv_label_set_text(label_co2_ppm, "CO2 ppm: ---");
  lv_obj_set_style_text_color(label_co2_ppm, lv_color_white(), LV_PART_MAIN);
  lv_obj_set_style_text_font(label_co2_ppm, &lv_font_montserrat_14, LV_PART_MAIN);
  lv_obj_add_flag(label_co2_ppm, LV_OBJ_FLAG_HIDDEN);

  label_pressure = lv_label_create(lv_scr_act());
  lv_label_set_text(label_pressure, "Pressure: ---");
  lv_obj_set_style_text_color(label_pressure, lv_color_white(), LV_PART_MAIN);
  lv_obj_set_style_text_font(label_pressure, &lv_font_montserrat_14, LV_PART_MAIN);
  lv_obj_add_flag(label_pressure, LV_OBJ_FLAG_HIDDEN);

  label_sht = lv_label_create(lv_scr_act());
  lv_label_set_text(label_sht, "T: ---  RH: ---");
  lv_obj_set_style_text_color(label_sht, lv_color_white(), LV_PART_MAIN);
  lv_obj_set_style_text_font(label_sht, &lv_font_montserrat_14, LV_PART_MAIN);
  lv_obj_add_flag(label_sht, LV_OBJ_FLAG_HIDDEN);

  label_sgp = lv_label_create(lv_scr_act());
  lv_label_set_text(label_sgp, "SGP40 VOC index: ---");
  lv_obj_set_style_text_color(label_sgp, lv_color_white(), LV_PART_MAIN);
  lv_obj_set_style_text_font(label_sgp, &lv_font_montserrat_14, LV_PART_MAIN);
  lv_obj_add_flag(label_sgp, LV_OBJ_FLAG_HIDDEN);

  label_pm = lv_label_create(lv_scr_act());
  lv_label_set_text(label_pm, "PM2.5: ---");
  lv_obj_set_style_text_color(label_pm, lv_color_white(), LV_PART_MAIN);
  lv_obj_set_style_text_font(label_pm, &lv_font_montserrat_14, LV_PART_MAIN);
  lv_obj_add_flag(label_pm, LV_OBJ_FLAG_HIDDEN);
}

// 帧解析：校验 header / length / checksum，提取 FW version 并输出
void process_frame() {
  uint16_t header = ((uint16_t)rx_buffer[0] << 8) | rx_buffer[1];
  if (header != 0x2E23)          return;

  uint8_t  payload_len = rx_buffer[4];
  uint16_t total_len   = payload_len + FRAME_OVERHEAD;

  uint8_t sum = 0;
  for (int i = 0; i < total_len - 1; i++) sum += rx_buffer[i];
  if (sum != rx_buffer[total_len - 1]) return;

  char fw[FW_VERSION_LEN + 1];
  memcpy(fw, &rx_buffer[FW_VERSION_OFFSET], FW_VERSION_LEN);
  fw[FW_VERSION_LEN] = '\0';
  Serial.print("FW version: ");
  Serial.println(fw);

  char display_text[32];

  if (!sensor_fw_saved) {
    sensor_fw_saved = true;
    snprintf(display_text, sizeof(display_text), "SensorPCB FW: %s", fw);
    lv_obj_set_style_text_font(label, &lv_font_montserrat_14, LV_PART_MAIN);
    lv_label_set_text(label, display_text);
    lv_obj_align(label, LV_ALIGN_TOP_LEFT, 5, 5);
  }

  uint16_t audio = R16BE(rx_buffer, AUDIO_OFFSET);
  uint16_t pir   = R16BE(rx_buffer, PIR_OFFSET);
  uint16_t light = R16BE(rx_buffer, LIGHT_OFFSET);

  Serial.print("AUDIO: "); Serial.println(audio);
  Serial.print("PIR:   "); Serial.println(pir);
  Serial.print("LIGHT: "); Serial.println(light);

  static uint16_t last_audio = 0xFFFF;
  static uint16_t last_pir   = 0xFFFF;
  static uint16_t last_light = 0xFFFF;

  snprintf(display_text, sizeof(display_text), "AUDIO: %u", audio);
  if (audio != last_audio) {
    lv_label_set_text(label_audio, display_text);
    last_audio = audio;
  }

  snprintf(display_text, sizeof(display_text), "PIR: %u", pir);
  if (pir != last_pir) {
    lv_label_set_text(label_pir, display_text);
    last_pir = pir;
  }

  snprintf(display_text, sizeof(display_text), "LIGHT: %u", light);
  if (light != last_light) {
    lv_label_set_text(label_light, display_text);
    last_light = light;
  }

  static bool sensor_ui_aligned = false;
  if (!sensor_ui_aligned) {
    sensor_ui_aligned = true;
    lv_obj_align_to(label_audio, label, LV_ALIGN_OUT_BOTTOM_LEFT, 0, 5);
    lv_obj_align_to(label_pir, label_audio, LV_ALIGN_OUT_BOTTOM_LEFT, 0, 5);
    lv_obj_align_to(label_light, label_pir, LV_ALIGN_OUT_BOTTOM_LEFT, 0, 5);
    lv_obj_align_to(label_co2, label_light, LV_ALIGN_OUT_BOTTOM_LEFT, 0, 5);
    lv_obj_align_to(label_co2_ppm, label_co2, LV_ALIGN_OUT_BOTTOM_LEFT, 0, 5);
    lv_obj_align_to(label_pressure, label_co2_ppm, LV_ALIGN_OUT_BOTTOM_LEFT, 0, 5);
    lv_obj_align_to(label_sht, label_pressure, LV_ALIGN_OUT_BOTTOM_LEFT, 0, 5);
    lv_obj_align_to(label_sgp, label_sht, LV_ALIGN_OUT_BOTTOM_LEFT, 0, 5);
    lv_obj_align_to(label_pm, label_sgp, LV_ALIGN_OUT_BOTTOM_LEFT, 0, 5);

    lv_obj_remove_flag(label_audio, LV_OBJ_FLAG_HIDDEN);
    lv_obj_remove_flag(label_pir, LV_OBJ_FLAG_HIDDEN);
    lv_obj_remove_flag(label_light, LV_OBJ_FLAG_HIDDEN);
    lv_obj_remove_flag(label_co2, LV_OBJ_FLAG_HIDDEN);
    lv_obj_remove_flag(label_co2_ppm, LV_OBJ_FLAG_HIDDEN);
    lv_obj_remove_flag(label_pressure, LV_OBJ_FLAG_HIDDEN);
    lv_obj_remove_flag(label_sht, LV_OBJ_FLAG_HIDDEN);
    lv_obj_remove_flag(label_sgp, LV_OBJ_FLAG_HIDDEN);
    lv_obj_remove_flag(label_pm, LV_OBJ_FLAG_HIDDEN);
  }
}

void process_bridge() {
  uint8_t len = 6 + br_buf[4];

  uint8_t sum = 0;
  for (int i = 0; i < len - 1; i++) sum += br_buf[i];
  if (sum != br_buf[len - 1]) return;

  uint8_t cmd = br_buf[3];

  if (cmd == 0x00) {
    if (br_buf[5] == 0x00) {
      Serial.println("ACKED");
      delay(20);
      if (bridge_state == 1) {
        Serial2.write(cmd_co2_sw_read, 7);
        bridge_state = 2;
      } else if (bridge_state == 3) {
        Serial2.write(cmd_co2_ppm_read, 7);
        bridge_state = 4;
      } else if (bridge_state == 5) {
        Serial2.write(cmd_gzp_read, 7);
        bridge_state = 6;
      } else if (bridge_state == 7) {
        Serial2.write(cmd_sht41_read, 7);
        bridge_state = 8;
      } else if (bridge_state == 9) {
        Serial2.write(cmd_sgp40_read, 7);
        bridge_state = 10;
      } else if (bridge_state == 11) {
        Serial2.write(cmd_pm2009x_read, 7);
        bridge_state = 12;
      }
      bridge_timeout = millis() + 200;
    } else {
      Serial.println("Bridge write ERROR");
      bridge_state = 0;
    }
  } else if (cmd == 0x01) {
    bridge_state = 0;

    Serial.print("HEX: ");
    for (int i = 0; i < len; i++) {
      if (br_buf[i] < 0x10) Serial.print('0');
      Serial.print(br_buf[i], HEX);
      Serial.print(' ');
    }
    Serial.println();

    if (br_buf[4] == 0x0C) {
      char sw_buf[11];
      int j = 0;
      for (int i = 6; i < len - 2; i++) sw_buf[j++] = (char)br_buf[i];
      sw_buf[j] = '\0';
      Serial.print("SW version: ");
      Serial.println(sw_buf);

      if (!co2_fw_saved) {
        co2_fw_saved = true;
        char lcd_text[32];
        snprintf(lcd_text, sizeof(lcd_text), "CO2 Sensor FW: %s", sw_buf);
        lv_label_set_text(label_co2, lcd_text);
        lv_obj_align_to(label_co2, label_light, LV_ALIGN_OUT_BOTTOM_LEFT, 0, 5);
        lv_obj_align_to(label_pressure, label_co2_ppm, LV_ALIGN_OUT_BOTTOM_LEFT, 0, 5);
      }
    } else if (br_buf[4] == 0x03 && br_buf[2] == 0x31) {
      uint16_t ppm = R16BE(br_buf, 6);
      Serial.print("CO2 ppm: ");
      Serial.println(ppm);

      char ppm_text[32];
      snprintf(ppm_text, sizeof(ppm_text), "CO2 ppm: %u", ppm);
      lv_label_set_text(label_co2_ppm, ppm_text);
    } else if (br_buf[4] == 0x06 && br_buf[2] == 0x78) {
      uint32_t Dtest = ((uint32_t)br_buf[6] << 16)
                     | ((uint16_t)br_buf[7] << 8)
                     |  br_buf[8];
      float pressure = ((PMAX - PMIN) / (DMAX - DMIN)
                     * ((float)Dtest - DMIN) + PMIN) / 100.0f;
      Serial.print("Pressure: ");
      Serial.print(pressure);
      Serial.println(" hPa");

      char pres_text[32];
      snprintf(pres_text, sizeof(pres_text), "Pressure: %.2f hPa", pressure);
      lv_label_set_text(label_pressure, pres_text);
    } else if (br_buf[4] == 0x06 && br_buf[2] == 0x44) {
      uint8_t t[2] = { br_buf[5], br_buf[6] };
      uint8_t crc_t = br_buf[7];
      uint8_t rh[2] = { br_buf[8], br_buf[9] };
      uint8_t crc_rh = br_buf[10];

      if (crc8(t, 2) != crc_t) {
        Serial.println("SHTC41: T CRC error");
        return;
      }
      if (crc8(rh, 2) != crc_rh) {
        Serial.println("SHTC41: RH CRC error");
        return;
      }

      uint16_t t_raw = (uint16_t)t[0] << 8 | t[1];
      uint16_t rh_raw = (uint16_t)rh[0] << 8 | rh[1];
      float temp_c = -45.0f + 175.0f * (float)t_raw / 65535.0f;
      float rh_pct = 0.0f + 125.0f * (float)rh_raw / 65535.0f;

      Serial.print("SHTC41: T=");
      Serial.print(temp_c, 2);
      Serial.print("°C  RH=");
      Serial.print(rh_pct, 2);
      Serial.println("%");

      char sht_text[32];
      snprintf(sht_text, sizeof(sht_text), "T: %.1f C  RH: %.0f %%", temp_c, rh_pct);
      lv_label_set_text(label_sht, sht_text);
    } else if (br_buf[4] == 0x03 && br_buf[2] == 0x59) {
      uint8_t raw[2] = { br_buf[5], br_buf[6] };
      uint8_t crc = br_buf[7];
      if (crc8(raw, 2) != crc) {
        Serial.println("SGP40: CRC error");
        return;
      }
      uint16_t sraw = ((uint16_t)raw[0] << 8) | raw[1];
      int32_t voc_index = 0;
      GasIndexAlgorithm_process(&gas_index_params, (int32_t)sraw, &voc_index);

      Serial.print("SGP40 VOC Index: ");
      Serial.println(voc_index);

      char sgp_text[32];
      snprintf(sgp_text, sizeof(sgp_text), "SGP40 VOC index: %ld", (long)voc_index);
      lv_label_set_text(label_sgp, sgp_text);
    } else if (br_buf[4] == 0x09 && br_buf[2] == 0x51) {
      uint16_t pm1_0  = R16BE(br_buf, 5);
      uint8_t  crc1    = br_buf[7];
      uint16_t pm2_5  = R16BE(br_buf, 8);
      uint8_t  crc2    = br_buf[10];
      uint16_t pm10_0 = R16BE(br_buf, 11);
      uint8_t  crc3    = br_buf[13];

      uint8_t data[2];
      data[0] = br_buf[5]; data[1] = br_buf[6];
      if (crc8(data, 2) != crc1) {
        Serial.println("PM2009X: PM1.0 CRC error");
        return;
      }
      data[0] = br_buf[8]; data[1] = br_buf[9];
      if (crc8(data, 2) != crc2) {
        Serial.println("PM2009X: PM2.5 CRC error");
        return;
      }
      data[0] = br_buf[11]; data[1] = br_buf[12];
      if (crc8(data, 2) != crc3) {
        Serial.println("PM2009X: PM10 CRC error");
        return;
      }

      Serial.printf("PM2009X: PM1.0=%u ug/m3  PM2.5=%u ug/m3  PM10=%u ug/m3\n", pm1_0, pm2_5, pm10_0);

      char pm_text[32];
      snprintf(pm_text, sizeof(pm_text), "PM2.5: %u ug/m3", pm2_5);
      lv_label_set_text(label_pm, pm_text);
    }
  }
}

void setup() {
  Serial.begin(115200);
  Serial.println("waiting for uart function test...");
  Serial2.begin(115200, SERIAL_8N1, UART_RX, UART_TX);

  GasIndexAlgorithm_init(&gas_index_params, 0);   // 0 = VOC type

  pinMode(TFT_BL, OUTPUT);
  digitalWrite(TFT_BL, HIGH);
  tft.begin();
  tft.setRotation(0);
  
  lv_init();
  /* Set millis() as tick source for LVGL v9 */
  lv_tick_set_cb(my_tick);
  
  lv_display_t *disp = lv_display_create(240, 320);
  lv_display_set_flush_cb(disp, tft_flush_cb);
  lv_display_set_buffers(disp, draw_buf1, draw_buf2, sizeof(draw_buf1), LV_DISPLAY_RENDER_MODE_PARTIAL);
  
  ui_create();
}

void loop() {
  lv_timer_handler();
  delay(5);

  static bool startup_done = false;
  if (!startup_done) {
    if (millis() < 3000) return;
    startup_done = true;
  }

  while (Serial2.available()) {
    uint8_t b = Serial2.read();

    if (!rx_synced) {
      if (b == 0x2E) { rx_buffer[0] = b; rx_synced = true; }
    } else if (rx_index == 0) {
      rx_buffer[1] = b;
      if (b == 0x23) {
        rx_index = 2;
      } else if (b == 0x41) {
        br_buf[0] = rx_buffer[0];
        br_buf[1] = rx_buffer[1];
        br_index = 2;
        br_collecting = true;
        rx_synced = false;
        continue;
      } else {
        rx_synced = false;
      }
    } else {
      rx_buffer[rx_index++] = b;
      uint16_t need = rx_buffer[4] + FRAME_OVERHEAD;
      if (rx_index >= need) {
        process_frame();
        rx_index = 0;
        rx_synced = false;
      }
    }

    if (br_collecting) {
      br_buf[br_index++] = b;
      uint8_t br_len = br_buf[4];
      if (br_index >= 6 + br_len) {
        process_bridge();
        br_collecting = false;
        br_index = 0;
      }
    }
  }

  static uint32_t bridge_next = 3000;
  static uint8_t  bridge_seq = 0;

  if (millis() - bridge_next >= 1000) {
    bridge_next = millis();
    bridge_seq = (bridge_seq + 1) % 6;

    if (bridge_seq == 0) {
      Serial2.write(cmd_co2_sw_write, 7);
      bridge_state = 1;
    } else if (bridge_seq == 1) {
      Serial2.write(cmd_co2_ppm_write, 7);
      bridge_state = 3;
    } else if (bridge_seq == 2) {
      Serial2.write(cmd_gzp_write, 7);
      bridge_state = 5;
    } else if (bridge_seq == 3) {
      Serial2.write(cmd_sht41_write, 7);
      bridge_state = 7;
    } else if (bridge_seq == 4) {
      Serial2.write(cmd_sgp40_write, sizeof(cmd_sgp40_write));
      bridge_state = 9;
    } else {
      Serial2.write(cmd_pm2009x_write, 7);
      bridge_state = 11;
    }
    bridge_timeout = millis() + 200;
  }

  if (bridge_state > 0 && millis() > bridge_timeout) {
    Serial.println("Bridge timeout");
    bridge_state = 0;
  }
}

uint8_t
crc8(const uint8_t *data, uint8_t len) {
  uint8_t crc = 0xFF;
  for (uint8_t i = 0; i < len; i++) {
    crc ^= data[i];
    for (uint8_t j = 0; j < 8; j++) {
      if (crc & 0x80) crc = (crc << 1) ^ 0x31;
      else           crc <<= 1;
    }
  }
  return crc;
}
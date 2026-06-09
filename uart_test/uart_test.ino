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

#include <TFT_eSPI.h>
#include <lvgl.h>

TFT_eSPI tft;

// 屏幕分辨率
#define SCR_W 240
#define SCR_H 320

// 缓冲区：屏幕面积的 1/10，单位字节
#define DRAW_BUF_SIZE (240 * 320 / 10 * (LV_COLOR_DEPTH / 8))
static uint32_t draw_buf[DRAW_BUF_SIZE / 4];

// 帧解析状态机
static uint8_t  rx_buffer[64];
static uint8_t  rx_index = 0;
static bool     rx_synced = false;

// 全局对象指针
static lv_obj_t *label;
static lv_obj_t *label_audio;
static lv_obj_t *label_pir;
static lv_obj_t *label_light;

// LVGL tick 回调，返回 millis()
static uint32_t my_tick(void) {
  return millis();
}

// 屏幕刷新回调
void tft_flush_cb(lv_display_t *disp, const lv_area_t *area, uint8_t *px_map) {
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

  label_pir = lv_label_create(lv_scr_act());
  lv_label_set_text(label_pir, "PIR: ---");
  lv_obj_set_style_text_color(label_pir, lv_color_white(), LV_PART_MAIN);
  lv_obj_set_style_text_font(label_pir, &lv_font_montserrat_14, LV_PART_MAIN);

  label_light = lv_label_create(lv_scr_act());
  lv_label_set_text(label_light, "LIGHT: ---");
  lv_obj_set_style_text_color(label_light, lv_color_white(), LV_PART_MAIN);
  lv_obj_set_style_text_font(label_light, &lv_font_montserrat_14, LV_PART_MAIN);
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
  snprintf(display_text, sizeof(display_text), "SensorPCB FW: %s", fw);
  lv_obj_set_style_text_font(label, &lv_font_montserrat_14, LV_PART_MAIN);
  lv_label_set_text(label, display_text);
  lv_obj_align(label, LV_ALIGN_TOP_LEFT, 5, 5);

  uint16_t audio = R16BE(rx_buffer, AUDIO_OFFSET);
  uint16_t pir   = R16BE(rx_buffer, PIR_OFFSET);
  uint16_t light = R16BE(rx_buffer, LIGHT_OFFSET);

  Serial.print("AUDIO: "); Serial.println(audio);
  Serial.print("PIR:   "); Serial.println(pir);
  Serial.print("LIGHT: "); Serial.println(light);

  snprintf(display_text, sizeof(display_text), "AUDIO: %u", audio);
  lv_label_set_text(label_audio, display_text);
  lv_obj_align_to(label_audio, label, LV_ALIGN_OUT_BOTTOM_LEFT, 0, 5);

  snprintf(display_text, sizeof(display_text), "PIR: %u", pir);
  lv_label_set_text(label_pir, display_text);
  lv_obj_align_to(label_pir, label_audio, LV_ALIGN_OUT_BOTTOM_LEFT, 0, 5);

  snprintf(display_text, sizeof(display_text), "LIGHT: %u", light);
  lv_label_set_text(label_light, display_text);
  lv_obj_align_to(label_light, label_pir, LV_ALIGN_OUT_BOTTOM_LEFT, 0, 5);
}

void setup() {
  Serial.begin(115200);
  Serial.println("waiting for uart function test...");
  Serial2.begin(115200, SERIAL_8N1, UART_RX, UART_TX);
  
  pinMode(TFT_BL, OUTPUT);
  digitalWrite(TFT_BL, HIGH);
  tft.begin();
  tft.setRotation(0);
  
  lv_init();
  /* Set millis() as tick source for LVGL v9 */
  lv_tick_set_cb(my_tick);
  
  lv_display_t *disp = lv_display_create(240, 320);
  lv_display_set_flush_cb(disp, tft_flush_cb);
  lv_display_set_buffers(disp, draw_buf, NULL, sizeof(draw_buf), LV_DISPLAY_RENDER_MODE_PARTIAL);
  
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
      if (b == 0x23) { rx_buffer[1] = b; rx_index = 2; }
      else if (b != 0x2E) { rx_synced = false; }
      else { rx_buffer[0] = b; }
    } else {
      rx_buffer[rx_index++] = b;
      uint16_t need = rx_buffer[4] + FRAME_OVERHEAD;
      if (rx_index >= need) {
        process_frame();
        rx_index = 0;
        rx_synced = false;
      }
    }
  }
}
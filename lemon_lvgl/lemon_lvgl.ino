#define ILI9341_DRIVER
#define TFT_MOSI 23
#define TFT_MISO 19
#define TFT_SCLK 18
#define TFT_CS   14
#define TFT_DC   26
#define TFT_RST  27
#define TFT_BL   25
#define TOUCH_CS 13

#include <TFT_eSPI.h>
#include <lvgl.h>

TFT_eSPI tft;

static lv_indev_t *indev;
static lv_obj_t *btn;
static bool btn_state = false;

#define BUF_W 240
#define BUF_H 80
static lv_color_t disp_buf1[BUF_W * BUF_H];
static lv_draw_buf_t draw_buf;

void tft_flush_cb(lv_display_t *disp, const lv_area_t *area, uint8_t *px_map) {
  int x1 = area->x1, y1 = area->y1, x2 = area->x2, y2 = area->y2;
  tft.startWrite();
  tft.setAddrWindow(x1, y1, x2-x1+1, y2-y1+1);
  tft.pushColors((uint16_t*)px_map, (x2-x1+1)*(y2-y1+1), true);
  tft.endWrite();
  lv_display_flush_ready(disp);
}

void touchpad_read(lv_indev_t *indev, lv_indev_data_t *data) {
  uint16_t touchX, touchY;
  bool touched = tft.getTouch(&touchX, &touchY);

  if (touched) {
    data->state = LV_INDEV_STATE_PRESSED;
    uint16_t screenX = map(touchY, 20, 310, 0, 240);
    uint16_t screenY = 320 - map(touchX, 20, 240, 0, 320);
    screenX = constrain(screenX, 0, 240);
    screenY = constrain(screenY, 0, 320);
    data->point.x = screenX;
    data->point.y = screenY;

    // 手动判断按钮区域
    if(btn) {
      lv_area_t btn_area;
      lv_obj_get_coords(btn, &btn_area);
      Serial.printf("BTN: x1=%d y1=%d x2=%d y2=%d, touch: %d,%d\n", btn_area.x1, btn_area.y1, btn_area.x2, btn_area.y2, screenX, screenY);
      if(screenX >= btn_area.x1 && screenX <= btn_area.x2 &&
         screenY >= btn_area.y1 && screenY <= btn_area.y2) {
            Serial.println("Pressed");
            lv_obj_set_style_bg_color(btn, lv_color_hex(0xFF6B6B), LV_PART_MAIN);
            lv_display_refr_now(lv_display_get_default());  // LVGL 9.x 强制刷新
      } else {
            Serial.println("Released");
            lv_obj_set_style_bg_color(btn, lv_color_hex(0x4ECDC4), LV_PART_MAIN);
            lv_display_refr_now(lv_display_get_default());  // LVGL 9.x 强制刷新
      }
    }
  } else {
    data->state = LV_INDEV_STATE_RELEASED;
    // 松开时始终恢复绿色
    if(btn) {
      lv_obj_set_style_bg_color(btn, lv_color_hex(0x4ECDC4), LV_PART_MAIN);
      lv_display_refr_now(lv_display_get_default());  // LVGL 9.x 强制刷新
    }
  }
}

void setup() {
  Serial.begin(115200);
  
  pinMode(TFT_BL, OUTPUT);
  digitalWrite(TFT_BL, HIGH);
  tft.begin();
  tft.setRotation(0);
  
  lv_init();
  lv_draw_buf_init(&draw_buf, BUF_W, BUF_H, LV_COLOR_FORMAT_RGB565, BUF_W, disp_buf1, BUF_W * BUF_H * sizeof(lv_color_t));
  
  lv_display_t *disp = lv_display_create(240, 320);
  lv_display_set_flush_cb(disp, tft_flush_cb);
  lv_display_set_draw_buffers(disp, &draw_buf, nullptr);
  
  btn = lv_btn_create(lv_scr_act());
  lv_obj_set_size(btn, 120, 50);
  lv_obj_center(btn);

  lv_obj_set_style_bg_color(btn, lv_color_hex(0x4ECDC4), LV_PART_MAIN);
  
  lv_obj_t *btn_label = lv_label_create(btn);
  lv_label_set_text(btn_label, "Press me");
  lv_obj_center(btn_label);
  
  indev = lv_indev_create();
  lv_indev_set_type(indev, LV_INDEV_TYPE_POINTER);
  lv_indev_set_read_cb(indev, touchpad_read);
  lv_indev_set_display(indev, disp);
}

void loop() {
  lv_timer_handler();  // LVGL 9.x 自动处理输入和事件
  delay(5);
}
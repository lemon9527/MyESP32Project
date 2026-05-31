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

// 屏幕分辨率
#define SCR_W 240
#define SCR_H 320

// 缓冲区 小屏够用
#define BUF_W 240
#define BUF_H 80
static lv_color_t disp_buf1[BUF_W * BUF_H];
static lv_draw_buf_t draw_buf;

// 全局输入设备和按钮指针
static lv_indev_t *indev;
static lv_obj_t *btn;

// 屏幕刷新回调
void tft_flush_cb(lv_display_t *disp, const lv_area_t *area, uint8_t *px_map) {
  int x1 = area->x1, y1 = area->y1, x2 = area->x2, y2 = area->y2;
  tft.startWrite();
  tft.setAddrWindow(x1, y1, x2-x1+1, y2-y1+1);
  tft.pushColors((uint16_t*)px_map, (x2-x1+1)*(y2-y1+1), true);
  tft.endWrite();
  lv_display_flush_ready(disp);
}

// 触摸回调
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
  } else {
    data->state = LV_INDEV_STATE_RELEASED;
  }
}

// UI 创建函数
void ui_create(void) {
  btn = lv_btn_create(lv_scr_act());
  lv_obj_set_size(btn, 120, 50);
  lv_obj_center(btn);

  lv_obj_set_style_bg_color(btn, lv_color_hex(0x4ECDC4), LV_PART_MAIN);
  
  lv_obj_t *btn_label = lv_label_create(btn);
  lv_label_set_text(btn_label, "Press me");
  lv_obj_center(btn_label);
  
  // 添加按钮事件回调 - 监听按下、松开和点击事件
  lv_obj_add_event_cb(btn, btn_event_cb, LV_EVENT_PRESSED, nullptr);
  lv_obj_add_event_cb(btn, btn_event_cb, LV_EVENT_RELEASED, nullptr);
  lv_obj_add_event_cb(btn, btn_event_cb, LV_EVENT_CLICKED, nullptr);
}

// 按钮事件回调函数
void btn_event_cb(lv_event_t *e)
{
  lv_event_code_t code = lv_event_get_code(e);
  lv_obj_t *target = (lv_obj_t *)lv_event_get_target(e);
  
  if(code == LV_EVENT_PRESSED)
  {
    // 按下时变色为红色
    lv_obj_set_style_bg_color(target, lv_color_hex(0xFF6B6B), LV_PART_MAIN);
    lv_refr_now(nullptr);  // 强制刷新以获得更快的响应
    Serial.println("Button PRESSED!");
  }
  else if(code == LV_EVENT_RELEASED)
  {
    // 松开时恢复绿色
    lv_obj_set_style_bg_color(target, lv_color_hex(0x4ECDC4), LV_PART_MAIN);
    lv_refr_now(nullptr);  // 强制刷新
    Serial.println("Button RELEASED!");
  }
  else if(code == LV_EVENT_CLICKED)
  {
    // 点击完成
    Serial.println("Button CLICKED!");
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
  
  ui_create();
  
  indev = lv_indev_create();
  lv_indev_set_type(indev, LV_INDEV_TYPE_POINTER);
  lv_indev_set_read_cb(indev, touchpad_read);
  lv_indev_set_display(indev, disp);
}

void loop() {
  lv_timer_handler();
  lv_indev_read(indev);
  delay(5);
}
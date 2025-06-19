#include <Arduino.h>
#include <BluetoothSerial.h>
#include <TFT_eSPI.h>
#include <TaskManagerIO.h>
#include <lvgl.h>

#include <stdexcept>
#include <string>
using namespace std;

#include "emu_mac_address.h"
#include "ledControl.h"

// #define USE_NAME
const char *pin = "1234";
String myBtName = "ESP32-BT-Master";

#if !defined(CONFIG_BT_SPP_ENABLED)
#error Serial Bluetooth not available or not enabled. It is only available for the ESP32 chip.
#endif

BluetoothSerial SerialBT;

#ifdef USE_NAME
// String slaveName = "EMUCANBT_SPP";
String slaveName = "EMULOGGER";
#else
uint8_t address[6] = {MAC0, MAC1, MAC2, MAC3, MAC4, MAC5};  // setup the value in lib/emu_mac_address.h
#endif

const int backLightPin = 27;
const int buzzerPin = 22;
bool buzzerOn = false;

int rpm;
int spd;
float afr;
float mapR;
float boost;
int tps;
int clt;
int ign;
int inj;
float bat;
int cel;

unsigned long previousMillis = 0;
const unsigned long reconnectInterval = 5000;

LV_FONT_DECLARE(lv_font_montserrat_14);
LV_FONT_DECLARE(lv_font_montserrat_18);
LV_FONT_DECLARE(lv_font_montserrat_20);
LV_FONT_DECLARE(lv_font_montserrat_28);

// Display & LVGL setup
TFT_eSPI tft = TFT_eSPI();
static lv_disp_draw_buf_t draw_buf;
static lv_color_t buf[LV_HOR_RES_MAX * 20];
lv_obj_t *table;

void connectToBt() {
  bool connected;
  if (!SerialBT.hasClient()) {
    ledWhite();
#ifdef USE_NAME
    connected = SerialBT.connect(slaveName);
#else
    connected = SerialBT.connect(address);
#endif
    if (connected) {
      Serial.println("Connected Successfully!");
      ledBlue();
    } else {
      Serial.println("Initial connect failed. Will retry in loop...");
      ledRed();
      delay(1000);
    }
  }
}

// LVGL Display Flush Callback
void my_disp_flush(lv_disp_drv_t *disp, const lv_area_t *area, lv_color_t *color_p) {
  uint16_t w = area->x2 - area->x1 + 1;
  uint16_t h = area->y2 - area->y1 + 1;
  tft.startWrite();
  tft.setAddrWindow(area->x1, area->y1, w, h);
  tft.pushColors((uint16_t *)&color_p->full, w * h, true);
  tft.endWrite();
  lv_disp_flush_ready(disp);
}

// Cell alignment fix
void my_table_event_cb(lv_event_t *e) {
  lv_obj_t *table = lv_event_get_target(e);
  lv_obj_draw_part_dsc_t *dsc = (lv_obj_draw_part_dsc_t *)lv_event_get_param(e);

  if (dsc->part == LV_PART_ITEMS) {
    uint16_t row = dsc->id / lv_table_get_col_cnt(table);
    uint16_t col = dsc->id % lv_table_get_col_cnt(table);

    if (col == 0 || col == 2) {
      dsc->label_dsc->font = &lv_font_montserrat_18;
    } else if (col == 1 || col == 3) {
      dsc->label_dsc->font = &lv_font_montserrat_20;
    }
    dsc->label_dsc->align = LV_TEXT_ALIGN_CENTER;
    if ((row == 0 && col == 1) || (row == 0 && col == 3) || (row == 1 && col == 1) || (row == 1 && col == 3) || (row == 2 && col == 1) || (row == 2 && col == 3) || (row == 3 && col == 1) || (row == 3 && col == 3) ||
        (row == 4 && col == 1) || (row == 4 && col == 3)) {
      dsc->label_dsc->align = LV_TEXT_ALIGN_RIGHT;
    }
    if (row == 5 && col == 1) {
      dsc->label_dsc->align = LV_TEXT_ALIGN_CENTER;
    }
  }
}

static void table_event_cb_bg(lv_event_t *e) {
  lv_obj_t *table = lv_event_get_target(e);
  lv_obj_draw_part_dsc_t *dsc = (lv_obj_draw_part_dsc_t *)lv_event_get_param(e);

  // Ensure dsc and rect_dsc are valid
  if (!dsc || !dsc->rect_dsc) return;

  // Only modify table cell backgrounds
  if (dsc->part == LV_PART_ITEMS) {
    uint16_t row = dsc->id / lv_table_get_col_cnt(table);
    uint16_t col = dsc->id % lv_table_get_col_cnt(table);

    const char *value_str = lv_table_get_cell_value(table, row, col);

    // Check if value_str is null or empty before conversion
    float value = 0.0f;  // Default value
    if (value_str != nullptr && value_str[0] != '\0') {
      try {
        value = std::stof(value_str);  // Convert string to float safely
      } catch (...) {
        value = 0.0f;  // Handle invalid conversions
      }
    }

    // Default cell color
    lv_color_t bg_color = lv_color_make(30, 30, 30);
    lv_color_t text_color = lv_color_white();

    if (row == 0 && col == 1 && value > 7000.00) {
      bg_color = lv_color_make(0, 0, 255);
      text_color = lv_color_white();
    }
    if (row == 1 && col == 3 && value > 100.00) {
      bg_color = lv_color_make(0, 0, 255);
      text_color = lv_color_white();
    }
    if (row == 1 && col == 3 && value < 55.00 && value > 01.00) {
      bg_color = lv_color_make(0, 255, 255);
      text_color = lv_color_black();
    }
    if (row == 2 && col == 3 && value < 12.00 && value > 01.00) {
      bg_color = lv_color_make(0, 0, 255);
      text_color = lv_color_white();
    }
    if (row == 3 && col == 3 && value > 1.10) {
      bg_color = lv_color_make(0, 0, 255);
      text_color = lv_color_white();
    }
    if (row == 5 && col == 1 && value_str != nullptr && value_str[0] != '\0') {
      bg_color = lv_color_make(0, 0, 255);
      text_color = lv_color_white();
    }

    // Apply background color to the cell
    dsc->rect_dsc->bg_color = bg_color;
    dsc->rect_dsc->bg_opa = LV_OPA_COVER;
    dsc->label_dsc->color = text_color;
  }
}

// Initialize LVGL Table
void create_table() {
  lv_obj_set_style_bg_color(lv_scr_act(), lv_color_make(30, 30, 30), LV_PART_MAIN);

  table = lv_table_create(lv_scr_act());
  lv_obj_align(table, LV_ALIGN_CENTER, -1, -1);
  lv_obj_set_style_text_opa(table, LV_OPA_COVER, 0);
  lv_obj_clear_flag(lv_scr_act(), LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_set_scrollbar_mode(table, LV_SCROLLBAR_MODE_OFF);
  lv_obj_set_style_text_color(table, lv_color_white(), LV_PART_ITEMS);
  lv_obj_set_style_bg_color(table, lv_color_make(30, 30, 30), LV_PART_MAIN);
  // lv_obj_set_style_text_font(table, &lv_font_montserrat_20, LV_PART_ITEMS);

  static lv_style_t style_cell0;
  lv_style_init(&style_cell0);
  lv_style_set_pad_top(&style_cell0, 12.8);
  lv_style_set_pad_bottom(&style_cell0, 12.8);
  lv_style_set_pad_left(&style_cell0, 4);
  lv_style_set_pad_right(&style_cell0, 4);
  lv_obj_add_style(table, &style_cell0, LV_PART_ITEMS);

  lv_table_set_col_cnt(table, 4);
  lv_table_set_row_cnt(table, 6);

  lv_obj_set_style_border_width(table, 1, LV_PART_ITEMS);
  lv_obj_set_style_border_color(table, lv_color_white(), LV_PART_ITEMS);
  lv_obj_set_style_border_side(table, LV_BORDER_SIDE_FULL, LV_PART_ITEMS);

  lv_table_set_col_width(table, 0, 53);
  lv_table_set_col_width(table, 1, 105);
  lv_table_set_col_width(table, 2, 47);
  lv_table_set_col_width(table, 3, 115);

  lv_table_add_cell_ctrl(table, 5, 1, LV_TABLE_CELL_CTRL_MERGE_RIGHT);
  lv_table_add_cell_ctrl(table, 5, 2, LV_TABLE_CELL_CTRL_MERGE_RIGHT);
  lv_table_add_cell_ctrl(table, 5, 3, LV_TABLE_CELL_CTRL_MERGE_RIGHT);

  lv_table_set_cell_value(table, 0, 0, "RPM");
  lv_table_set_cell_value(table, 0, 2, "SPD");
  lv_table_set_cell_value(table, 1, 0, "AFR");
  lv_table_set_cell_value(table, 1, 2, "CLT");
  lv_table_set_cell_value(table, 2, 0, "TPS");
  lv_table_set_cell_value(table, 2, 2, "BAT");
  lv_table_set_cell_value(table, 3, 0, "MAP");
  lv_table_set_cell_value(table, 3, 2, "BST");
  lv_table_set_cell_value(table, 4, 0, "INJ");
  lv_table_set_cell_value(table, 4, 2, "IGN");
  lv_table_set_cell_value(table, 5, 0, "CEL");

  lv_obj_add_event_cb(table, my_table_event_cb, LV_EVENT_DRAW_PART_BEGIN, NULL);
  lv_obj_add_event_cb(table, table_event_cb_bg, LV_EVENT_DRAW_PART_BEGIN, NULL);

  lv_timer_handler();
}

bool isBlueOn = false;
void ledStatus() {
  if (!SerialBT.hasClient()) {
    // we should reinit all values
    cel = 0;
    if (!isBlueOn) {
      ledBlue();
    } else {
      ledOff();
    }
    isBlueOn = !isBlueOn;
  } else {
    if (cel > 0) {
      isBlueOn = false;
      ledRed();
    } else {
      if (!isBlueOn) {
        ledBlue();
      }
      isBlueOn = true;
    }
  }
}

int decodeCheckEngine(uint16_t value) {
  // TODO: move it in a dedicated file like emuBluetooth.cpp
  int cel_codes = 0;
  string cel_names = "";
  if (value == 0) {
    return 0;
  } else {
    if (value & (1 << 0)) {
      cel_codes++;  // Bit 0
      cel_names = "CLT ";
    }
    if (value & (1 << 1)) {
      cel_codes++;  // Bit 1
      cel_names += "IAT ";
    }
    if (value & (1 << 2)) {
      cel_codes++;  // Bit 2
      cel_names += "MAP ";
    }
    if (value & (1 << 3)) {
      cel_codes++;  // Bit 3
      cel_names += "WBO ";
    }
    if (value & (1 << 8)) {
      cel_codes++;  // Bit 8
      cel_names += "FF SENSOR ";
    }
    if (value & (1 << 9)) {
      cel_codes++;  // Bit 9
      cel_names += "DBW ";
    }
    if (value & (1 << 10)) {
      cel_codes++;  // Bit 10
      cel_names += "FPR ";
    }

    lv_table_set_cell_value(table, 5, 1, cel_names.c_str());
    return cel_codes;
  }
}

void callbackReadBTData(const uint8_t *buffer, size_t size) {
  uint8_t channel;
  uint8_t idChar;
  uint8_t valueH;
  uint8_t valueL;
  uint8_t checksum;
  uint16_t value;
  int chData;
  for (size_t i = 0; i < size;) {
    channel = buffer[i];
    valueH = buffer[i + 2];
    valueL = buffer[i + 3];
    value = (valueH << 8) | valueL;
    checksum = buffer[i + 4];  // should be checked
    chData = static_cast<int>(channel);
    // Serial.printf("channel %d - value %d - checksum %d\n", channel, value, checksum);
    // Serial.printf("msg: %x %x %x %x %x\n", channel, idChar, valueH, valueL, checksum);
    if (chData == 1) {
      rpm = static_cast<int>(value);
      lv_table_set_cell_value(table, 0, 1, String(rpm).c_str());
    } else if (chData == 28) {
      spd = (static_cast<int>(value));
      lv_table_set_cell_value(table, 0, 3, (String(spd) + " KM/H").c_str());
    } else if (chData == 12) {
      afr = (static_cast<float>(value) / 10);
      lv_table_set_cell_value(table, 1, 1, String(afr).c_str());
    } else if (chData == 2) {
      mapR = (static_cast<float>(value) / 100);
      boost = (mapR - 1.0132f);
      lv_table_set_cell_value(table, 3, 1, (String(mapR) + " BAR").c_str());
      lv_table_set_cell_value(table, 3, 3, (String(boost) + " BAR").c_str());
    } else if (chData == 3) {
      tps = static_cast<int>(value);
      lv_table_set_cell_value(table, 2, 1, (String(tps) + " %").c_str());
    } else if (chData == 24) {
      clt = static_cast<int>(value);
      lv_table_set_cell_value(table, 1, 3, (String(clt) + " °C").c_str());
    } else if (chData == 6) {
      ign = (static_cast<int>(value) / 2);
      lv_table_set_cell_value(table, 4, 3, (String(ign) + " °").c_str());
    } else if (chData == 19) {
      inj = (static_cast<int>(value) / 2);
      lv_table_set_cell_value(table, 4, 1, (String(inj) + " %").c_str());
    } else if (chData == 5) {
      bat = (static_cast<float>(value) / 37);
      lv_table_set_cell_value(table, 2, 3, (String(bat) + " V").c_str());
    } else if (chData == 255) {
      cel = decodeCheckEngine(value);
    }
    i += 5;
  }
}

void refreshDisplay() {
  buzzerOn = (cel > 0 || clt > 105 || rpm > 7000 || boost > 1.10 || (bat < 12.00 && bat > 1.00));
  digitalWrite(buzzerPin, (millis() % 600 < 300) && buzzerOn);

  lv_obj_invalidate(table);
  lv_timer_handler();
}

void loop() {
  taskManager.runLoop();
}

void setup() {
  // Init led
  pinMode(4, OUTPUT);
  pinMode(17, OUTPUT);
  pinMode(16, OUTPUT);
  ledOff();

  // init TFT
  tft.init();
  pinMode(backLightPin, OUTPUT);
  digitalWrite(backLightPin, LOW);
  tft.fillScreen(TFT_DARKGREY);
  tft.setRotation(1);

  // init Serial
  Serial.begin(115200);
  Serial.println("setup!");

  pinMode(buzzerPin, OUTPUT);

  // Initialize LVGL
  lv_init();
  lv_refr_now(NULL);
  lv_disp_draw_buf_init(&draw_buf, buf, NULL, LV_HOR_RES_MAX * 10);

  // Setup LVGL Display Driver
  static lv_disp_drv_t disp_drv;
  lv_disp_drv_init(&disp_drv);
  disp_drv.hor_res = 320;
  disp_drv.ver_res = 240;
  disp_drv.flush_cb = my_disp_flush;
  disp_drv.draw_buf = &draw_buf;
  lv_disp_drv_register(&disp_drv);

  // init bluetooth
  SerialBT.begin(myBtName, true);
#ifndef USE_NAME
  SerialBT.setPin(pin);
#endif
  SerialBT.onData(callbackReadBTData);

  // set grid
  create_table();
  digitalWrite(backLightPin, HIGH);

  // init tasks
  taskManager.yieldForMicros(250);
  taskManager.scheduleFixedRate(250, ledStatus);
  taskManager.scheduleFixedRate(300, refreshDisplay);
  taskManager.scheduleFixedRate(reconnectInterval, connectToBt);
}

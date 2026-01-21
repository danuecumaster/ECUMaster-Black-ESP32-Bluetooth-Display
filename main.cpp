#include <Arduino.h>
#define LV_COLOR_16_SWAP 0
#include <lvgl.h>
#include <TFT_eSPI.h>
#include "BluetoothSerial.h"
#include <string>
#include <stdexcept>
#include <SPI.h>
#include <SD.h>
extern const lv_font_t ui_font_JBM_18;
extern const lv_font_t ui_font_JBM_15;
extern const lv_font_t ui_font_JBM_10;
using namespace std;

//#define USE_NAME
const char *pin = "1234";										/* BT Pin */
String myBtName = "ESP32-BT-Master";							/* ESP's BT Name */

#if !defined(CONFIG_BT_SPP_ENABLED)
	#error Serial Bluetooth not available or not enabled. It is only available for the ESP32 chip.
#endif

BluetoothSerial SerialBT;

#ifdef USE_NAME
	String slaveName = "EMUCANBT_SPP";
#else
	uint8_t address[6] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }; /* Update the MAC Address */
#endif

#define SCK  18
#define MISO 19
#define MOSI 23
#define CS   5

File logFile;
unsigned long lastLogMillis 	= 0;
uint32_t logCounter 			= 0;
const unsigned long logInterval = 200;
const uint64_t MIN_FREE_BYTES 	= 100ULL * 1024ULL * 1024ULL; 	/* Min empty storage: 100 MB */
static unsigned long lastFlush 	= 0;

const int backLightPin 	= 27;
const int buzzerPin 	= 22;
bool buzzerOn 			= false;
bool btIconSts 			= false;
static lv_style_t style_bt;
static lv_style_t style_max0;
static lv_style_t style_max1;
static lv_style_t style_max2;
static lv_style_t style_max3;
static bool style_initialized = false;

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
float maxboost 	= -100.0f;
int maxclt 		= -40;

unsigned long previousMillis 			= 0;
const unsigned long reconnectInterval 	= 5000;

LV_FONT_DECLARE(lv_font_montserrat_14);
LV_FONT_DECLARE(lv_font_montserrat_28);
LV_FONT_DECLARE(ui_font_JBM_18);                
LV_FONT_DECLARE(ui_font_JBM_15);                
LV_FONT_DECLARE(ui_font_JBM_10);                      

lv_obj_t *bt_icon_label;
lv_obj_t *max_icon_label;  
lv_obj_t *max_icon_label1;  
lv_obj_t *max_val_label_clt;
lv_obj_t *max_val_label_bst;             

// Display & LVGL setup
TFT_eSPI tft = TFT_eSPI();
static lv_disp_draw_buf_t draw_buf;
static lv_color_t buf[LV_HOR_RES_MAX * 20];
lv_obj_t *table;

uint64_t getFreeBytes() {
	uint64_t total = SD.totalBytes();
	uint64_t used  = SD.usedBytes();
	if (total <= used) return 0;
	return total - used;
}

void ensureFreeSpace() {
	uint64_t freeBytes = getFreeBytes();
	if (freeBytes >= MIN_FREE_BYTES) {
		Serial.println("SD space OK");
		return;
	}

	Serial.println("Low SD space, deleting old logs...");

	// Delete from the oldest possible index
	for (int index = 1; index < 10000 && freeBytes < MIN_FREE_BYTES; index++) {
		char name[20];
		sprintf(name, "/log_%04d.csv", index);

		if (SD.exists(name)) {
			File f = SD.open(name);
			uint32_t size = f ? f.size() : 0;
			f.close();
			if (SD.remove(name)) {
				freeBytes += size;
				Serial.print("Deleted ");
				Serial.print(name);
				Serial.print(" | freed ");
				Serial.print(size);
				Serial.println(" bytes");
			}
		}
	}
	Serial.print("Free space after cleanup: ");
	Serial.println(getFreeBytes());
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
	//lv_obj_set_style_text_font(table, &ui_font_JBM_20, LV_PART_ITEMS);

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

	lv_table_set_col_width(table, 0, 47);
	lv_table_set_col_width(table, 1, 119);
	lv_table_set_col_width(table, 2, 47);
	lv_table_set_col_width(table, 3, 107);

	lv_table_add_cell_ctrl(table, 5, 1, LV_TABLE_CELL_CTRL_MERGE_RIGHT);
	lv_table_add_cell_ctrl(table, 5, 2, LV_TABLE_CELL_CTRL_MERGE_RIGHT);
	lv_table_add_cell_ctrl(table, 5, 3, LV_TABLE_CELL_CTRL_MERGE_RIGHT);

	lv_table_add_cell_ctrl(table, 4, 2, LV_TABLE_CELL_CTRL_MERGE_RIGHT);
	lv_table_add_cell_ctrl(table, 4, 3, LV_TABLE_CELL_CTRL_MERGE_RIGHT);

	lv_table_set_cell_value(table, 0, 0, "RPM");
	lv_table_set_cell_value(table, 0, 2, "SPD");
	lv_table_set_cell_value(table, 1, 0, "AFR");
	lv_table_set_cell_value(table, 1, 2, "CLT");
	lv_table_set_cell_value(table, 2, 0, "TPS");
	lv_table_set_cell_value(table, 2, 2, "BAT");
	lv_table_set_cell_value(table, 3, 0, "INJ");
	lv_table_set_cell_value(table, 3, 2, "IGN");
	lv_table_set_cell_value(table, 4, 0, "BST");
	lv_table_set_cell_value(table, 5, 0, "CEL");

	lv_obj_add_event_cb(table, my_table_event_cb, LV_EVENT_DRAW_PART_BEGIN, NULL);
	lv_obj_add_event_cb(table, table_event_cb_bg, LV_EVENT_DRAW_PART_BEGIN, NULL);

	create_bt_icon();
	lv_timer_handler();
}

void setup() {
	tft.init();
	pinMode(backLightPin, OUTPUT);
	digitalWrite(backLightPin, LOW);
	uint16_t darkGray = ((30 & 0xF8) << 8) | ((30 & 0xFC) << 3) | (30 >> 3);
	tft.fillScreen(darkGray);
	tft.setRotation(1);
	Serial.begin(1000000);

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

	SerialBT.begin(myBtName, true);

	create_table();
	digitalWrite(backLightPin, HIGH);
	connectToBt();
	setupSD();
}

void connectToBt() {
	bool connected;
	#ifndef USE_NAME
		SerialBT.setPin(pin);
	#endif

	#ifdef USE_NAME
		connected = SerialBT.connect(slaveName);
	#else
		connected = SerialBT.connect(address);
	#endif

	if (connected) {
		Serial.println("Connected Successfully!");
	} else {
		Serial.println("Initial connect failed. Will retry in loop...");
	}
	update_bt_icon_color(SerialBT.hasClient(), false);
}

void setupSD() {
	SPI.begin(SCK, MISO, MOSI, CS);

	if (!SD.begin(CS)) {
		Serial.println("SD FAIL");
		return;
	}

	ensureFreeSpace();

	// Create new CSV file every boot
	String filename = getNextFilename();
	logFile 		= SD.open(filename, FILE_WRITE);

	if (!logFile) {
		Serial.println("FILE CREATE FAIL");
		return;
	}

	Serial.print("Logging to ");
	Serial.println(filename);

	// CSV header
	logFile.println("_time,_map,_rpm,_tps,_afr,_ign,_inj,_clt,_spd");
	logFile.flush();
}

static char buf_rpm[12];
static char buf_spd[16];
static char buf_afr[12];
static char buf_boost[16];
static char buf_tps[12];
static char buf_clt[12];
static char buf_ign[12];
static char buf_inj[12];
static char buf_bat[12];
static char buf_max_clt[32];
static char buf_max_boost[32];

bool readFrame(uint8_t *frame) {
	static uint8_t buf[5];
	while (SerialBT.available()) {
		uint8_t b = SerialBT.read();
		// Shift buffer left
		buf[0] = buf[1];
		buf[1] = buf[2];
		buf[2] = buf[3];
		buf[3] = buf[4];
		buf[4] = b;
		// Sync on IDCHAR only
		if (buf[1] == 0xA3) {
			memcpy(frame, buf, 5);
			return true;
		}
	}
	return false;
}

void loop() {
	uint8_t frame[5];
	uint8_t channel;
	uint16_t value;
	int chData;
	unsigned long currentMillis = millis();

	if (!SerialBT.connected()) {
		// Attempt reconnection every 5 seconds
		if (currentMillis - previousMillis >= reconnectInterval) {
			previousMillis = currentMillis;
			connectToBt();
		}
	}

	update_bt_icon_color(SerialBT.hasClient(), false);

	// Wait until at least 5 bytes are available
	while (readFrame(frame)) {
		channel = frame[0];
		value   = (frame[2] << 8) | frame[3];

		switch (channel) {
			case 1: case 2: case 3: case 5:
			case 6: case 12: case 19:
			case 24: case 28: case 255:
				break;
			default:
				continue;
		}

		if (channel == 1  && value > 9000) continue;
		if (channel == 24 && value > 250)  continue;
		if (channel == 28 && value > 500)  continue;
		if (channel == 2  && value > 400)  continue;

		chData = static_cast<int>(channel);
		if (chData == 1) {
			rpm = static_cast<int>(value);
			snprintf(buf_rpm, sizeof(buf_rpm), "%d", rpm);
			lv_table_set_cell_value(table, 0, 1, buf_rpm);
		} else if (chData == 28) {
			spd = (static_cast<int>(value) / 2.8f);
			snprintf(buf_spd, sizeof(buf_spd), "%d KM/H", spd);
			lv_table_set_cell_value(table, 0, 3, buf_spd);
		} else if (chData == 12) {
			afr = (static_cast<float>(value) / 10.0f);
			snprintf(buf_afr, sizeof(buf_afr), "%.2f", afr);
			lv_table_set_cell_value(table, 1, 1, buf_afr);
		} else if (chData == 2) {
			mapR = (static_cast<float>(value) / 100.0f);
			boost = (mapR - 1.0132f);
			if(maxboost < boost) { maxboost = boost; }      
			// lv_table_set_cell_value(table, 3, 1, (String(mapR) + " BAR").c_str());
			snprintf(buf_boost, sizeof(buf_boost), "%.2f BAR", boost);
			lv_table_set_cell_value(table, 4, 1, buf_boost);    
			snprintf(buf_max_boost, sizeof(buf_max_boost), "%.2fBAR", maxboost); 
			lv_label_set_text(max_val_label_bst, buf_max_boost);
		} else if (chData == 3) {
			tps = static_cast<int>(value);
			snprintf(buf_tps, sizeof(buf_tps), "%d %%", tps);
			lv_table_set_cell_value(table, 2, 1, buf_tps);
		} else if (chData == 24) {
			clt = static_cast<int>(value);    
			if(maxclt < clt) { maxclt = clt; }    
			snprintf(buf_clt, sizeof(buf_clt), "%d°C", clt);
			lv_table_set_cell_value(table, 1, 3, buf_clt);    
			snprintf(buf_max_clt, sizeof(buf_max_clt), "%d°C", maxclt); 
			lv_label_set_text(max_val_label_clt, buf_max_clt);
		} else if (chData == 6) {
			ign = (static_cast<int>(value) / 2);
			snprintf(buf_ign, sizeof(buf_ign), "%d °", ign);
			lv_table_set_cell_value(table, 3, 3, buf_ign);
		} else if (chData == 19) {
			inj = (static_cast<int>(value) / 2);
			snprintf(buf_inj, sizeof(buf_inj), "%d %%", inj);
			lv_table_set_cell_value(table, 3, 1, buf_inj);
		} else if (chData == 5) {
			bat = (static_cast<float>(value) / 37.0f);
			snprintf(buf_bat, sizeof(buf_bat), "%.2f V", bat);
			lv_table_set_cell_value(table, 2, 3, buf_bat);
		} else if (chData == 255) {
			cel = decodeCheckEngine(value);
		}
	}

	buzzerOn = (cel > 0 || clt > 110 || rpm > 7000 || boost > 1.20 || (bat < 11.00 && bat > 1.00));
	digitalWrite(buzzerPin, (millis() % 600 < 300) && buzzerOn);

	// ---------------- SD LOGGING ----------------
	if (logFile && rpm > 0 && millis() - lastLogMillis >= logInterval) {
		lastLogMillis = millis();

		char line[128];

		// Scale floats to integers
		int map_i = (int)(mapR * 100.0f);   // _map x100
		int afr_i = (int)(afr  * 100.0f);   // _afr x100

		// Clamp against sensor glitches
		if (map_i < 0) map_i = 0;
		if (afr_i < 0) afr_i = 0;

		logCounter++;

		int len = snprintf(
			line, sizeof(line),
			"%lu,%d,%d,%d,%d,%d,%d,%d,%d",
			logCounter,// _time
			map_i,     // _map_x100
			rpm,       // _rpm
			tps,       // _tps
			afr_i,     // _afr_x100
			ign,       // _ign
			inj,       // _inj
			clt,       // _clt
			spd        // _spd
		);

		if (len > 0 && len < (int)sizeof(line)) {
			logFile.write((uint8_t*)line, len);
			logFile.write('\n');
			if (millis() - lastFlush > 2000) {
				logFile.flush();
				lastFlush = millis();
			}
		}
	}

	lv_obj_invalidate(table);
	lv_timer_handler();
}

int decodeCheckEngine(uint16_t value) {
	int cel_codes = 0; string cel_names = "";
	if (value == 0) {
		return 0;
	}
	else if (value > 0) {
		if (value & (1 << 0)) {
			cel_codes++;  // Bit 0
			cel_names = "CLT ";
		}
		if (value & (1 << 1)) {
			//cel_codes++;  // Bit 1
			//cel_names += "IAT ";
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
			//cel_codes++;  // Bit 8
			//cel_names += "FF SENSOR ";
		}
		if (value & (1 << 9)) {
			//cel_codes++;  // Bit 9
			//cel_names += "DBW ";
		}
		if (value & (1 << 10)) {
			//cel_codes++;  // Bit 10
			//cel_names += "FPR ";
		}
		lv_table_set_cell_value(table, 5, 1, cel_names.c_str());
		return cel_codes;
	}
}

// Cell alignment fix
void my_table_event_cb(lv_event_t * e) {
	lv_obj_t * table = lv_event_get_target(e);
	lv_obj_draw_part_dsc_t * dsc = (lv_obj_draw_part_dsc_t *)lv_event_get_param(e);

	if (dsc->part == LV_PART_ITEMS) {
		uint16_t row = dsc->id / lv_table_get_col_cnt(table);
		uint16_t col = dsc->id % lv_table_get_col_cnt(table);

		dsc->label_dsc->font = &ui_font_JBM_18;
		dsc->label_dsc->align = LV_TEXT_ALIGN_CENTER;
		if ((row == 0 && col == 1) || (row == 0 && col == 3) || (row == 1 && col == 1) || (row == 1 && col == 3) || (row == 2 && col == 1) || (row == 2 && col == 3) || (row == 3 && col == 1) || (row == 3 && col == 3) ||
		(row == 4 && col == 1) || (row == 4 && col == 2)) {
			dsc->label_dsc->align = LV_TEXT_ALIGN_RIGHT;
		}
		if ((row == 5 && col == 1)) {
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
		if (row == 4 && col == 1 && value > 1.10) {
			bg_color = lv_color_make(0, 0, 255);
			text_color = lv_color_white();
		}
		if (row == 4 && col == 2) {
			bg_color = lv_color_make(55, 55, 55);
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

void update_bt_icon_color(bool is_connected, bool firstTime) {
	if (btIconSts != is_connected || firstTime) {
		if (!style_initialized) {
			lv_style_init(&style_bt);
			style_initialized = true;
		}
		if (is_connected) {
			lv_style_set_text_color(&style_bt, lv_color_make(0, 255, 0)); // Green
		} else {
			lv_style_set_text_color(&style_bt, lv_color_make(0, 0, 255)); // Red
		}
		lv_obj_add_style(bt_icon_label, &style_bt, 0);
		btIconSts = is_connected;
	}
}

void create_bt_icon() {
	bt_icon_label = lv_label_create(lv_scr_act());
	lv_label_set_text(bt_icon_label, LV_SYMBOL_BLUETOOTH);
	lv_obj_set_style_text_font(bt_icon_label, &lv_font_montserrat_28, LV_PART_MAIN);
	lv_obj_align(bt_icon_label, LV_ALIGN_BOTTOM_RIGHT, -3, -5);
	update_bt_icon_color(SerialBT.hasClient(), true);

	max_icon_label = lv_label_create(lv_scr_act());
	lv_label_set_text(max_icon_label, "CLT                BOOST");
	lv_obj_set_style_text_font(max_icon_label, &ui_font_JBM_10, LV_PART_MAIN);
	lv_style_init(&style_max0);
	lv_style_set_text_color(&style_max0, lv_color_make(255, 255, 255));
	lv_obj_add_style(max_icon_label, &style_max0, 0);
	lv_obj_align(max_icon_label, LV_ALIGN_BOTTOM_RIGHT, -3, -64);

	max_icon_label1 = lv_label_create(lv_scr_act());
	lv_label_set_text(max_icon_label1, LV_SYMBOL_CHARGE);
	lv_obj_set_style_text_font(max_icon_label1, &lv_font_montserrat_14, LV_PART_MAIN);
	lv_style_init(&style_max3);
	lv_style_set_text_color(&style_max3, lv_color_make(255, 255, 255));
	lv_obj_add_style(max_icon_label1, &style_max3, 0);
	lv_obj_align(max_icon_label1, LV_ALIGN_BOTTOM_RIGHT, -75, -60);

	max_val_label_clt = lv_label_create(lv_scr_act());
	lv_obj_set_style_text_font(max_val_label_clt, &ui_font_JBM_15, LV_PART_MAIN);
	lv_style_init(&style_max1);
	lv_style_set_text_color(&style_max1, lv_color_make(255, 255, 255));
	lv_obj_add_style(max_val_label_clt, &style_max1, 0);
	lv_obj_align(max_val_label_clt, LV_ALIGN_BOTTOM_LEFT, 173, -42);
	lv_label_set_text(max_val_label_clt, String(" ").c_str());

	max_val_label_bst = lv_label_create(lv_scr_act());
	lv_obj_set_style_text_font(max_val_label_bst, &ui_font_JBM_15, LV_PART_MAIN);
	lv_style_init(&style_max2);
	lv_style_set_text_color(&style_max2, lv_color_make(255, 255, 255));
	lv_obj_add_style(max_val_label_bst, &style_max2, 0);
	lv_obj_align(max_val_label_bst, LV_ALIGN_BOTTOM_RIGHT, -3, -42);
	lv_label_set_text(max_val_label_bst, String(" ").c_str());
}

String getNextFilename() {
	int index = 1;
	char name[20];
	while (true) {
		sprintf(name, "/log_%04d.csv", index);
		if (!SD.exists(name)) {
			return String(name);
		}
		index++;
	}
}
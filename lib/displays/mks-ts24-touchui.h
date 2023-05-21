#pragma once

#include <Arduino.h>
#include <SPI.h>

// TODO: move this out
#define LCD_SCK GPIO_NUM_18
#define LCD_MISO GPIO_NUM_19
#define LCD_MOSI GPIO_NUM_23
#define LCD_RS GPIO_NUM_33
#define LCD_EN GPIO_NUM_5
#define LCD_RST GPIO_NUM_27
#define LCD_CS GPIO_NUM_25
#define TOUCH_CS GPIO_NUM_26
#include "TFT_eSPI.h"


#include "lvgl.h"
// #include "demos/lv_demos.h"

namespace buff {

namespace monitoring_display {

auto static tft = TFT_eSPI();

// const uint16_t FILL_COLOR = static_cast<uint16_t>(0xCCCCC);

static bool displaySetupFully = false;


#define LV_BUF_SIZE 10 * LV_HOR_RES_MAX
LVGL_UI_PAGE_t mainPage;

static lv_disp_buf_t displayBuffer;
static lv_color_t bmpBuffer[LV_BUF_SIZE];

void tftSetup() {
    tft.init();
    /*
        Visualization of the TS24
          Makerbase
        A            B
         ------------
        |            |
        |            |
        |            |
        |            |
        |            |
         ------------
        C            D
    */
    // tft.setRotation(0);  // right-to-left, (0,0) = B
    // tft.setRotation(1);  // right-to-left, (0,0) = B
    // tft.setRotation(2);  // right-to-left, (0,0) = C
    // tft.setRotation(3);  // right-to-left, (0,0) = D
    // tft.setRotation(4);  // right-to-left, (0,0) = D
    // tft.setRotation(5);  // left-to-left, (0,0) = B
    tft.setRotation(6);     // left-to-right, (0,0) = A
    // tft.setRotation(7);  // left-to-right, (0,0) = C

    // tft.fillScreen(FILL_COLOR);
}

void lvSetup() {
    lv_init();

    lv_disp_buf_init(&disp_buf, bmp_public_buf, NULL, LV_BUF_SIZE);
    lv_disp_drv_t disp_drv;
    lv_disp_drv_init(&disp_drv);
    disp_drv.hor_res = tft.width();
    disp_drv.ver_res = tft.height();
    disp_drv.flush_cb = flushCB;
    disp_drv.buffer = &disp_buf;
    lv_disp_drv_register(&disp_drv);

    lv_indev_drv_t indev_drv;
    lv_indev_drv_init(&indev_drv);
    indev_drv.type = LV_INDEV_TYPE_POINTER;
    indev_drv.read_cb = readCB;
    lv_indev_drv_register(&indev_drv);
}

void setupDisplay() {
    tftSetup()
    lvSetup();

    /*Create a container with ROW flex direction*/
    lv_obj_t* cont_row = lv_obj_create(lv_scr_act());
    lv_obj_set_size(cont_row, tft.width(), 75);
    lv_obj_align(cont_row, LV_ALIGN_TOP_MID, 0, 5);
    lv_obj_set_flex_flow(cont_row, LV_FLEX_FLOW_ROW);

    /*Create a container with COLUMN flex direction*/
    lv_obj_t* cont_col = lv_obj_create(lv_scr_act());
    lv_obj_set_size(cont_col, 200, 150);
    lv_obj_align_to(cont_col, cont_row, LV_ALIGN_OUT_BOTTOM_MID, 0, 5);
    lv_obj_set_flex_flow(cont_col, LV_FLEX_FLOW_COLUMN);

    uint32_t i;
    for (i = 0; i < 10; i++) {
        lv_obj_t* obj;
        lv_obj_t* label;

        /*Add items to the row*/
        obj = lv_btn_create(cont_row);
        lv_obj_set_size(obj, 100, LV_PCT(100));

        label = lv_label_create(obj);
        lv_label_set_text_fmt(label, "Item: %" LV_PRIu32 "", i);
        lv_obj_center(label);

        /*Add items to the column*/
        obj = lv_btn_create(cont_col);
        lv_obj_set_size(obj, LV_PCT(100), LV_SIZE_CONTENT);

        label = lv_label_create(obj);
        lv_label_set_text_fmt(label, "Item: %" LV_PRIu32, i);
        lv_obj_center(label);
    }

    displaySetupFully = true;
}

void flushCB(lv_disp_drv_t* disp, const lv_area_t* area, lv_color_t* color_p) {
    uint32_t w = (area->x2 - area->x1 + 1);
    uint32_t h = (area->y2 - area->y1 + 1);

    tft.startWrite();
    tft.setAddrWindow(area->x1, area->y1, w, h);

// #if defined(USE_LCD_DMA)
    tft.pushColorsDMA(&color_p->full, w * h, true);
// #else
//     tft.pushColors(&color_p->full, w * h, true);
// #endif

    tft.endWrite();
    lv_disp_flush_ready(disp);
}

bool readCB(struct _lv_indev_drv_t* indev_drv, lv_indev_data_t* data) {
    uint16_t touchX = 0, touchY = 0;
    static uint16_t last_x = 0;
    static uint16_t last_y = 0;
    boolean touched = tft.getTouch(&touchY, &touchX);

    if (touchX > 480) {
        touchX = 480;
    }

    touchX = LV_HOR_RES_MAX - touchX;
    touchY = LV_VER_RES_MAX - touchY;

    if (touched != false) {
        last_x = touchX;
        last_y = touchY;
        data->point.x = last_x;
        data->point.y = last_y;
        data->state = LV_INDEV_STATE_PR;
    } else {
        data->point.x = last_x;
        data->point.y = last_y;
        data->state = LV_INDEV_STATE_REL;
    }
    return false;
}

void displayPH(const float pH, const float convertedPH, const float rawPH_mvag, const float calibratedPH_mvag, const ulong asOf) {
    if (!displaySetupFully) {
        return;
    }
}

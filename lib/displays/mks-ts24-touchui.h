#pragma once

#include <Arduino.h>
#include <SPI.h>

// TODO: move this out
// #define LCD_SCK GPIO_NUM_18
// #define LCD_MISO GPIO_NUM_19
// #define LCD_MOSI GPIO_NUM_23
// #define LCD_RS GPIO_NUM_33
#define LCD_EN GPIO_NUM_5
// #define LCD_RST GPIO_NUM_27
// #define LCD_CS GPIO_NUM_25
#define TOUCH_CS GPIO_NUM_26
#include "TFT_eSPI.h"
// #define TFT_WIDTH TS24_WIDTH
#include "lvgl.h"
// #include "demos/lv_demos.h"

namespace buff {

namespace monitoring_display {

// Portrait Mode dimensions
static const uint16_t TS24_WIDTH = 240;
static const uint16_t TS24_HEIGHT = 320;


#define LANDSCAPE_SCREEN
#ifdef LANDSCAPE_SCREEN
static const uint16_t SCREEN_WIDTH = TS24_WIDTH;
static const uint16_t SCREEN_HEIGHT = TS24_HEIGHT;
#else  // Portrait
static const uint16_t SCREEN_WIDTH = TFT_HEIGHT;
static const uint16_t SCREEN_HEIGHT = TFT_WIDTH;
#endif



auto static tft = TFT_eSPI(SCREEN_WIDTH, TS24_HEIGHT);

static bool displaySetupFully = false;

static lv_disp_draw_buf_t drawBuf;
static lv_color_t buf[SCREEN_WIDTH * 10];

/* Display flushing */
void flushCB(lv_disp_drv_t* disp, const lv_area_t* area, lv_color_t* color_p) {
    uint32_t w = (area->x2 - area->x1 + 1);
    uint32_t h = (area->y2 - area->y1 + 1);

    tft.startWrite();
    tft.setAddrWindow(area->x1, area->y1, w, h);
    tft.pushColors((uint16_t*)&color_p->full, w * h, true);
    tft.endWrite();

    lv_disp_flush_ready(disp);
}

void touchReadCB(lv_indev_drv_t* indev_driver, lv_indev_data_t* data) {
    uint16_t touchX, touchY;

    bool touched = tft.getTouch(&touchX, &touchY, 600);

    touchX = tft.width() - touchX;
    touchY = tft.height() - touchY;

    if (!touched) {
        data->state = LV_INDEV_STATE_REL;
    } else {
        data->state = LV_INDEV_STATE_PR;

        /*Set the coordinates*/
        data->point.x = touchX;
        data->point.y = touchY;

        Serial.print("Data x ");
        Serial.println(touchX);

        Serial.print("Data y ");
        Serial.println(touchY);
    }
}

void event_handler(lv_event_t* e) {
    lv_event_code_t code = lv_event_get_code(e);

    if (code == LV_EVENT_CLICKED) {
        Serial.println("Clicked");
    } else if (code == LV_EVENT_VALUE_CHANGED) {
        Serial.println("Toggled");
    }
}

void tftSetup() {
    tft.begin();
    // tft.init();
    tft.initDMA();
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
    tft.setRotation(6);  // left-to-right, (0,0) = A
    // tft.setRotation(7);  // left-to-right, (0,0) = C

    uint16_t calData[5] = {275, 3620, 264, 3532, 1};
    tft.setTouch(calData);
}

void lvSetup() {
    lv_init();

    lv_disp_draw_buf_init(&drawBuf, buf, NULL, SCREEN_WIDTH * 10);

    /*Initialize the display*/
    static lv_disp_drv_t disp_drv;
    lv_disp_drv_init(&disp_drv);
    /*Change the following line to your display resolution*/
    disp_drv.hor_res = SCREEN_WIDTH;
    disp_drv.ver_res = SCREEN_HEIGHT;
    disp_drv.flush_cb = flushCB;
    disp_drv.draw_buf = &drawBuf;
    lv_disp_drv_register(&disp_drv);

    /*Initialize the (dummy) input device driver*/
    static lv_indev_drv_t indev_drv;
    lv_indev_drv_init(&indev_drv);
    indev_drv.type = LV_INDEV_TYPE_POINTER;
    indev_drv.read_cb = touchReadCB;
    lv_indev_drv_register(&indev_drv);
}

void createMainPage() {
    /*Create a container with ROW flex direction*/
    lv_obj_t* cont_row = lv_obj_create(lv_scr_act());
    lv_obj_set_size(cont_row, SCREEN_WIDTH, 75);
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
        lv_obj_add_event_cb(obj, event_handler, LV_EVENT_ALL, NULL);
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
}

void enableDisplayHardware() {
    pinMode(LCD_EN, OUTPUT);
    digitalWrite(LCD_EN, LOW);
}

void demoLabels() {
    /* Create simple label */
    lv_obj_t* label = lv_label_create(lv_scr_act());
    auto labelText = "TESTING1";
    lv_label_set_text(label, labelText);
    // lv_obj_align(label, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_pos(label, 0, 0);

    lv_obj_t* label2 = lv_label_create(lv_scr_act());
    auto labelText2 = "T2";
    lv_label_set_text(label2, labelText2);
    lv_obj_set_pos(label2, 200, 50);

    lv_obj_t* label3 = lv_label_create(lv_scr_act());
    auto labelText3 = "T3";
    lv_label_set_text(label3, labelText3);
    lv_obj_set_pos(label3, 200, 50);

    lv_obj_t* label5 = lv_label_create(lv_scr_act());
    auto labelText5 = "T5";
    lv_label_set_text(label5, labelText5);
    lv_obj_set_pos(label5, 230, 320);

    lv_obj_t* label4 = lv_label_create(lv_scr_act());
    auto labelText4 = "TESTING4";
    lv_label_set_text(label4, labelText4);
    lv_obj_set_pos(label4, 200, 200);
}

void demoButtons() {
    lv_obj_t* label;

    lv_obj_t* btn1 = lv_btn_create(lv_scr_act());
    lv_obj_add_event_cb(btn1, event_handler, LV_EVENT_ALL, NULL);
    lv_obj_align(btn1, LV_ALIGN_CENTER, 0, -40);

    label = lv_label_create(btn1);
    lv_label_set_text(label, "Button");
    lv_obj_center(label);

    lv_obj_t* btn2 = lv_btn_create(lv_scr_act());
    lv_obj_add_event_cb(btn2, event_handler, LV_EVENT_ALL, NULL);
    lv_obj_align(btn2, LV_ALIGN_CENTER, 0, 40);
    lv_obj_add_flag(btn2, LV_OBJ_FLAG_CHECKABLE);
    lv_obj_set_height(btn2, LV_SIZE_CONTENT);

    label = lv_label_create(btn2);
    lv_label_set_text(label, "Toggle");
    lv_obj_center(label);
}

void setupDisplay() {
    enableDisplayHardware();
    tftSetup();
    lvSetup();


    createMainPage();
    // demoLabels();
    // demoButtons();

    // Done
    displaySetupFully = true;
}

void displayPH(const float pH, const float convertedPH, const float rawPH_mvag, const float calibratedPH_mvag, const ulong asOf) {
    if (!displaySetupFully) {
        return;
    }
}

void loopDisplay() {
    lv_timer_handler(); /* let the GUI do its work */
}

}  // namespace monitoring_display
}  // namespace buff

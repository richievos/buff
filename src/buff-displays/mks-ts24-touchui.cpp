#ifdef DISPLAY_MKS_TS24_TOUCH
#include <Arduino.h>
#include <SPI.h>

#include "buff-displays/monitoring-display.h"

// TODO: move this out
#define LCD_EN GPIO_NUM_5
#define TOUCH_CS GPIO_NUM_26
#include "TFT_eSPI.h"
#include "lvgl.h"

// stdlib
#include <memory>

// My Libs
#include "mqtt-common.h"
#include "readings/alk-measure-common.h"
#include "readings/reading-store.h"

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

lv_obj_t* timeLabel;
lv_obj_t* phLabel;

lv_obj_t* triggerRoller;
lv_obj_t* readingsList;

lv_obj_t* debugRawPHLabel;

std::shared_ptr<mqtt::Publisher> publisher;

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

        // Serial.print("Data x ");
        // Serial.println(touchX);
        // Serial.print("Data y ");
        // Serial.println(touchY);
    }
}

void event_handler(lv_event_t* e) {
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t* obj = lv_event_get_target(e);

    if (code == LV_EVENT_CLICKED) {
        Serial.println("Clicked");
        char buf[128];
        lv_roller_get_selected_str(triggerRoller, buf, sizeof(buf));
        std::string title(buf);

        publisher->publishMeasureAlk(title, millis());
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

void enableDisplayHardware() {
    pinMode(LCD_EN, OUTPUT);
    digitalWrite(LCD_EN, LOW);
}

void createMainPage() {
    lv_obj_t* mainPage = lv_obj_create(lv_scr_act());
    lv_obj_set_flex_flow(mainPage, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_size(mainPage, SCREEN_WIDTH, SCREEN_HEIGHT);
    lv_obj_set_style_pad_column(mainPage, 5, 0);
    lv_obj_set_style_pad_row(mainPage, 0, 0);
    lv_obj_set_align(mainPage, LV_ALIGN_TOP_LEFT);
    lv_obj_clear_flag(mainPage, LV_OBJ_FLAG_SCROLLABLE);

    /***************
     * Title bar
     ***************/
    static lv_style_t headerStyle;
    lv_style_init(&headerStyle);
    lv_style_set_bg_color(&headerStyle, lv_palette_main(LV_PALETTE_GREY));
    lv_style_set_text_color(&headerStyle, lv_color_black());

    lv_obj_t* headerBar = lv_obj_create(mainPage);
    lv_obj_set_size(headerBar, SCREEN_WIDTH, LV_SIZE_CONTENT);
    lv_obj_set_style_pad_row(headerBar, 0, 0);
    lv_obj_add_style(headerBar, &headerStyle, 0);

    // time
    timeLabel = lv_label_create(headerBar);
    lv_obj_set_align(timeLabel, LV_ALIGN_BOTTOM_LEFT);
    lv_label_set_text(timeLabel, "00:00:00");
    lv_obj_set_size(timeLabel, 120, LV_SIZE_CONTENT);

    // ph
    phLabel = lv_label_create(headerBar);
    lv_obj_set_align(phLabel, LV_ALIGN_BOTTOM_RIGHT);
    lv_label_set_text(phLabel, "pH: 00.0");
    lv_obj_set_size(phLabel, 60, LV_SIZE_CONTENT);

    /***************
     * Re-triggering row
     ***************/
    lv_obj_t* triggerRow = lv_obj_create(mainPage);
    lv_obj_set_size(triggerRow, LV_PCT(100), LV_SIZE_CONTENT);
    lv_obj_set_flex_flow(triggerRow, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(triggerRow, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_column(triggerRow, 5, 0);

    // roller
    triggerRoller = lv_roller_create(triggerRow);
    lv_roller_set_visible_row_count(triggerRoller, 3);
    lv_obj_center(triggerRoller);
    lv_obj_set_size(triggerRoller, LV_PCT(65), 70);
    // lv_obj_add_event_cb(triggerRoller, event_handler, LV_EVENT_ALL, NULL);

    // trigger button
    lv_obj_t* triggerBtn = lv_btn_create(triggerRow);
    lv_obj_set_size(triggerBtn, LV_PCT(35), 40);
    lv_obj_add_event_cb(triggerBtn, event_handler, LV_EVENT_ALL, NULL);

    lv_obj_t* triggerLabel = lv_label_create(triggerBtn);
    lv_label_set_text(triggerLabel, "Trigger");
    lv_obj_set_align(triggerLabel, LV_ALIGN_CENTER);
    lv_obj_set_style_text_align(triggerLabel, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_size(triggerLabel, LV_SIZE_CONTENT, LV_SIZE_CONTENT);

    /***************
     * List of readings
     ***************/
    readingsList = lv_obj_create(mainPage);
    lv_obj_set_size(readingsList, LV_PCT(100), 175);
    lv_obj_align_to(readingsList, triggerRow, LV_ALIGN_OUT_BOTTOM_MID, 0, 5);
    lv_obj_set_flex_flow(readingsList, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_row(readingsList, 5, 0);

    /***************
     * Debug bar
     ***************/
    lv_obj_t* debugBar = lv_obj_create(mainPage);
    lv_obj_set_size(debugBar, LV_PCT(100), LV_SIZE_CONTENT);
    lv_obj_align(debugBar, LV_ALIGN_TOP_MID, 0, 5);
    lv_obj_set_flex_flow(debugBar, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(debugBar, LV_FLEX_ALIGN_END, LV_FLEX_ALIGN_END, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_column(debugBar, 5, 0);

    static lv_style_t smallLabelStyle;
    lv_style_init(&smallLabelStyle);
    lv_style_set_text_font(&smallLabelStyle, &lv_font_montserrat_10);

    // raw ph
    debugRawPHLabel = lv_label_create(debugBar);
    lv_label_set_text(debugRawPHLabel, "raw pH: 00.0");
    lv_obj_set_size(debugRawPHLabel, LV_PCT(100), LV_SIZE_CONTENT);
    lv_obj_add_style(debugRawPHLabel, &smallLabelStyle, 0);
}

void refreshTriggerList(const std::set<std::string>& titles) {
    std::string concatTitle = "";
    for (auto title : titles) {
        if (concatTitle.size() > 0) {
            concatTitle += "\n";
        }
        concatTitle += title;
    }

    lv_roller_set_options(triggerRoller,
                          concatTitle.c_str(),
                          LV_ROLLER_MODE_NORMAL);
}

void refreshReadingList(const std::vector<std::reference_wrapper<alk_measure::PersistedAlkReading>> alkReadings) {
    lv_obj_clean(readingsList);

    const size_t bufferSize = 256;
    char printBuff[bufferSize];

    for (int i = 0; i < std::min(alkReadings.size(), static_cast<size_t>(10)); i++) {
        auto readingRef = alkReadings[i];
        auto reading = readingRef.get();
        lv_obj_t* btn = lv_btn_create(readingsList);

        lv_obj_t* label = lv_label_create(btn);
        snprintf(printBuff, bufferSize, "%s: %.1f dkh", reading.title.c_str(), reading.alkReadingDKH);
        lv_label_set_text(label, printBuff);
        lv_obj_set_size(btn, LV_PCT(100), LV_SIZE_CONTENT);
    }
}

void updateDisplay(std::shared_ptr<reading_store::ReadingStore> readingStore) {
    auto alkReadings = readingStore->getReadingsSortedByAsOf();

    refreshTriggerList(readingStore->getRecentTitles(alkReadings));
    refreshReadingList(alkReadings);
}

void setupDisplay(std::shared_ptr<reading_store::ReadingStore> readingStore, std::shared_ptr<mqtt::Publisher> pub) {
    publisher = pub;

    enableDisplayHardware();
    tftSetup();
    lvSetup();

    createMainPage();
    updateDisplay(readingStore);

    displaySetupFully = true;
}

// TODO: move this to a shared helper
void renderTime(char* temp, size_t bufferSize, const unsigned long timeInSec) {
    // millis to time
    const time_t rawtime = (time_t)timeInSec;
    struct tm* dt = gmtime(&rawtime);

    // TODO: HACK convert to PDT
    dt->tm_hour += (24 - 7) % 24;

    // format
    strftime(temp, bufferSize, "%H:%M:%S", dt);
}

void displayPH(const float rawPH, const float calibratedPH, const float rawPH_mvag, const float calibratedPH_mvag, const ulong asOfMS, const unsigned long asOfAdjustedSec) {
    if (!displaySetupFully) {
        return;
    }

    size_t bufSize = 150;
    char buf[bufSize];

    // ph
    sprintf(buf, "ph: %2.1f", calibratedPH_mvag);
    lv_label_set_text(phLabel, buf);

    // debug ph 🕑
    sprintf(buf, "ph: %2.1f (%2.1f), rawPH: %2.1f (%2.1f)", calibratedPH, calibratedPH_mvag, rawPH, rawPH_mvag);
    lv_label_set_text(debugRawPHLabel, buf);

    // time
    renderTime(buf, bufSize, asOfAdjustedSec);
    lv_label_set_text(timeLabel, buf);
}

void loopDisplay() {
    lv_timer_handler();
}

}  // namespace monitoring_display
}  // namespace buff

#endif

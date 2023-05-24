#ifdef DISPLAY_MKS_MONITORING
#include <Arduino.h>
#include <SPI.h>

#include "buff-displays/monitoring-display.h"

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

// #define USE_MONITORING_DISPLAY_BUFFER

namespace buff {

namespace monitoring_display {

auto static tft = TFT_eSPI();
#ifdef USE_MONITORING_DISPLAY_BUFFER
auto static spr = TFT_eSprite(&tft);
#endif

static bool displaySetupFully = false;

const uint16_t FILL_COLOR = static_cast<uint16_t>(0xCCCCC);

void setupDisplay(std::shared_ptr<reading_store::ReadingStore> readingStore) {
    pinMode(LCD_EN, OUTPUT);
    digitalWrite(LCD_EN, LOW);

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
    tft.setRotation(6);  // left-to-right, (0,0) = A
    // tft.setRotation(7);  // left-to-right, (0,0) = C

    tft.fillScreen(FILL_COLOR);

#ifdef USE_MONITORING_DISPLAY_BUFFER
    spr.createSprite(tft.width(), tft.height());
#endif

    displaySetupFully = true;
    // Serial.println("Setup display");
    // #ifdef USE_VERSION_003
    // #define LCD_BLK_ON digitalWrite(LCD_EN, HIGH)
    // #define LCD_BLK_OFF digitalWrite(LCD_EN, LOW)
    // #else
    // #define LCD_BLK_ON digitalWrite(LCD_EN, LOW)
    // #define LCD_BLK_OFF digitalWrite(LCD_EN, HIGH)
    // #endif
}

void displayPH(const float pH, const float convertedPH, const float rawPH_mvag, const float calibratedPH_mvag, const ulong asOfMS, const ulong asOfAdjustedSec) {
    if (!displaySetupFully) {
        return;
    }

#ifdef USE_MONITORING_DISPLAY_BUFFER
    auto target = spr;
#else
    auto target = tft;
#endif

    target.setTextSize(1);           // Normal 1:1 pixel scale
    target.setTextColor(TFT_WHITE);  // Draw white text
    target.setCursor(0, 0);          // Start at top-left corner
    target.setTextFont(2);

    target.print(F("asOf: "));
    target.println(asOfMS);

    target.fillScreen(FILL_COLOR);
    target.print(F("calib ph: "));
    target.println(convertedPH, 3);
    target.print(F("mvag(calib): "));
    target.println(calibratedPH_mvag, 3);

    target.println();
    target.println();
    target.println();
    target.println();

    target.print(F("raw pH: "));
    target.println(pH, 3);
    target.print(F("mavg(raw pH): "));
    target.println(rawPH_mvag, 3);

#ifdef USE_MONITORING_DISPLAY_BUFFER
    target.pushSprite(0, 0);
#endif
}

void loopDisplay() {}
void updateDisplay(std::shared_ptr<reading_store::ReadingStore> readingStore) {}

}  // namespace monitoring_display
}  // namespace buff

#endif

#pragma once

#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>


namespace buff {
namespace monitoring_display {

const uint SCREEN_WIDTH = 128;  // OLED display width, in pixels
const uint SCREEN_HEIGHT = 64;  // OLED display height, in pixels

const uint8_t DISPLAY_I2C_ADDRESS = 0x3c;

#define OLED_RESET -1  // Reset pin # (or -1 if sharing Arduino reset pin)
::Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

static bool displaySetupFully = false;

void setupDisplay() {
    // SSD1306_SWITCHCAPVCC = generate display voltage from 3.3V internally
    if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
        Serial.println(F("SSD1306 allocation failed"));
    } else {
        displaySetupFully = true;
    }
}

void displayPH(const float pH, const float convertedPH, const float rawPH_mvag, const float calibratedPH_mvag, const ulong asOf) {
    if (!displaySetupFully) {
        return;
    }
    display.clearDisplay();

    display.setTextSize(1);       // Normal 1:1 pixel scale
    display.setTextColor(WHITE);  // Draw white text
    display.setCursor(0, 0);      // Start at top-left corner
    display.cp437(true);          // Use full 256 char 'Code Page 437' font

    display.print(F("pH="));
    display.println(pH, 3);
    display.print(F("mavg(pH)="));
    display.println(rawPH_mvag, 3);

    display.print(F("calib="));
    display.println(convertedPH, 3);
    display.print(F("mvag(calib)="));
    display.println(calibratedPH_mvag, 3);

    display.print(F("asOf="));
    display.println(asOf);

    display.display();
}

}  // namespace monitoring_display
}  // namespace buff
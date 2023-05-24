#pragma once

#include "mks-skinny/I2SOut.h"
#include "mks-skinny/Pins.h"

#define BEEPER I2SO(7)

extern int i2s_out_stop();

static void setup_mks() {
    Serial.println("Initializing I2S");
    i2s_out_init();

    pinMode(BEEPER, OUTPUT);
    digitalWrite(BEEPER, LOW);
}

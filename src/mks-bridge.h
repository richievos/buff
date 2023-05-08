#pragma once

#include "mks-skinny/I2SOut.h"
#include "mks-skinny/Pins.h"

#define BEEPER I2SO(7)

extern int i2s_out_stop();

void setup_mks() {
    Serial.println("Initializing I2S");
    i2s_out_init();

    // ?
    // mks-Grbl_Esp32/src/Machines/i2s_out_xyz_mks_dlc32.h
    pinMode(BEEPER, OUTPUT);
    digitalWrite(BEEPER, LOW);
}

void loop_mks() {
}

#pragma once

/*******************************
 * Board Config
 *******************************/
// Arduino Board
// enum ArduinoPinConfig {
//   DIR_PIN = 6,
//   STEP_PIN = 7,

//   MS1_PIN = 10,
//   MS2_PIN = 11,
//   MS3_PIN = 12,
// };

// ESP32
enum ArduinoPinConfig {
    STEPPER_RST_PIN = 2,

    FILL_WATER_DIR_PIN = 32,
    FILL_WATER_STEP_PIN = 33,

    // TODO: delete these, not using them anymore
    MS1_PIN = 25,
    MS2_PIN = 26,
    MS3_PIN = 27,

    REAGENT_DIR_PIN = 18,
    REAGENT_STEP_PIN = 19,

    DRAIN_WATER_DIR_PIN = 16,
    DRAIN_WATER_STEP_PIN = 17,

    // MS1_PIN = 25,
    // MS2_PIN = 26,
    // MS3_PIN = 27,

};
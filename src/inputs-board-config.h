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
struct ArduinoPinConfig {
    short STEPPER_EN_PIN;

    short FILL_WATER_DIR_PIN;
    short FILL_WATER_STEP_PIN;

    short REAGENT_DIR_PIN;
    short REAGENT_STEP_PIN;

    short DRAIN_WATER_DIR_PIN;
    short DRAIN_WATER_STEP_PIN;
};

const ArduinoPinConfig ESP32_CONFIG = {
    .STEPPER_EN_PIN = 2,

    .FILL_WATER_DIR_PIN = 32,
    .FILL_WATER_STEP_PIN = 33,

    .REAGENT_DIR_PIN = 18,
    .REAGENT_STEP_PIN = 19,

    .DRAIN_WATER_DIR_PIN = 16,
    .DRAIN_WATER_STEP_PIN = 17,
};

/************************************************
 * MKS DLC32
 * Diagrams: https://github.com/makerbase-mks/MKS-DLC32/blob/main/hardware/
 * Source: https://github.com/makerbase-mks/MKS-DLC32-FIRMWARE
 * Pins: https://github.com/makerbase-mks/MKS-DLC32-FIRMWARE/blob/main/Firmware/Grbl_Esp32/src/Machines/i2s_out_xyz_mks_dlc32.h
 * ESP-32S pinout: https://randomnerdtutorials.com/esp32-pinout-reference-gpios/
 ************************************************/
struct mks {
    short MKS_X_STEP_PIN = 12;
    short MKS_X_DIRECTION_PIN = 14;
    short MKS_Y_STEP_PIN = 26;
    short MKS_Y_DIRECTION_PIN = 15;
    short MKS_Z_STEP_PIN = 27;
    short MKS_Z_DIRECTION_PIN = 33;
    short MKS_X_LIMIT_PIN = 17;
    short MKS_Y_LIMIT_PIN = 4;
    short MKS_Z_LIMIT_PIN = 16;

    // OK to comment out to use pin for other features
    short MKS_STEPPERS_DISABLE_PIN = 13;

    // short SPINDLE_TYPE = SpindleType::PWM;
    short MKS_SPINDLE_OUTPUT_PIN = 2;   // labeled SpinPWM
    short MKS_SPINDLE_ENABLE_PIN = 22;  // labeled SpinEnbl

    short MKS_COOLANT_MIST_PIN = 21;   // labeled Mist
    short MKS_COOLANT_FLOOD_PIN = 25;  // labeled Flood
    short MKS_PROBE_PIN = 32;          // labeled Probe
} MKS_PINS;

const ArduinoPinConfig MKS_DLC32_CONFIG = {
    .STEPPER_EN_PIN = MKS_PINS.MKS_STEPPERS_DISABLE_PIN,

    .FILL_WATER_DIR_PIN = MKS_PINS.MKS_X_DIRECTION_PIN,
    .FILL_WATER_STEP_PIN = MKS_PINS.MKS_X_STEP_PIN,

    .REAGENT_DIR_PIN = MKS_PINS.MKS_Y_DIRECTION_PIN,
    .REAGENT_STEP_PIN = MKS_PINS.MKS_Y_STEP_PIN,

    .DRAIN_WATER_DIR_PIN = MKS_PINS.MKS_Z_DIRECTION_PIN,
    .DRAIN_WATER_STEP_PIN = MKS_PINS.MKS_Z_STEP_PIN,
};

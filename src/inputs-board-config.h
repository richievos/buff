#pragma once

/*******************************
 * Board Config
 *******************************/
const short UNSET_VALUE = -1;

struct ArduinoPinConfig {
    short STEPPER_DISABLE_PIN;

    short FILL_WATER_DIR_PIN;
    short FILL_WATER_STEP_PIN;

    short REAGENT_DIR_PIN;
    short REAGENT_STEP_PIN;

    short DRAIN_WATER_DIR_PIN;
    short DRAIN_WATER_STEP_PIN;

    short I2C_SDA;
    short I2C_SCL;

    short STIRRER_PIN = UNSET_VALUE;
    short STIRRER_PWM_VALUE = UNSET_VALUE;
};

const ArduinoPinConfig ESP32_CONFIG = {
    .STEPPER_DISABLE_PIN = 2,

    .FILL_WATER_DIR_PIN = 32,
    .FILL_WATER_STEP_PIN = 33,

    .REAGENT_DIR_PIN = 18,
    .REAGENT_STEP_PIN = 19,

    .DRAIN_WATER_DIR_PIN = 16,
    .DRAIN_WATER_STEP_PIN = 17,

    .I2C_SDA = 21,
    .I2C_SCL = 22,
};

/************************************************
 * MKS DLC32
 * Diagrams: https://github.com/makerbase-mks/MKS-DLC32/blob/main/hardware/
 * Source: https://github.com/makerbase-mks/MKS-DLC32-FIRMWARE
 * Pins: https://github.com/makerbase-mks/MKS-DLC32-FIRMWARE/blob/main/Firmware/Grbl_Esp32/src/Machines/i2s_out_xyz_mks_dlc32.h
 * ESP-32S pinout: https://randomnerdtutorials.com/esp32-pinout-reference-gpios/
 ************************************************/
#ifdef BOARD_MKS_DLC32
#include "mks-skinny/I2SOut.h"
#include "mks-skinny/Pins.h"

// from Grbl_Esp32/src/Machines/i2s_out_xyz_mks_dlc32.h
struct mks {
    short MKS_X_DISABLE_PIN = I2SO(0);
    short MKS_X_DIRECTION_PIN = I2SO(2);
    short MKS_X_STEP_PIN = I2SO(1);

    short MKS_Y_DISABLE_PIN = I2SO(0);
    short MKS_Y_DIRECTION_PIN = I2SO(6);
    short MKS_Y_STEP_PIN = I2SO(5);

    short MKS_Z_DISABLE_PIN = I2SO(0);
    short MKS_Z_DIRECTION_PIN = I2SO(4);
    short MKS_Z_STEP_PIN = I2SO(3);

    short MKS_X_LIMIT_PIN = GPIO_NUM_36;
    short MKS_Y_LIMIT_PIN = GPIO_NUM_35;
    short MKS_Z_LIMIT_PIN = GPIO_NUM_34;

    short MKS_STEPPERS_DISABLE_PIN = I2SO(0);

    short MKS_IIC_SCL_PIN = GPIO_NUM_4;
    short MKS_IIC_SDA_PIN = GPIO_NUM_0;

    short MKS_SPINDLE_OUTPUT_PIN = GPIO_NUM_32;
} MKS_PINS;

const ArduinoPinConfig MKS_DLC32_CONFIG = {
    .STEPPER_DISABLE_PIN = MKS_PINS.MKS_STEPPERS_DISABLE_PIN,

    .FILL_WATER_DIR_PIN = MKS_PINS.MKS_X_DIRECTION_PIN,
    .FILL_WATER_STEP_PIN = MKS_PINS.MKS_X_STEP_PIN,

    .REAGENT_DIR_PIN = MKS_PINS.MKS_Y_DIRECTION_PIN,
    .REAGENT_STEP_PIN = MKS_PINS.MKS_Y_STEP_PIN,

    .DRAIN_WATER_DIR_PIN = MKS_PINS.MKS_Z_DIRECTION_PIN,
    .DRAIN_WATER_STEP_PIN = MKS_PINS.MKS_Z_STEP_PIN,

    .I2C_SDA = MKS_PINS.MKS_IIC_SDA_PIN,
    .I2C_SCL = MKS_PINS.MKS_IIC_SCL_PIN,

    .STIRRER_PIN = MKS_PINS.MKS_SPINDLE_OUTPUT_PIN,
    .STIRRER_PWM_VALUE = 50,
};
#endif

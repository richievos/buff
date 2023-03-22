#pragma once

#include <string>

// Buff Libraries
#include "ph.h"
#include "doser-config.h"

/*******************************
 * Basics (don't need to change)
 *******************************/
const int MQTT_BROKER_PORT = 1883;
const float DEFAULT_TRIGGER_OUTPUT_ML = 3.0;

/*******************************
 * Wifi Settings
 * TODO: fill in
 *******************************/
// TODO: Wifi Settings
const std::string wifiSSID = TODO(SSID_HERE);
const std::string wifiPassword = TODO(WIFI_PASSWORD_HERE);

// hostname attempted to be used for wifi dhcp
const std::string hostname = "reefbuff";

/*******************************
 * PH Calibrations
 * TODO: fill in
 *******************************/
// how often to read a new ph value
const uint PH_READ_INTERVAL_MS = 1000;

// I2C address of the PH reading board (from the circuit board's docs)
const uint8_t PH_I2C_ADDRESS = 98;

// calibrate the ph probe and enter in the settings here
const PHCalibrator::CalibrationPoint phLowPoint = { .actualPH = 7.0, .readPH = 7.19 };
const PHCalibrator::CalibrationPoint phHighPoint = { .actualPH = 4.0, .readPH = 5.3 };

PHCalibrator phCalibrator(phLowPoint, phHighPoint);

/*******************************
 * Stepper motor config
 * TODO: fill in
 *******************************/
const StepperConfig stepperConfig = kphm100STB10;

auto fillStepper = std::make_unique<A4988>(stepperConfig.fullStepsForFullRotation, FILL_WATER_DIR_PIN, FILL_WATER_STEP_PIN, MS1_PIN, MS2_PIN, MS3_PIN);

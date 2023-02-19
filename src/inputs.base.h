#pragma once

#include <string>

// My Libraries
#include "ph.h"

/*******************************
 * Basics (don't need to change)
 *******************************/
const int MQTT_BROKER_PORT = 1883;
const float DEFAULT_TRIGGER_OUTPUT_ML = 3.0;

/*******************************
 * Hardware Settings
 * TODO: verify
 *******************************/
// I2C address of the PH reading board (from the circuit board's docs)
const uint8_t PH_I2C_ADDRESS = 98;

/*******************************
 * Wifi Settings
 * TODO: fill in
 *******************************/
// TODO: Wifi Settings
const std::string wifiSSID = TODO(SSID_HERE);
const std::string wifiPassword = TODO(WIFI_PASSWORD_HERE);

// hostname attempted to be used for wifi dhcp
const std::string hostname = "reefalk";

/*******************************
 * PH Calibrations
 * TODO: fill in
 *******************************/
// calibrate the ph probe and enter in the settings here
const PHCalibrator::CalibrationPoint phLowPoint = { .actualPH = 7.0, .readPH = 7.19 };
const PHCalibrator::CalibrationPoint phHighPoint = { .actualPH = 4.0, .readPH = 5.3 };

PHCalibrator phCalibrator(phLowPoint, phHighPoint);

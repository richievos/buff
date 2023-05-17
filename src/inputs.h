#pragma once

#include <map>
#include <string>

// Buff Libraries
#include "doser-config.h"
#include "inputs-board-config.h"
#include "ph-robotank-sensor.h"
#include "ph.h"

// Other inputs
// const std::string wifiSSID;
// const std::string wifiPassword;
#include "../inputs/creds.h"

namespace buff {

namespace inputs {

/*******************************
 * Basics (don't need to change)
 *******************************/
const int MQTT_BROKER_PORT = 1883;
const float DEFAULT_TRIGGER_OUTPUT_ML = 3.0;

#define STRINGIFY(x) #x
#define TOSTRING(x) STRINGIFY(x)
#define BUFF_NAME TOSTRING(OPT_BUFF_NAME)

// hostname attempted to be used for wifi dhcp
const std::string hostname = BUFF_NAME;

/*******************************
 * PH Calibrations
 * TODO: fill in
 *******************************/

// I2C address of the PH reading board (from the circuit board's docs)
const auto roboTankPHSensorI2CAddress = 98l;
defineRoboTankSignalReaderFunc(roboTankPHSensorI2CAddress)

    const ph::PHReadConfig phReadConfig = {
        // how often to read a new ph value
        .readIntervalMS = 1000,

        .phReadFunc = nameForRoboTankSignalReaderFunc(roboTankPHSensorI2CAddress)};

// calibrate the ph probe and enter in the settings here
// apr 25 pre
// const ph::PHCalibrator::CalibrationPoint phHighPoint = {.actualPH = 7.0, .readPH = 7.120};
// const ph::PHCalibrator::CalibrationPoint phLowPoint = {.actualPH = 4.0, .readPH = 5.138};
// apr 25 post
// const ph::PHCalibrator::CalibrationPoint phHighPoint = {.actualPH = 7.0, .readPH = 7.095};
// const ph::PHCalibrator::CalibrationPoint phLowPoint = {.actualPH = 4.0, .readPH = 5.05};

// apr25 probe2, board2
// const ph::PHCalibrator::CalibrationPoint phHighPoint = {.actualPH = 7.0, .readPH = 7.32};
// const ph::PHCalibrator::CalibrationPoint phLowPoint = {.actualPH = 4.0, .readPH = 5.34};

// apr25 probe2, board3
// const ph::PHCalibrator::CalibrationPoint phHighPoint = {.actualPH = 7.0, .readPH = 6.67};
// const ph::PHCalibrator::CalibrationPoint phLowPoint = {.actualPH = 4.0, .readPH = 4.6};

// apr25 probe2, board4
// const ph::PHCalibrator::CalibrationPoint phHighPoint = {.actualPH = 7.0, .readPH = 6.84};
// const ph::PHCalibrator::CalibrationPoint phLowPoint = {.actualPH = 4.0, .readPH = };

// const PHCalibrator::CalibrationPoint phLowPoint = { .actualPH = 4.0, .readPH = 5.244 };
// const ph::PHCalibrator::CalibrationPoint phLowPoint = {.actualPH = 4.0, .readPH = 5.239};

// May3 (after left out to dry)
// const ph::PHCalibrator::CalibrationPoint phHighPoint = {.actualPH = 10.0, .readPH = 9.53};
// const ph::PHCalibrator::CalibrationPoint phHighPoint = {.actualPH = 7.0, .readPH = 7.238};
// const ph::PHCalibrator::CalibrationPoint phLowPoint = {.actualPH = 4.0, .readPH = 5.243};

// May15 new
// const ph::PHCalibrator::CalibrationPoint phHighPoint = {.actualPH = 10.0, .readPH = 9.53};
const ph::PHCalibrator::CalibrationPoint phHighPoint = {.actualPH = 7.0, .readPH = 6.73};
const ph::PHCalibrator::CalibrationPoint phLowPoint = {.actualPH = 4.0, .readPH = 4.7};

// 8.8
//  4old: 4.68

ph::PHCalibrator phCalibrator(phLowPoint, phHighPoint);

/*******************************
 * INPUTS Board config
 *******************************/
#ifdef BOARD_ESP32
const auto PIN_CONFIG = ESP32_CONFIG;
#elif BOARD_MKS_DLC32
const auto PIN_CONFIG = MKS_DLC32_CONFIG;
#endif

// KPHM
// pre-may-3
// const DoserConfig fillDoserConfig = {.mlPerFullRotation = 0.289, .motorRPM = 200, .microStepType = SIXTEENTH, .clockwiseDirectionMultiplier = -1};
// post-may-3
// const DoserConfig fillDoserConfig = {.mlPerFullRotation = 0.286, .motorRPM = 200, .microStepType = SIXTEENTH, .clockwiseDirectionMultiplier = -1};
// new doser temp
// const DoserConfig fillDoserConfig = {.mlPerFullRotation = 0.31, .motorRPM = 200, .microStepType = THIRTY_SECOND, .degreesPerStep = 2.0};
// 3=4.055, 1=1.352
const DoserConfig fillDoserConfig = {.mlPerFullRotation = 0.27, .motorRPM = 120,
                                     //
                                     .microStepType = SIXTEENTH,
                                     .fullStepsPerRotation = 277,
                                     .clockwiseDirectionMultiplier = -1};

// GOSO
// original piping
// DoserConfig reagentDoserConfig = {.mlPerFullRotation = 0.175, .motorRPM = 60, .microStepType = SIXTEENTH, .clockwiseDirectionMultiplier = 1};
// const DoserConfig reagentDoserConfig = {.mlPerFullRotation = 0.181, .motorRPM = 90, .microStepType = SIXTEENTH, .clockwiseDirectionMultiplier = -1};
// const DoserConfig reagentDoserConfig = {.mlPerFullRotation = 0.181, .motorRPM = 90, .microStepType = SIXTEENTH, .clockwiseDirectionMultiplier = -1};
// const DoserConfig reagentDoserConfig = {.mlPerFullRotation = 0.187, .motorRPM = 90, .microStepType = THIRTY_SECOND, .degreesPerStep = 2.0, .clockwiseDirectionMultiplier = -1};
// const DoserConfig reagentDoserConfig = {.mlPerFullRotation = 0.187, .motorRPM = 30, .microStepType = EIGHTH, .degreesPerStep = 2.0};
// 1 = 1.7
const DoserConfig reagentDoserConfig = {
    .mlPerFullRotation = 0.176, .motorRPM = 60,
    //
    .microStepType = SIXTEENTH,
    .fullStepsPerRotation = 274,
    .clockwiseDirectionMultiplier = 1};

// 1-3 piping
// DoserConfig reagentDoserConfig = {.mlPerFullRotation = 0.0501, .motorRPM = 240, .microStepType = SIXTEENTH, .clockwiseDirectionMultiplier = 1};
// KPHM
// 420 rpm was a bit noisy
// DoserConfig drainDoserConfig = {.mlPerFullRotation = 0.29, .motorRPM = 500, .microStepType = EIGHTH, .clockwiseDirectionMultiplier = -1};
// const DoserConfig drainDoserConfig = {.mlPerFullRotation = 0.295, .motorRPM = 400, .microStepType = SIXTEENTH, .clockwiseDirectionMultiplier = -1};
// const DoserConfig drainDoserConfig = {.mlPerFullRotation = 0.295, .motorRPM = 400, .microStepType = SIXTEENTH, .clockwiseDirectionMultiplier = -1};
// const DoserConfig drainDoserConfig = {.mlPerFullRotation = 0.295, .motorRPM = 250, .microStepType = THIRTY_SECOND, .degreesPerStep = 2.0};

// const DoserConfig drainDoserConfig = {.mlPerFullRotation = 0.295, .motorRPM = 60, .microStepType = FULL, .degreesPerStep = 2.0};
const DoserConfig drainDoserConfig = {.mlPerFullRotation = 0.295, .motorRPM = 120,
                                      //
                                      .microStepType = SIXTEENTH,
                                      .fullStepsPerRotation = 293,
                                      .clockwiseDirectionMultiplier = 1};

#ifdef ACCEL_STEPPER_DRIVER
const std::map<MeasurementDoserType, std::shared_ptr<AccelStepper>> doserSteppers = {
    {MeasurementDoserType::FILL, std::make_shared<AccelStepper>(AccelStepper::DRIVER, PIN_CONFIG.FILL_WATER_STEP_PIN, PIN_CONFIG.FILL_WATER_DIR_PIN)},
    {MeasurementDoserType::REAGENT, std::make_shared<AccelStepper>(AccelStepper::DRIVER, PIN_CONFIG.REAGENT_STEP_PIN, PIN_CONFIG.REAGENT_DIR_PIN)},
    {MeasurementDoserType::DRAIN, std::make_shared<AccelStepper>(AccelStepper::DRIVER, PIN_CONFIG.DRAIN_WATER_STEP_PIN, PIN_CONFIG.DRAIN_WATER_DIR_PIN)},
};

std::map<MeasurementDoserType, std::shared_ptr<doser::Doser>> doserInstances = {
    {MeasurementDoserType::FILL, std::make_shared<doser::AccelStepperDoser>(fillDoserConfig, doserSteppers.at(MeasurementDoserType::FILL))},
    {MeasurementDoserType::REAGENT, std::make_shared<doser::AccelStepperDoser>(reagentDoserConfig, doserSteppers.at(MeasurementDoserType::REAGENT))},
    {MeasurementDoserType::DRAIN, std::make_shared<doser::AccelStepperDoser>(drainDoserConfig, doserSteppers.at(MeasurementDoserType::DRAIN))},
};

#else
const std::map<MeasurementDoserType, std::shared_ptr<A4988>> doserSteppers = {
    {MeasurementDoserType::FILL, std::make_shared<A4988>(fillDoserConfig.fullStepsPerRotation, PIN_CONFIG.FILL_WATER_DIR_PIN, PIN_CONFIG.FILL_WATER_STEP_PIN)},
    {MeasurementDoserType::REAGENT, std::make_shared<A4988>(reagentDoserConfig.fullStepsPerRotation, PIN_CONFIG.REAGENT_DIR_PIN, PIN_CONFIG.REAGENT_STEP_PIN)},
    {MeasurementDoserType::DRAIN, std::make_shared<A4988>(drainDoserConfig.fullStepsPerRotation, PIN_CONFIG.DRAIN_WATER_DIR_PIN, PIN_CONFIG.DRAIN_WATER_STEP_PIN)},
    // {MeasurementDoserType::FILL, std::make_shared<A4988>(200, PIN_CONFIG.FILL_WATER_DIR_PIN, PIN_CONFIG.FILL_WATER_STEP_PIN)},
    // {MeasurementDoserType::REAGENT, std::make_shared<A4988>(200, PIN_CONFIG.REAGENT_DIR_PIN, PIN_CONFIG.REAGENT_STEP_PIN)},
    // {MeasurementDoserType::DRAIN, std::make_shared<A4988>(200, PIN_CONFIG.DRAIN_WATER_DIR_PIN, PIN_CONFIG.DRAIN_WATER_STEP_PIN)},
};

std::map<MeasurementDoserType, std::shared_ptr<doser::Doser>> doserInstances = {
    {MeasurementDoserType::FILL, std::make_shared<doser::BasicStepperDoser>(fillDoserConfig, doserSteppers.at(MeasurementDoserType::FILL))},
    {MeasurementDoserType::REAGENT, std::make_shared<doser::BasicStepperDoser>(reagentDoserConfig, doserSteppers.at(MeasurementDoserType::REAGENT))},
    {MeasurementDoserType::DRAIN, std::make_shared<doser::BasicStepperDoser>(drainDoserConfig, doserSteppers.at(MeasurementDoserType::DRAIN))},
};
#endif

alk_measure::AlkMeasurementConfig alkMeasureConf = {
    .primeTankWaterFillVolumeML = 1.0,
    .primeReagentVolumeML = 0.2,

    .measurementTankWaterVolumeML = 200,

    // .measurementTankWaterVolumeML = 200,
    // .extraPurgeVolumeML = 50,

    // .initialReagentDoseVolumeML = 3.0,
    // .incrementalReagentDoseVolumeML = 0.1,

    // .stirAmountML = 3.0,
    // .stirTimes = 10,

    // .reagentStrengthMoles = 0.1,

    // Adjustment for the manual 0.1 HCL mix
    .calibrationMultiplier = 1.0};

}  // namespace inputs
}  // namespace buff

#pragma once

// #include <Arduino.h>

#include <memory>
#include <string>

// Buff Libraries
#include "inputs-board-config.h"

namespace buff {
/*******************************
 * Drivers
 *******************************/
enum DriverStepType {
    FULL = 1,
    QUARTER = 4,
    EIGHTH = 8,
    SIXTEENTH = 16,
    THIRTY_SECOND = 32
};

/*******************************
 * Dosers
 *******************************/
struct DoserConfig {
    float mlPerFullRotation;
    int motorRPM;
    int microStepType;

    // double degreesPerStep = 1.8;
    int fullStepsPerRotation = 200;

    // used to flip the rotation
    // A4988 = 1
    // TMC2208 = -1
    int clockwiseDirectionMultiplier = 1;
};

enum MeasurementDoserType {
    FILL = 0,
    DRAIN = 10,
    REAGENT = 20
};

static std::map<std::string, MeasurementDoserType> const MEASUREMENT_DOSER_TYPE_NAME_TO_MEASUREMENT_DOSER =
    {{"fill", MeasurementDoserType::FILL},
     {"drain", MeasurementDoserType::DRAIN},
     {"reagent", MeasurementDoserType::REAGENT}};

}  // namespace buff

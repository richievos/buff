#pragma once

// #include <Arduino.h>

#include <memory>
#include <string>

// Buff Libraries
#include "inputs-board-config.h"
#include "std-backport.h"

namespace buff {
/*******************************
 * Drivers
 *******************************/
enum DriverStepType {
    FULL = 1,
    QUARTER = 4,
    EIGHTH = 8,
    SIXTEENTH = 16
};

/*******************************
 * Steppers
 *******************************/
struct StepperConfig {
    int fullStepsForFullRotation;
    std::string name;
    float mlPer10Rotations;
};

const StepperConfig generic_1_8degree = {200, "generic_1_8degree", 0.0};

const StepperConfig kphm100STB10 = {200, "kphm100STB10", 2.7};

/*******************************
 * Dosers
 *******************************/
struct DoserConfig {
    float mlPerFullRotation;
    int motorRPM;
    int microStepType;
};
}  // namespace buff

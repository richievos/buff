#pragma once

#include <map>

// Arduino Libraries
#include <A4988.h>

// My Libraries
#include "boardconfig.h"
#include "stdbackport.h"

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
    String name;
    float mlPer10Rotations;
};

const StepperConfig generic_1_8degree = {200, "generic_1_8degree", 0.0};

const StepperConfig kphm100STB10 = {200, "kphm100STB10", 2.7};

/*******************************
 * Doser Helpers
 *******************************/
class Calibrator {
   private:
    const double _mlPerFullRotation;
    const int _fullRotationDegrees;

   public:
    Calibrator(float mlPerFullRotation)
        : _mlPerFullRotation(mlPerFullRotation), _fullRotationDegrees(360) {}

    const int degreesForMLOutput(const float mlOutput) const {
        // want 1.1ml, and each full rotation is 0.2ml, so need 5.5 full rotations
        const double rotations = mlOutput / _mlPerFullRotation;

        // 5.5 rotations * 360 degrees = 1,980 degrees
        return round(rotations * _fullRotationDegrees);
    }

    const double getMlPerFullRotation() {
        return _mlPerFullRotation;
    }
};

struct DoserConfig {
    float mlPerFullRotation;
    int motorRPM;
    int microStepType;
};

struct Doser {
    const std::unique_ptr<A4988> stepper;
    std::unique_ptr<Calibrator> calibrator;
    DoserConfig config;
};

/*******************************
 * INPUTS Board config
 *******************************/
const StepperConfig stepperConfig = kphm100STB10;

auto mainStepper = std::make_unique<A4988>(stepperConfig.fullStepsForFullRotation, DIR_PIN, STEP_PIN, MS1_PIN, MS2_PIN, MS3_PIN);

/*******************************
 * Useful functions
 *******************************/
std::map<std::string, std::unique_ptr<Doser>> nameToDoser;

DoserConfig mainDoserConfig = {.mlPerFullRotation = 0.28, .motorRPM = 60, .microStepType = SIXTEENTH};
Doser mainDoser = {
    .stepper = std::move(mainStepper),
    .calibrator = std::move(std::make_unique<Calibrator>(mainDoserConfig.mlPerFullRotation)),
    .config = mainDoserConfig};


Doser &selectDoser(const std::string doserName) {
    if (nameToDoser.count(doserName)) {
        return *nameToDoser[doserName];
    } else {
        // TODO: should raise if a name given that we don't know
        return *nameToDoser["main"];
    }
}

void setupDosers() {
    nameToDoser.emplace("main", std::unique_ptr<Doser>(&mainDoser));
    // stepper.setSpeedProfile(stepper.LINEAR_SPEED, MOTOR_ACCEL, MOTOR_DECEL);
    mainDoser.stepper->begin(mainDoserConfig.motorRPM, mainDoserConfig.microStepType);
    mainDoser.stepper->enable();
}

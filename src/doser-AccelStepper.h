#pragma once

#include <map>
#include <memory>
#include <string>

// Arduino Libraries
#include <AccelStepper.h>

// Buff Libraries
#include "doser-common.h"

namespace buff {
namespace doser {

class AccelStepperDoser : public Doser {
   public:
    AccelStepperDoser(DoserConfig doserConfig, std::shared_ptr<AccelStepper> s) : Doser(doserConfig), stepper(s) {}

    std::shared_ptr<AccelStepper> stepper;

    virtual void doseML(const float outputML, Calibrator* aCalibrator = nullptr) {
        if (aCalibrator == nullptr) aCalibrator = calibrator.get();

        const long degreesRotation = aCalibrator->degreesForMLOutput(outputML) *
                                     config.clockwiseDirectionMultiplier;
        const long steps = round(config.microStepType * (degreesRotation / config.degreesPerStep));
        Serial.print("Outputting mlToOutput=");
        Serial.print(outputML);
        Serial.print("ml,");
        Serial.print(" via degreesRotation=");
        Serial.print(degreesRotation);
        Serial.print(" via steps=");
        Serial.print(steps);
        Serial.print(" with mlPerFullRotation=");
        Serial.print(aCalibrator->getMlPerFullRotation());
        Serial.print("\n");

        stepper->move(steps);
        stepper->runToPosition();
    }

    virtual void setup() {
        auto maxSpeed = config.motorRPM * config.microStepType;
        Serial.print("Setting max speed, steps=");
        Serial.println(maxSpeed);
        stepper->setMaxSpeed(maxSpeed);
        stepper->setAcceleration(maxSpeed);
    }
};

}  // namespace doser
}  // namespace buff

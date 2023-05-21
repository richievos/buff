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

        const double partialRotation = aCalibrator->partialRotationsForMLOutput(outputML);
        const long steps = partialRotationToSteps(partialRotation);
        Serial.print("[AS] Outputting mlToOutput=");
        Serial.print(outputML);
        Serial.print("ml,");
        Serial.print(" via partialRotation=");
        Serial.print(partialRotation);
        Serial.print(" via steps=");
        Serial.print(steps);
        Serial.print(" with mlPerFullRotation=");
        Serial.print(aCalibrator->getMlPerFullRotation());
        Serial.println();

        stepper->move(steps);
        stepper->runToPosition();
    }

    virtual void setup() {
        const auto rps = config.motorRPM / 60.0;
        const long stepsPerRevolution = partialRotationToSteps(1.0);
        const auto maxSpeed = rps * stepsPerRevolution;
        Serial.print("[AS] Setting max speed, steps=");
        Serial.println(maxSpeed);
        stepper->setMaxSpeed(maxSpeed);
        stepper->setAcceleration(maxSpeed);
        stepper->setMinPulseWidth(80);
    }

    virtual void debugRotateDegrees(const int degreesRotation) {
        const long steps = degreesToFullSteps(degreesRotation);
        Serial.print("[AS] Outputting via degreesRotation=");
        Serial.print(degreesRotation);
        Serial.print(" via steps=");
        Serial.print(steps);
        Serial.println();

        stepper->move(steps);
        stepper->runToPosition();
    }

    virtual void debugRotateSteps(const long steps) {
        Serial.print("[AS] Outputting via steps=");
        Serial.print(steps);
        Serial.println();

        stepper->move(steps);
        stepper->runToPosition();
    }
};

}  // namespace doser
}  // namespace buff

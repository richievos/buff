#pragma once

#include <map>
#include <memory>
#include <string>

// Arduino Libraries
#include <A4988.h>

// Buff Libraries
#include "doser/doser-common.h"

namespace buff {
namespace doser {

class BasicStepperDoser : public Doser {
   public:
    BasicStepperDoser(DoserConfig doserConfig, std::shared_ptr<A4988> s) : Doser(doserConfig), stepper(s) {}

    std::shared_ptr<A4988> stepper;

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
    }

    virtual void setup() {
        stepper->begin(config.motorRPM, config.microStepType);
    }

    virtual void debugRotateDegrees(const int degreesRotation) {
        const long steps = degreesToFullSteps(degreesRotation);
        Serial.print("[BS] Outputting via degreesRotation=");
        Serial.print(degreesRotation);
        Serial.print(" via steps=");
        Serial.print(steps);
        Serial.println();

        stepper->move(steps);
    }

    virtual void debugRotateSteps(const long steps) {
        Serial.print("[BS] Outputting via steps=");
        Serial.print(steps);
        Serial.println();

        stepper->move(steps);
    }
};

}  // namespace doser
}  // namespace buff

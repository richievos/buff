#pragma once

#include <map>
#include <memory>
#include <string>

// Arduino Libraries
#include <A4988.h>

// Buff Libraries
#include "doser-common.h"

namespace buff {
namespace doser {

class BasicStepperDoser : public Doser {
   public:
    BasicStepperDoser(DoserConfig doserConfig, std::shared_ptr<A4988> s) : Doser(doserConfig), stepper(s) {}

    std::shared_ptr<A4988> stepper;

    virtual void doseML(const float outputML, Calibrator* aCalibrator = nullptr) {
        if (aCalibrator == nullptr) aCalibrator = calibrator.get();

        const int degreesRotation = aCalibrator->degreesForMLOutput(outputML) *
                                    config.clockwiseDirectionMultiplier;

        Serial.print("Outputting mlToOutput=");
        Serial.print(outputML);
        Serial.print("ml,");
        Serial.print(" via degreesRotation=");
        Serial.print(degreesRotation);
        Serial.print(" with mlPerFullRotation=");
        Serial.print(aCalibrator->getMlPerFullRotation());
        Serial.print("\n");

        stepper->rotate(degreesRotation);
    }

    virtual void setup() {
        stepper->begin(config.motorRPM, config.microStepType);
    }
};

}  // namespace doser
}  // namespace buff

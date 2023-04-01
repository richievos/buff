#pragma once

#include <map>
#include <memory>
#include <string>

// Arduino Libraries
#include <A4988.h>

// Buff Libraries
#include "doser-config.h"
#include "inputs-board-config.h"
#include "inputs.h"

namespace buff {
namespace doser {
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

class Doser {
   public:
    std::shared_ptr<A4988> stepper;
    std::shared_ptr<Calibrator> calibrator;
    DoserConfig config;

    void doseML(const float outputML, Calibrator* aCalibrator = nullptr) {
        if (aCalibrator == nullptr) aCalibrator = calibrator.get();

        const int degreesRotation = aCalibrator->degreesForMLOutput(outputML);

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
};

class BuffDosers {
   private:
    std::map<MeasurementDoserType, std::unique_ptr<Doser>> _doserTypeToDoser;

   public:
    BuffDosers() {}

    Doser& selectDoser(const MeasurementDoserType doserType) {
        auto it = _doserTypeToDoser.find(doserType);
        if (it != _doserTypeToDoser.end()) {
            return *it->second;
        } else {
            // TODO: should raise if a name given that we don't know
            Serial.print("Did not find doser for doserType=");
            Serial.println(doserType);
            return *_doserTypeToDoser[FILL];
        }
    }

    // TODO: return emplace value
    void emplace(const MeasurementDoserType doserType, std::unique_ptr<Doser> doser) {
        _doserTypeToDoser.emplace(doserType, std::move(doser));
    }
};

MeasurementDoserType lookupMeasurementDoserType(const std::string doserType) {
    auto it = MEASUREMENT_DOSER_TYPE_NAME_TO_MEASUREMENT_DOSER.find(doserType);
    if (it != MEASUREMENT_DOSER_TYPE_NAME_TO_MEASUREMENT_DOSER.end()) {
        return it->second;
    } else {
        // TODO: should raise if a name given that we don't know
        return MeasurementDoserType::FILL;
    }
}

void setupDoser(BuffDosers& buffDosers, const MeasurementDoserType doserType, const DoserConfig& doserConfig, std::shared_ptr<A4988> stepper) {
    auto doser = std::make_unique<Doser>();
    doser->stepper = std::move(stepper);
    doser->calibrator = std::move(std::make_unique<Calibrator>(doserConfig.mlPerFullRotation));
    doser->config = doserConfig;

    doser->stepper->begin(doserConfig.motorRPM, doserConfig.microStepType);
    doser->stepper->enable();
    buffDosers.emplace(doserType, std::move(doser));
}

std::unique_ptr<buff::doser::BuffDosers> setupDosers(const std::map<MeasurementDoserType, DoserConfig> doserConfigs, const std::map<MeasurementDoserType, std::shared_ptr<A4988>> steppers) {
    digitalWrite(STEPPER_RST_PIN, LOW);
    pinMode(STEPPER_RST_PIN, OUTPUT);

    auto dosers = std::make_unique<buff::doser::BuffDosers>();

    for (auto i : doserConfigs) {
        auto doserType = i.first;
        auto doserConfig = i.second;
        setupDoser(*dosers, doserType, doserConfig, std::move(steppers.at(doserType)));
    }

    digitalWrite(STEPPER_RST_PIN, HIGH);

    return std::move(dosers);
}

}  // namespace doser
}  // namespace buff

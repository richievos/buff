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
#include "std-backport.h"

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

enum MeasurementDoserType {
    MAIN = 0,
    DRAIN = 10,
    REAGENT = 20
};

static std::map<std::string, MeasurementDoserType> const MEASUREMENT_DOSER_TYPE_NAME_TO_MEASUREMENT_DOSER =
    {{"main", MeasurementDoserType::MAIN},
     {"drain", MeasurementDoserType::DRAIN},
     {"reagent", MeasurementDoserType::REAGENT}};

class Doser {
   public:
    const std::unique_ptr<A4988> stepper;
    std::unique_ptr<Calibrator> calibrator;
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
            return *_doserTypeToDoser[MAIN];
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
        return MeasurementDoserType::MAIN;
    }
}

void setupDoser(BuffDosers& buffDosers, const buff::doser::MeasurementDoserType doserType, const DoserConfig& doserConfig, std::unique_ptr<A4988> stepper) {
    Doser doser = {
        .stepper = std::move(stepper),
        .calibrator = std::move(std::make_unique<Calibrator>(doserConfig.mlPerFullRotation)),
        .config = doserConfig};

    buffDosers.emplace(doserType, std::move(std::unique_ptr<Doser>(&doser)));
    doser.stepper->begin(doserConfig.motorRPM, doserConfig.microStepType);
    doser.stepper->enable();
}

std::unique_ptr<buff::doser::BuffDosers> setupDosers() {
    auto dosers = std::make_unique<buff::doser::BuffDosers>();
    setupDoser(*dosers, buff::doser::MeasurementDoserType::MAIN, mainDoserConfig, std::move(mainStepper));
    return std::move(dosers);
}

}  // namespace doser
}  // namespace buff

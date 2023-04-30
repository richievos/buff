#pragma once

#include <Arduino.h>

#include <cmath>
#include <map>
#include <memory>
#include <string>

// Buff Libraries
#include "doser-config.h"

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
    DoserConfig config;

    Doser(DoserConfig doserConfig) : config(doserConfig) {}

    std::shared_ptr<Calibrator> calibrator;

    virtual void doseML(const float outputML, Calibrator* aCalibrator = nullptr) = 0;

    virtual void setup() = 0;
};

class BuffDosers {
   private:
    std::map<MeasurementDoserType, std::shared_ptr<Doser>> _doserTypeToDoser;
    const short _doserEnablePin;

   public:
    BuffDosers(short doserEnablePin): _doserEnablePin(doserEnablePin) {}

    std::shared_ptr<Doser> selectDoser(const MeasurementDoserType doserType) {
        auto it = _doserTypeToDoser.find(doserType);
        if (it != _doserTypeToDoser.end()) {
            return it->second;
        } else {
            // TODO: should raise if a name given that we don't know
            Serial.print("Did not find doser for doserType=");
            Serial.println(doserType);
            return _doserTypeToDoser[FILL];
        }
    }

    // TODO: return emplace value
    void emplace(const MeasurementDoserType doserType, std::shared_ptr<Doser> doser) {
        _doserTypeToDoser.emplace(doserType, doser);
    }


    void disableDosers() {
        digitalWrite(_doserEnablePin, HIGH);
    }

    void enableDosers() {
        digitalWrite(_doserEnablePin, LOW);
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

template <class DOSER_TYPE, class STEPPER_TYPE>
void setupDoser(BuffDosers& buffDosers, const MeasurementDoserType doserType, const DoserConfig& doserConfig, std::shared_ptr<STEPPER_TYPE> stepper) {
    auto doser = std::make_unique<DOSER_TYPE>(doserConfig);
    doser->stepper = std::move(stepper);
    doser->calibrator = std::move(std::make_unique<Calibrator>(doserConfig.mlPerFullRotation));
    doser->setup();

    // TODO: if this was a UART based stepper, we could explicitly set the microStepType
    buffDosers.emplace(doserType, std::move(doser));
}

template <class DOSER_TYPE, class STEPPER_TYPE>
std::unique_ptr<buff::doser::BuffDosers> setupDosers(const short doserEnablePin, const std::map<MeasurementDoserType, DoserConfig> doserConfigs, const std::map<MeasurementDoserType, std::shared_ptr<STEPPER_TYPE>> steppers) {
    auto dosers = std::make_unique<buff::doser::BuffDosers>(doserEnablePin);
    pinMode(doserEnablePin, OUTPUT);
    dosers->disableDosers();

    for (auto i : doserConfigs) {
        auto doserType = i.first;
        auto doserConfig = i.second;
        setupDoser<DOSER_TYPE, STEPPER_TYPE>(*dosers, doserType, doserConfig, std::move(steppers.at(doserType)));
    }

    return std::move(dosers);
}

}  // namespace doser
}  // namespace buff

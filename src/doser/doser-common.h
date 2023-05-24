#pragma once

#include <Arduino.h>

#include <cmath>
#include <map>
#include <memory>
#include <string>

// Buff Libraries
#include "doser/doser-config.h"

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
        const double rotations = partialRotationsForMLOutput(mlOutput);

        // 5.5 rotations * 360 degrees = 1,980 degrees
        return round(rotations * _fullRotationDegrees);
    }

    const double partialRotationsForMLOutput(const float mlOutput) const {
        // want 1.1ml, and each full rotation is 0.2ml, so need 5.5 full rotations
        return mlOutput / _mlPerFullRotation;
    }

    const double getMlPerFullRotation() {
        return _mlPerFullRotation;
    }
};

class Doser {
   public:
    const DoserConfig config;

    Doser(DoserConfig doserConfig) : config(doserConfig) {}

    std::shared_ptr<Calibrator> calibrator;

    virtual void doseML(const float outputML, Calibrator* aCalibrator = nullptr) = 0;

    virtual void setup() = 0;

    virtual void debugRotateDegrees(const int deg) = 0;
    virtual void debugRotateSteps(const long steps) = 0;

    long partialRotationToSteps(const double partialRotation) {
        return round(partialRotation * config.fullStepsPerRotation * config.microStepType) * config.clockwiseDirectionMultiplier;
    }

    long degreesToFullSteps(const double degrees) {
        const double fullRotation = 360.0;
        const double partialRotation = (degrees / fullRotation);
        return partialRotationToSteps(partialRotation);
    }
};

class BuffDosers {
   private:
    std::map<MeasurementDoserType, std::shared_ptr<Doser>> _doserTypeToDoser;
    const short _doserDisablePin;

   public:
    BuffDosers(short doserDisablePin) : _doserDisablePin(doserDisablePin) {}

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
        digitalWrite(_doserDisablePin, HIGH);
    }

    void enableDosers() {
        digitalWrite(_doserDisablePin, LOW);
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

template <class STEPPER_TYPE>
void setupDoser(BuffDosers& buffDosers, const MeasurementDoserType &doserType, std::shared_ptr<Doser> doser, std::shared_ptr<STEPPER_TYPE> stepper) {
    doser->calibrator = std::move(std::make_unique<Calibrator>(doser->config.mlPerFullRotation));
    doser->setup();

    // TODO: if this was a UART based stepper, we could explicitly set the microStepType
    buffDosers.emplace(doserType, std::move(doser));
}

template <class STEPPER_TYPE>
std::unique_ptr<buff::doser::BuffDosers> setupDosers(const short doserDisablePin,
                                                             const std::map<MeasurementDoserType, std::shared_ptr<Doser>> &doserInstances,
                                                             const std::map<MeasurementDoserType, std::shared_ptr<STEPPER_TYPE>> &steppers) {
    auto dosers = std::make_unique<buff::doser::BuffDosers>(doserDisablePin);
    pinMode(doserDisablePin, OUTPUT);
    dosers->disableDosers();

    for (auto i : doserInstances) {
        auto doserType = i.first;
        auto doserInstance = i.second;
        setupDoser<STEPPER_TYPE>(*dosers, doserType, doserInstance, std::move(steppers.at(doserType)));
    }

    return std::move(dosers);
}

}  // namespace doser
}  // namespace buff

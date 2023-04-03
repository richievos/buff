#pragma once

// Buff Libraries
#include "ph.h"

namespace buff {
namespace alk_measure {

struct AlkReading {
    unsigned long asOfMS = 0;

    float tankWaterVolumeML = 0.0;
    float reagentVolumeML = 0.0;

    float alkReadingDKH = 0.0;

    ph::PHReading phReading;
};

struct AlkMeasurementConfig {
    float primeTankWaterFillVolumeML = 10;
    float primeReagentVolumeML = 0.5;

    float measurementTankWaterVolumeML = 200;
    float extraPurgeVolumeML = 50;

    float initialReagentDoseVolumeML = 3.0;
    float incrementalReagentDoseVolumeML = 0.1;

    float stirAmountML = 3.0;
    int stirTimes = 10;

    float reagentStrengthMoles = 0.1;
};

}  // namespace alk_measure
}  // namespace buff

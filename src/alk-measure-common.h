#pragma once

// Buff Libraries
#include "ph.h"

namespace buff {
namespace alk_measure {

struct AlkReading {
    unsigned long asOfMS = 0;
    unsigned long asOfAdjustedSec = 0;

    float tankWaterVolumeML = 0.0;
    float reagentVolumeML = 0.0;

    float alkReadingDKH = 0.0;

    ph::PHReading phReading;

    std::string title;
};

struct PersistedAlkReading {
    unsigned long asOfAdjustedSec;
    float alkReadingDKH;
    std::string title;
};

struct AlkMeasurementConfig {
    float primeTankWaterFillVolumeML = 1.0;
    float primeReagentReverseVolumeML = -2.6;
    float primeReagentVolumeML = 2.7;

    float measurementTankWaterVolumeML = 200;
    float extraPurgeVolumeML = 50;

    // 3.0 with 200ml & 0.1M gives 4.2 dkh
    // float initialReagentDoseVolumeML = 3.0;
    // 4.0 with 200ml & 0.1M gives 5.6 dkh
    float initialReagentDoseVolumeML = 4.0;
    // 5.0 with 200ml & 0.1M gives 7.0 dkh
    // float initialReagentDoseVolumeML = 5.0;
    // 11.0 with 200ml & 0.1M gives 15.4 dkh
    float maxReagentDoseML = 11.0;

    float incrementalReagentDoseVolumeML = 0.1;

    float stirAmountML = 1.0;
    int stirTimes = 1;

    float reagentStrengthMoles = 0.1;

    // to adjust the calculated result by a configured value. Is effectively
    // the same as just adjusting the reagentStrengthMoles value
    float calibrationMultiplier = 1.0;
};

}  // namespace alk_measure
}  // namespace buff

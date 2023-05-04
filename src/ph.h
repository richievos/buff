#pragma once

#include <functional>

// Buff Libraries
#include "ph-common.h"

namespace buff {
namespace ph {

using PHReadingFunctionPtr = std::function<float()>;

struct PHReadConfig {
    unsigned int readIntervalMS;
    PHReadingFunctionPtr phReadFunc;
};

class PHCalibrator {
   public:
    struct CalibrationPoint {
        const float actualPH;
        const float readPH;
    };

   private:
    const CalibrationPoint _lowPoint;
    const CalibrationPoint _highPoint;

   public:
    PHCalibrator(CalibrationPoint lowPoint, CalibrationPoint highPoint) : _lowPoint(lowPoint), _highPoint(highPoint){};

    float convert(float reading) const {
        // pHx - the pH of the sample
        // pHref1 - the pH of the calibration buffer 1 (generally 4)
        // pHref2 - the pH of the calibration buffer 2 (generally 7)
        //
        // Mref1 - the measured value for buffer 1
        // Mref2 - the measured value for buffer 2
        //
        // Mx - the measured value for the sample
        // pHx = pHref1 + (pHref2 – pHref1) * (Mx – Mref1) / (Mref2 - Mref1)
        // pHx = 4 + 3 * (Mx – M4) / (M7 - M4)
        return _lowPoint.actualPH +
               (_highPoint.actualPH - _lowPoint.actualPH) *
                   (reading - _lowPoint.readPH) /
                   (_highPoint.readPH - _lowPoint.readPH);
    }
};
}  // namespace ph
}  // namespace buff

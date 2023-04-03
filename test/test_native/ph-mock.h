#pragma once

#include <iterator>
#include <vector>

#include "ph-controller.h"
#include "ph.h"

namespace buff {
const ph::PHCalibrator::CalibrationPoint NoOpHighPoint = {.actualPH = 7.0, .readPH = 7.0};
const ph::PHCalibrator::CalibrationPoint NoOpLowPoint = {.actualPH = 4.0, .readPH = 4.0};
const ph::PHCalibrator NoOpPHCalibrator(NoOpLowPoint, NoOpHighPoint);

struct callable_iter {
    callable_iter(const std::vector<float> &vals) {
        _iter = vals.begin();
    }

    float operator()() {
        auto val = *_iter;
        _iter++;
        return val;
    }

   private:
    std::vector<float>::const_iterator _iter;
};

static std::unique_ptr<ph::controller::PHReader> buildPHReader(std::vector<float> &vals, const ph::PHCalibrator &calibrator = NoOpPHCalibrator) {
    std::function<float()> readerFunc = callable_iter(vals);

    const ph::PHReadConfig phReadConfig = {
        .readIntervalMS = 1000,
        .phReadFunc = readerFunc};

    return std::make_unique<ph::controller::PHReader>(phReadConfig, calibrator);
}

}  // namespace buff

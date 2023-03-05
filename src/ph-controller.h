#pragma once

#include <memory>

#include "ZzzMovingAvg.h"

// Buff Libraries
#include "ph.h"
#include "std-backport.h"

/*******************************
 * Useful functions
 *******************************/
namespace buff {
namespace ph {
namespace controller {

template <size_t NUM_SAMPLES>
class PHReadingStats {
   private:
    ZzzMovingAvg<NUM_SAMPLES, uint, ulong> rawPHStats;
    ZzzMovingAvg<NUM_SAMPLES, uint, ulong> calibPHStats;

    const float phMetricScaleFactor = 10000;

    PHReading _mostRecentReading;

   public:
    PHReading addReading(PHReading reading) {
        _mostRecentReading = reading;

        rawPHStats.add(round(reading.rawPH * phMetricScaleFactor));
        calibPHStats.add(round(reading.calibratedPH * phMetricScaleFactor));

        _mostRecentReading.rawPH_mavg = rawPHStats.get() / phMetricScaleFactor;
        _mostRecentReading.calibratedPH = calibPHStats.get() / phMetricScaleFactor;

        return _mostRecentReading;
    }

    PHReading mostRecentReading() const {
        return _mostRecentReading;
    }
};

class PHReader {
   private:
    const PHCalibrator _phCalibrator;
    const PHReadConfig &_phReadConfig;

    unsigned long nextPHReadTime = 0;

   public:
    PHReader(const PHReadConfig &phReadConfig, const PHCalibrator &phCalibrator) : _phReadConfig(phReadConfig), _phCalibrator(phCalibrator) {}

    PHReading readNewPHSignal(ulong currentMillis = -1) const {
        if (currentMillis = -1) {
            currentMillis = millis();
        }

        const auto ph = (_phReadConfig.phReadFunc)();
        const auto calibratedPH = _phCalibrator.convert(ph);

        PHReading phReading = {.asOfMS = currentMillis, .rawPH = ph, .calibratedPH = calibratedPH};
        return phReading;
    }

    template <size_t NUM_SAMPLES>
    PHReading readNewPHSignalWithStats(PHReadingStats<NUM_SAMPLES> &phReadingStats, ulong currentMillis = -1) const {
        auto phReading = readNewPHSignal(currentMillis);
        return phReadingStats.addReading(phReading);
    }

    template <size_t NUM_SAMPLES>
    std::unique_ptr<PHReading> readNewPHSignalIfTimeAndUpdate(PHReadingStats<NUM_SAMPLES> &phReadingStats) {
        unsigned long currentMillis = millis();

        if (nextPHReadTime > currentMillis) {
            return nullptr;
        }

        auto phReading = readNewPHSignalWithStats<NUM_SAMPLES>(phReadingStats, currentMillis);

        nextPHReadTime = millis() + _phReadConfig.readIntervalMS;

        return std::make_unique<PHReading>(phReadingStats.mostRecentReading());
    }
};

/*******************************
 * Setup & Loop
 *******************************/
void setupPH() {
    setupPH_RoboTankPHBoard();
}
}  // namespace controller
}  // namespace ph
}  // namespace buff

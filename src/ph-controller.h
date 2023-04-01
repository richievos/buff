#pragma once

#include <memory>

#include "ZzzMovingAvg.h"

// Buff Libraries
#include "ph.h"

/*******************************
 * Useful functions
 *******************************/
namespace buff {
namespace ph {
namespace controller {

template <size_t NUM_SAMPLES>
class PHReadingStats {
   private:
    ZzzMovingAvg<NUM_SAMPLES, uint, unsigned long> _rawPHStats;
    ZzzMovingAvg<NUM_SAMPLES, uint, unsigned long> _calibPHStats;

    const float phMetricScaleFactor = 10000;

    PHReading _mostRecentReading;

   public:
    PHReading addReading(PHReading reading) {
        _mostRecentReading = reading;

        _rawPHStats.add(round(reading.rawPH * phMetricScaleFactor));
        _calibPHStats.add(round(reading.calibratedPH * phMetricScaleFactor));

        _mostRecentReading.rawPH_mavg = _rawPHStats.get() / phMetricScaleFactor;
        _mostRecentReading.calibratedPH_mavg = _calibPHStats.get() / phMetricScaleFactor;

        return _mostRecentReading;
    }

    PHReading mostRecentReading() const {
        return _mostRecentReading;
    }

    size_t readingCount() {
        return _rawPHStats.size();
    }

    bool receivedMinReadings() {
        return readingCount() >= NUM_SAMPLES;
    }
};

class PHReader {
   private:
    const PHCalibrator _phCalibrator;
    const PHReadConfig _phReadConfig;

    unsigned long nextPHReadTime = 0;

   public:
    PHReader(const PHReadConfig phReadConfig, const PHCalibrator &phCalibrator) : _phReadConfig(phReadConfig), _phCalibrator(phCalibrator) {}

    PHReading readNewPHSignal(unsigned long currentMillis = -1) const {
        if (currentMillis == -1) {
            currentMillis = millis();
        }

        const auto ph = (_phReadConfig.phReadFunc)();
        const auto calibratedPH = _phCalibrator.convert(ph);

        PHReading phReading = {.asOfMS = currentMillis, .rawPH = ph, .calibratedPH = calibratedPH};
        return phReading;
    }

    template <size_t NUM_SAMPLES>
    PHReading readNewPHSignalWithStats(PHReadingStats<NUM_SAMPLES> &phReadingStats, unsigned long currentMillis = -1) const {
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
// void setupPH() {
// }
}  // namespace controller
}  // namespace ph
}  // namespace buff

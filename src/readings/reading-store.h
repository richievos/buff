#pragma once

#include <functional>
#include <memory>
#include <set>
#include <vector>

#include "readings/alk-measure-common.h"
#include "numeric.h"
#include "readings/ph-common.h"
#include "string-manip.h"

namespace buff {
namespace reading_store {

const size_t MAX_TITLE_LEN = 10;
const size_t READINGS_TO_KEEP = 80;

/************
 * ReadingStore
 ***********/
class ReadingStore {
   private:
    std::vector<alk_measure::PersistedAlkReading> _mostRecentReadings;
    unsigned char _tipIndex = 0;
    ph::PHReading _phReading;
    const size_t _readingsToKeep;

   public:
    ReadingStore(size_t readingsToKeep) : _readingsToKeep(readingsToKeep), _mostRecentReadings(readingsToKeep) {}

    void addPHReading(const ph::PHReading& reading) {
        _phReading = reading;
    };

    const ph::PHReading& getMostRecentPHReading() {
        return _phReading;
    }

    void addAlkReading(const alk_measure::PersistedAlkReading reading, bool persist = false) {
        _mostRecentReadings[_tipIndex] = reading;
        _tipIndex++;
        if (_tipIndex >= _readingsToKeep) {
            _tipIndex = 0;
        }
    };

    const std::vector<alk_measure::PersistedAlkReading>& getReadings() {
        return _mostRecentReadings;
    }

    const std::vector<std::reference_wrapper<alk_measure::PersistedAlkReading>> getReadingsSortedByAsOf() {
        std::vector<std::reference_wrapper<alk_measure::PersistedAlkReading>> sorted{_mostRecentReadings.begin(), _mostRecentReadings.end()};
        std::sort(sorted.begin(), sorted.end(),
                  [](std::reference_wrapper<alk_measure::PersistedAlkReading>& a, std::reference_wrapper<alk_measure::PersistedAlkReading>& b) { return a.get().asOfAdjustedSec > b.get().asOfAdjustedSec; });
        return sorted;
    }

    // TODO: this method taking these params is ugly
    const std::set<std::string> getRecentTitles(const std::vector<std::reference_wrapper<alk_measure::PersistedAlkReading>>& readings) {
        std::set<std::string> values;
        for (auto reading : readings) {
            auto title = reading.get().title;
            richiev::strings::trim(title);
            if (title.size() > 0) {
                values.insert(title);
            }
        }
        return values;
    }

    void updateTipIndex(const unsigned char tipIndex) {
        _tipIndex = tipIndex;
    }

    const unsigned char getTipIndex() { return _tipIndex; }
};

void persistReadingStore(std::shared_ptr<ReadingStore> readingStore);
std::unique_ptr<ReadingStore> setupReadingStore(size_t readingsToKeep);

}  // namespace reading_store
}  // namespace buff

#pragma once

#include <Preferences.h>

#include <functional>
#include <memory>
#include <vector>

#include "alk-measure-common.h"
#include "numeric.h"

namespace buff {
namespace reading_store {

const char* PREFERENCE_NS = "buff";

struct PersistedAlkReading {
    unsigned long asOfMSAdjusted;
    float alkReadingDKH;
    std::string title;
};

/************
 * I/O
 ***********/
Preferences preferences;

const size_t MAX_TITLE_LEN = 10;
const size_t READINGS_TO_KEEP = 30;

// +1 to avoid inserting a null pointer at the beginning of the string
const auto KEY_I_OFFSET = static_cast<unsigned char>(1);
#define DKH_KEY(i) \
    { static_cast<unsigned char>(KEY_I_OFFSET + i), 'D', 0 }
#define AS_OF_KEY(i) \
    { static_cast<unsigned char>(KEY_I_OFFSET + i), 'A', 0 }
#define TITLE_KEY(i) \
    { static_cast<unsigned char>(KEY_I_OFFSET + i), 'T', 0 }
#define INDEX_KEY \
    { 'I', 0 }

void persistAlkReading(const unsigned char i, const PersistedAlkReading& reading) {
    char dkhKey[] = DKH_KEY(i);
    char asOfKey[] = AS_OF_KEY(i);
    char titleKey[] = TITLE_KEY(i);

    preferences.putUChar(dkhKey, numeric::smallFloatToByte(reading.alkReadingDKH));
    preferences.putULong(asOfKey, reading.asOfMSAdjusted);
    if (reading.title.size() >= 0) {
        preferences.putString(titleKey, reading.title.substr(0, MAX_TITLE_LEN).c_str());
    }
}

PersistedAlkReading readAlkReading(const unsigned char i) {
    PersistedAlkReading reading;

    char dkhKey[] = DKH_KEY(i);
    char asOfKey[] = AS_OF_KEY(i);
    char titleKey[] = TITLE_KEY(i);

    reading.alkReadingDKH = numeric::byteToSmallFloat(preferences.getUChar(dkhKey, 0));
    reading.asOfMSAdjusted = preferences.getULong(asOfKey, 0);
    reading.title = preferences.getString(titleKey).c_str();

    return reading;
}

void persistIndex(const unsigned char i) {
    char indexKey[] = INDEX_KEY;
    preferences.putUChar(indexKey, i);
}

unsigned char readIndex() {
    char indexKey[] = INDEX_KEY;
    return preferences.getUChar(indexKey);
}

/************
 * ReadingStore
 ***********/
template <size_t N>
class ReadingStore {
   private:
    std::vector<PersistedAlkReading> _mostRecentReadings;
    unsigned char _tipIndex = 0;

   public:
    ReadingStore() : _mostRecentReadings(N) {}

    void addReading(const PersistedAlkReading reading, bool persist = false) {
        _mostRecentReadings[_tipIndex] = reading;
        _tipIndex++;
        if (_tipIndex >= N) {
            _tipIndex = 0;
        }
    };

    const std::vector<PersistedAlkReading>& getReadings() {
        return _mostRecentReadings;
    }

    const std::vector<std::reference_wrapper<PersistedAlkReading>> getReadingsSortedByAsOf() {
        std::vector<std::reference_wrapper<PersistedAlkReading>> sorted{_mostRecentReadings.begin(), _mostRecentReadings.end()};
        std::sort(sorted.begin(), sorted.end(),
                  [](std::reference_wrapper<PersistedAlkReading>& a, std::reference_wrapper<PersistedAlkReading>& b) { return a.get().asOfMSAdjusted > b.get().asOfMSAdjusted; });
        return sorted;
    }

    void
    updateTipIndex(const unsigned char tipIndex) {
        _tipIndex = tipIndex;
    }

    const unsigned char getTipIndex() { return _tipIndex; }
};

template <size_t N>
void persistReadingStore(std::shared_ptr<ReadingStore<N>> readingStore) {
    preferences.begin(PREFERENCE_NS, false);
    auto& readings = readingStore->getReadings();
    for (unsigned char i = 0; i < readings.size(); i++) {
        persistAlkReading(i, readings[i]);
    }

    persistIndex(readingStore->getTipIndex());
    preferences.end();
}

#include "Arduino.h"
template <size_t N>
std::unique_ptr<ReadingStore<N>> setupReadingStore() {
    auto readingStore = std::make_unique<ReadingStore<N>>();

    preferences.begin(PREFERENCE_NS, true);
    for (unsigned char i = 0; i < N; i++) {
        PersistedAlkReading reading = readAlkReading(i);
        if (reading.alkReadingDKH != 0) {
            readingStore->addReading(reading);
        }
    }

    auto index = readIndex();
    readingStore->updateTipIndex(index);
    preferences.end();

    return std::move(readingStore);
}

}  // namespace reading_store
}  // namespace buff

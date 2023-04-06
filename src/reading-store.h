#pragma once

#include <Preferences.h>

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
};

/************
 * I/O
 ***********/
Preferences preferences;

#define DKH_KEY(i) \
    { i, 'D', 0 }
#define AS_OF_KEY(i) \
    { i, 'A', 0 }
#define INDEX_KEY \
    { 'I', 0 }

void persistAlkReading(const unsigned char i, const PersistedAlkReading& reading) {
    char dkhKey[] = DKH_KEY(i);
    preferences.putUChar(dkhKey, numeric::smallFloatToByte(reading.alkReadingDKH));
    char asOfKey[] = AS_OF_KEY(i);
    preferences.putULong(asOfKey, reading.asOfMSAdjusted);
}

PersistedAlkReading readAlkReading(const unsigned char i) {
    PersistedAlkReading reading;

    char dkhKey[] = DKH_KEY(i);
    reading.alkReadingDKH = numeric::byteToSmallFloat(preferences.getUChar(dkhKey, 0));
    char asOfKey[] = AS_OF_KEY(i);
    reading.asOfMSAdjusted = preferences.getULong(asOfKey, 0);

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

    void updateTipIndex(const unsigned char tipIndex) {
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
        if (reading.asOfMSAdjusted != 0) {
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

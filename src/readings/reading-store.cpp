#include <Arduino.h>
#include <Preferences.h>

#include "readings/reading-store.h"

namespace buff {
namespace reading_store {

const char* PREFERENCE_NS = "buff";

/************
 * I/O
 ***********/
Preferences preferences;

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

void persistAlkReading(const unsigned char i, const alk_measure::PersistedAlkReading& reading) {
    char dkhKey[] = DKH_KEY(i);
    char asOfKey[] = AS_OF_KEY(i);
    char titleKey[] = TITLE_KEY(i);

    preferences.putUChar(dkhKey, numeric::smallFloatToByte(reading.alkReadingDKH));
    preferences.putULong(asOfKey, reading.asOfAdjustedSec);
    if (reading.title.size() >= 0) {
        preferences.putString(titleKey, reading.title.substr(0, MAX_TITLE_LEN).c_str());
    }
}

alk_measure::PersistedAlkReading readAlkReading(const unsigned char i) {
    alk_measure::PersistedAlkReading reading;

    char dkhKey[] = DKH_KEY(i);
    char asOfKey[] = AS_OF_KEY(i);
    char titleKey[] = TITLE_KEY(i);

    reading.alkReadingDKH = numeric::byteToSmallFloat(preferences.getUChar(dkhKey, 0));
    reading.asOfAdjustedSec = preferences.getULong(asOfKey, 0);
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

void persistReadingStore(std::shared_ptr<ReadingStore> readingStore) {
    preferences.begin(PREFERENCE_NS, false);
    auto& readings = readingStore->getReadings();
    for (unsigned char i = 0; i < readings.size(); i++) {
        persistAlkReading(i, readings[i]);
    }

    persistIndex(readingStore->getTipIndex());
    preferences.end();
}

std::unique_ptr<ReadingStore> setupReadingStore(size_t readingsToKeep) {
    auto readingStore = std::make_unique<ReadingStore>(readingsToKeep);

    preferences.begin(PREFERENCE_NS, true);
    for (unsigned char i = 0; i < readingsToKeep; i++) {
        alk_measure::PersistedAlkReading reading = readAlkReading(i);
        if (reading.alkReadingDKH != 0) {
            readingStore->addAlkReading(reading);
        }
    }

    auto index = readIndex();
    readingStore->updateTipIndex(index);
    preferences.end();

    return std::move(readingStore);
}

}  // namespace reading_store
}  // namespace buff

#pragma once

#include <memory>

#include "readings/reading-store.h"

namespace buff {
namespace monitoring_display {

void setupDisplay(std::shared_ptr<reading_store::ReadingStore> readingStore);
void displayPH(const float pH, const float convertedPH, const float rawPH_mvag, const float calibratedPH_mvag, const ulong asOfMS, const unsigned long asOfAdjustedSec);
void loopDisplay();

void updateDisplay(std::shared_ptr<reading_store::ReadingStore> readingStore);

}  // namespace monitoring_display
}  // namespace buff

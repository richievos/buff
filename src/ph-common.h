#pragma once

namespace buff {
namespace ph {

struct PHReading {
    unsigned long asOfMS;
    unsigned long asOfAdjustedSec;

    float rawPH;
    float rawPH_mavg;

    float calibratedPH;
    float calibratedPH_mavg;
};

}  // namespace ph
}  // namespace buff

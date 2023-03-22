#pragma once

namespace buff {
namespace ph {

struct PHReading {
    unsigned long asOfMS;

    float rawPH;
    float rawPH_mavg;

    float calibratedPH;
    float calibratedPH_mavg;
};

}  // namespace ph
}  // namespace buff

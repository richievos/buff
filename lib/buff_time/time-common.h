#pragma once

#include <Arduino.h>

namespace buff {
namespace buff_time {
class TimeWrapper {
    public:
    virtual unsigned long getAdjustedTimeMS() {
        return millis();
    }
};

}
}  // namespace buff

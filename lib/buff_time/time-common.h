#pragma once

#include <Arduino.h>

namespace buff {
namespace buff_time {
class TimeWrapper {
    public:
    virtual unsigned long getAdjustedTimeSeconds() {
        return millis();
    }
};

}
}  // namespace buff

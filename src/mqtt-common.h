#pragma once

#include "readings/alk-measure-common.h"
#include "readings/ph-common.h"

namespace buff {
namespace mqtt {
const std::string alkRead("readings/alk");
const std::string measureAlk("execute/measure_alk");
const std::string phRead("readings/ph");

// TODO: this should live outside mqtt
class Publisher {
   public:
    virtual void publishPH(const ph::PHReading& phReading) = 0;
    virtual void publishAlkReading(const alk_measure::AlkReading& alkReading) = 0;
    virtual void publishMeasureAlk(const std::string& title, const unsigned long asOfMS);

    virtual ~Publisher() {}
};
}  // namespace mqtt
}  // namespace buff

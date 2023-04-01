#pragma once

namespace buff {
namespace mqtt {
const std::string phRead("readings/ph");
const std::string alkRead("readings/alk");

// TODO: this should live outside mqtt
class Publisher {
   public:
    virtual void publishPH(const ph::PHReading& phReading);
    virtual void publishAlkReading(const alk_measure::AlkReading& alkReading);
};
}  // namespace mqtt
}  // namespace buff

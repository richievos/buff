#pragma once

namespace buff {
namespace mqtt {
const std::string phRead("readings/ph");
const std::string alkRead("readings/alk");

// TODO: this should live outside mqtt
class Publisher {
   public:
    virtual void publishPH(const ph::PHReading& phReading) = 0;
    virtual void publishAlkReading(const alk_measure::AlkReading& alkReading) = 0;

    virtual ~Publisher() {}
};
}  // namespace mqtt
}  // namespace buff

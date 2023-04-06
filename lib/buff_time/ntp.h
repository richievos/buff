// based on https://github.com/PaulStoffregen/Time/blob/master/examples/TimeNTP_ESP8266WiFi/TimeNTP_ESP8266WiFi.ino
#pragma once

#include <NTPClient.h>
#include "time-common.h"

#include <memory>

namespace ntp {

WiFiUDP ntpUDP;

std::unique_ptr<NTPClient> setupNTP() {
    auto timeClient = std::make_unique<NTPClient>(ntpUDP);

    Serial.println("Setting up ntp client");
    timeClient->setUpdateInterval(1000 * 60 * 30);
    timeClient->begin();
    timeClient->setTimeOffset(0);

    const int num_tries = 5;
    for (int i = 0; i < num_tries && !timeClient->update(); i++) {
        timeClient->forceUpdate();
        delay(1000);
    }

    Serial.println("Current time: ");
    Serial.println(timeClient->getFormattedTime());

    return std::move(timeClient);
}

void loopNTP(std::shared_ptr<NTPClient> timeClient) {
    // this will trigger a time refresh if needed
    // WARNING: this is sync and blocking
    timeClient->update();
}

class NTPTimeWrapper : public buff::buff_time::TimeWrapper {
   private:
    const std::shared_ptr<NTPClient> _timeClient;

   public:
    NTPTimeWrapper(std::shared_ptr<NTPClient> timeClient): _timeClient(timeClient) {}
    virtual unsigned long getAdjustedTimeMS() {
        return _timeClient->getEpochTime();
    }
};

}  // namespace ntp

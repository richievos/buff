#include <Arduino.h>
#include <ArduinoFake.h>
#include <unity.h>

#include <iterator>
#include <vector>

#include "alk-measure.h"
#include "doser.h"
#include "mqtt-common.h"

using namespace buff;

class TestPublisher : mqtt::Publisher {
    void publishPH(const ph::PHReading& phReading) {
    }

    void publishAlkReading(const alk_measure::AlkReading& alkReading) {
    }
};

void testBeginStartsEmpty() {
    // MqttClient client();
    doser::BuffDosers buffDosers;
    // buff::alk_measure::AlkMeasurer measurer;
}

void runAlkMeasureTests() {
}

// #include "std-backport.h"

// #include <cmath>
// #include <cstring>
#include <functional>
#include <memory>

// Arduino libraries
// #include <PubSubClient.h>
// #define TINY_MQTT_DEBUG 2
// #define TINY_MQTT_ASYNC 1
#include <WiFi.h>

#include "Arduino.h"
#include "ZzzMovingAvg.h"

// Buff Libraries
#include "alk-measure.h"
#include "controller.h"
#include "doser.h"
#include "inputs.h"
#include "monitoring-display.h"
#include "mqtt-publish.h"
#include "mqtt.h"
#include "mywifi.h"
#include "ph-controller.h"

namespace buff {
/*******************************
 * Shared vars
 *******************************/
alk_measure::AlkMeasurementConfig alkMeasureConf = {
    .measurementTankWaterVolumeML = 100,
    .initialReagentDoseVolumeML = 0.5
    // float primeTankWaterFillVolumeML = 10;
    // float primeReagentVolumeML = 0.5;

    // float measurementTankWaterVolumeML = 200;
    // float extraPurgeVolumeML = 50;

    // float initialReagentDoseVolumeML = 3.0;
    // float incrementalReagentDoseVolumeML = 0.1;

    // float stirAmountML = 3.0;
    // int stirTimes = 10;

    // float reagentStrengthMoles = 0.1;
};

std::shared_ptr<alk_measure::AlkMeasurer> alkMeasurer = nullptr;

const size_t STANDARD_PH_MAVG_LENGTH = 30;
auto phReader = std::make_shared<ph::controller::PHReader>(phReadConfig, phCalibrator);
ph::controller::PHReadingStats<STANDARD_PH_MAVG_LENGTH> phReadingStats;

auto mqttBroker = std::make_shared<MqttBroker>(MQTT_BROKER_PORT);
auto mqttClient = std::make_shared<MqttClient>(mqttBroker.get());

auto publisher = std::make_shared<mqtt::MQTTPublisher>(mqttClient);

std::shared_ptr<doser::BuffDosers> buffDosers = nullptr;

/**************************
 * Setup & Loop
 **************************/
void setup() {
    Serial.begin(115200);

    richiev::connectWifi(hostname, wifiSSID, wifiPassword);
    buffDosers = std::move(doser::setupDosers(doserConfigs, doserSteppers));
    // TODO: make this configurable
    setupPH_RoboTankPHBoard();
    alkMeasurer = std::move(alk_measure::alkMeasureSetup(buffDosers, alkMeasureConf, phReader));

    monitoring_display::setupDisplay();

    controller::setupController(mqttBroker, mqttClient, buffDosers, alkMeasurer, publisher);
}

void loop() {
    auto phReadingPtr = phReader->readNewPHSignalIfTimeAndUpdate<STANDARD_PH_MAVG_LENGTH>(phReadingStats);
    if (phReadingPtr != nullptr) {
        publisher->publishPH(*phReadingPtr);
    }

    controller::loopController();

    richiev::mqtt::loopMQTT(mqttBroker, mqttClient);
}

}  // namespace buff

/**************************
 * Setup & Loop
 **************************/
void setup() {
    buff::setup();
}

void loop() {
    buff::loop();
}

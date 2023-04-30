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

// Buff Libraries
#include "alk-measure.h"
#include "controller.h"
#include "doser.h"
#include "inputs.h"
#include "mqtt-publish.h"
#include "mqtt.h"
#include "mywifi.h"
#include "ntp.h"
#include "ota.h"
#include "ph-controller.h"

namespace buff {
/*******************************
 * Shared vars
 *******************************/
alk_measure::AlkMeasurementConfig alkMeasureConf = {
    .primeTankWaterFillVolumeML = 1.0,
    .primeReagentVolumeML = 0.2,

    .measurementTankWaterVolumeML = 200,

    // float measurementTankWaterVolumeML = 200;
    // float extraPurgeVolumeML = 50;

    // float initialReagentDoseVolumeML = 3.0;
    // float incrementalReagentDoseVolumeML = 0.1;

    // float stirAmountML = 3.0;
    // int stirTimes = 10;

    // float reagentStrengthMoles = 0.1;
};

const size_t STANDARD_PH_MAVG_LENGTH = 30;
auto phReader = std::make_shared<ph::controller::PHReader>(inputs::phReadConfig, inputs::phCalibrator);
ph::controller::PHReadingStats<STANDARD_PH_MAVG_LENGTH> phReadingStats;

auto mqttBroker = std::make_shared<MqttBroker>(inputs::MQTT_BROKER_PORT);
auto mqttClient = std::make_shared<MqttClient>(mqttBroker.get());

auto publisher = std::make_shared<mqtt::MQTTPublisher>(mqttClient);

std::shared_ptr<NTPClient> ntpClient;
std::shared_ptr<buff_time::TimeWrapper> timeClient;

std::shared_ptr<doser::BuffDosers> buffDosers = nullptr;

/**************************
 * Setup & Loop
 **************************/
void setup() {
    Serial.begin(115200);

    richiev::connectWifi(inputs::hostname, inputs::wifiSSID, inputs::wifiPassword);
    richiev::ota::setupOTA(inputs::hostname);

    buffDosers = std::move(doser::setupDosers<doser::BasicStepperDoser>(inputs::PIN_CONFIG.STEPPER_EN_PIN, inputs::doserConfigs, inputs::doserSteppers));
    // TODO: make this configurable
    setupPH_RoboTankPHBoard();

    // trigger a NTP refresh
    ntpClient = std::move(ntp::setupNTP());
    timeClient = std::make_shared<ntp::NTPTimeWrapper>(ntpClient);

    controller::setupController(mqttBroker, mqttClient, buffDosers, phReader, alkMeasureConf, publisher, timeClient);
}

void loop() {
    auto phReadingPtr = phReader->readNewPHSignalIfTimeAndUpdate<STANDARD_PH_MAVG_LENGTH>(phReadingStats);
    if (phReadingPtr != nullptr) {
        publisher->publishPH(*phReadingPtr);
    }

    ntp::loopNTP(ntpClient);

    controller::loopController();

    richiev::mqtt::loopMQTT(mqttBroker, mqttClient);
    richiev::ota::loopOTA();
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

// #include "std-backport.h"

// #include <cmath>
// #include <cstring>
#include <functional>
#include <memory>

#include <Arduino.h>
#include <WiFi.h>

#include "mks-skinny/I2SOut.h"

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
    Wire.begin(inputs::PIN_CONFIG.I2C_SDA, inputs::PIN_CONFIG.I2C_SCL);

    richiev::connectWifi(inputs::hostname, inputs::wifiSSID, inputs::wifiPassword);
    richiev::ota::setupOTA(inputs::hostname);

    buffDosers = std::move(doser::setupDosers(inputs::PIN_CONFIG.STEPPER_DISABLE_PIN, inputs::doserInstances, inputs::doserSteppers));
    // TODO: make this configurable
    setupPH_RoboTankPHBoard();

    // trigger a NTP refresh
    ntpClient = std::move(ntp::setupNTP());
    timeClient = std::make_shared<ntp::NTPTimeWrapper>(ntpClient);

    controller::setupController(mqttBroker, mqttClient, buffDosers, phReader, inputs::alkMeasureConf, publisher, timeClient);
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

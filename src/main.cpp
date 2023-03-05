// #include <cmath>
// #include <cstring>
#include <functional>
#include <memory>

// Arduino libraries
// #include <PubSubClient.h>
// #define TINY_MQTT_DEBUG 2
// #define TINY_MQTT_ASYNC 1
#include <Arduino.h>
#include <WiFi.h>

#include "ZzzMovingAvg.h"

// Buff Libraries
#include "doser.h"
#include "inputs.h"
#include "monitoring-display.h"
#include "mqtt.h"
#include "ph-controller.h"

namespace buff {
/*******************************
 * Shared vars
 *******************************/
const size_t STANDARD_PH_MAVG_LENGTH = 30;
ph::controller::PHReader phReader(phReadConfig, phCalibrator);
ph::controller::PHReadingStats<STANDARD_PH_MAVG_LENGTH> phReadingStats;

MqttBroker mqttBroker(MQTT_BROKER_PORT);
MqttClient mqttClient(&mqttBroker);

std::unique_ptr<doser::BuffDosers> buffDosers = nullptr;

/*******************************
 * Wifi
 *******************************/
void setupWifi() {
    /*******************
     * WIFI
     *******************/
    WiFi.config(INADDR_NONE, INADDR_NONE, INADDR_NONE, INADDR_NONE);
    WiFi.setHostname(hostname.c_str());
    WiFi.mode(WIFI_STA);
    WiFi.begin(wifiSSID.c_str(), wifiPassword.c_str());

    Serial << "Connecting to WiFi ..";
    while (WiFi.status() != WL_CONNECTED) {
        Serial.print('.');
        delay(1000);
    }

    Serial.print("Connected ip=");
    Serial.print(WiFi.localIP());
    Serial.println("\n");
}

/**************************
 * Setup & Loop
 **************************/
void setup() {
    Serial.begin(115200);

    setupWifi();
    buffDosers = doser::setupDosers();
    ph::controller::setupPH();

    monitoring_display::setupDisplay();

    /*******************
     * MQTT
     *******************/
    mqtt::mqttSetup(mqttBroker, mqttClient, *buffDosers);
}

void loop() {
    auto phReadingPtr = phReader.readNewPHSignalIfTimeAndUpdate<STANDARD_PH_MAVG_LENGTH>(phReadingStats);
    if (phReadingPtr != nullptr) {
        mqtt::publishPH(mqttClient, *phReadingPtr);
    }

    mqtt::mqttLoop(mqttBroker, mqttClient);
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

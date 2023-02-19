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

// My Libraries
#include "doser.h"
#include "inputs.h"
#include "monitoringdisplay.h"
#include "mqtt.h"
#include "ph.h"
#include "stdbackport.h"

/*******************************
 * MQTT
 *******************************/
MqttBroker mqttBroker(MQTT_BROKER_PORT);
MqttClient mqttClient(&mqttBroker);

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

    Serial << "Connected ip=" << WiFi.localIP() << "\n";
}

/**************************
 * pH Stuff
 **************************/
const uint PH_INTERVAL_MS = 1000;
static unsigned long nextPHReadTime = 0;

void readPHLoop() {
    unsigned long currentMillis = millis();

    if (nextPHReadTime > currentMillis) {
        return;
    }

    const auto pH = getPH(PH_I2C_ADDRESS);
    const auto convertedPH = phCalibrator.convert(pH);

    publishPH(mqttClient, currentMillis, pH, convertedPH);

    nextPHReadTime = millis() + PH_INTERVAL_MS;
}


void setupPH() {
    // PH
    Wire.begin();
    Wire.setClock(10000);  // set I2C bus to 10 KHz - this is important!
}

void setup() {
    Serial.begin(115200);

    setupWifi();
    setupDosers();
    setupPH();

    setupDisplay();

    /*******************
     * MQTT
     *******************/
    mqttSetup(mqttBroker, mqttClient);
}

void loop() {
    readPHLoop();

    mqttLoop(mqttBroker, mqttClient);
}

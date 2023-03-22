#pragma once

// Arduino Libraries
#include <ArduinoJson.h>
#include <TinyMqtt.h>

// Buff Libraries
#include "alk-measure.h"
#include "mqtt-common.h"
#include "ph-common.h"

namespace buff {
namespace mqtt {
/*******************************
 * Handlers
 *******************************/
void publishMessage(MqttClient& mqttClient, const Topic& topic, const DynamicJsonDocument& doc) {
    String serializedDoc;
    serializeJson(doc, serializedDoc);

    mqttClient.publish(topic, serializedDoc);
}

void publishPH(MqttClient& mqttClient, const ph::PHReading& phReading) {
    DynamicJsonDocument updateDoc(1024);

    updateDoc["asOf"] = phReading.asOfMS;
    updateDoc["rawPH"] = phReading.rawPH;
    updateDoc["rawPH_mavg"] = phReading.rawPH_mavg;
    updateDoc["calibratedPH"] = phReading.calibratedPH;
    updateDoc["calibratedPH_mavg"] = phReading.calibratedPH_mavg;

    publishMessage(mqttClient, Topic(phRead), updateDoc);
}

void publishAlkReading(MqttClient& mqttClient, const alk_measure::AlkReading& alkReading) {
    DynamicJsonDocument updateDoc(1024);

    updateDoc["asOf"] = alkReading.asOfMS;
    updateDoc["calibratedPH_mavg"] = alkReading.phReading.calibratedPH_mavg;
    updateDoc["reagentVolumeML"] = alkReading.reagentVolumeML;
    updateDoc["tankWaterVolumeML"] = alkReading.tankWaterVolumeML;

    publishMessage(mqttClient, Topic(phRead), updateDoc);
}

}  // namespace mqtt
}  // namespace buff

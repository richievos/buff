#pragma once

#include <memory>

// Arduino Libraries
#include <ArduinoJson.h>
#include <TinyMqtt.h>

// Buff Libraries
#include "mqtt-common.h"
#include "readings/alk-measure.h"
#include "readings/ph-common.h"

namespace buff {
namespace mqtt {

class MQTTPublisher : public Publisher {
   public:
    MQTTPublisher(std::shared_ptr<MqttClient> mqttClient) : _mqttClient(mqttClient) {}

    virtual void publishMessage(const Topic& topic, const DynamicJsonDocument& doc) {
        String serializedDoc;
        serializeJson(doc, serializedDoc);

        _mqttClient->publish(topic, serializedDoc);
    }

    void publishPH(const ph::PHReading& phReading) {
        DynamicJsonDocument updateDoc(256);

        updateDoc["asOf"] = phReading.asOfMS;
        updateDoc["asOfAdjustedSec"] = phReading.asOfAdjustedSec;
        updateDoc["rawPH"] = phReading.rawPH;
        updateDoc["rawPH_mavg"] = phReading.rawPH_mavg;
        updateDoc["calibratedPH"] = phReading.calibratedPH;
        updateDoc["calibratedPH_mavg"] = phReading.calibratedPH_mavg;

        publishMessage(Topic(phRead), updateDoc);
    }

    void publishAlkReading(const alk_measure::AlkReading& alkReading) {
        DynamicJsonDocument updateDoc(512);

        updateDoc["asOf"] = alkReading.asOfMS;
        updateDoc["asOfAdjustedSec"] = alkReading.asOfAdjustedSec;
        updateDoc["title"] = alkReading.title;
        updateDoc["reagentVolumeML"] = alkReading.reagentVolumeML;
        updateDoc["tankWaterVolumeML"] = alkReading.tankWaterVolumeML;
        updateDoc["alkReadingDKH"] = alkReading.alkReadingDKH;

        updateDoc["calibratedPH_mavg"] = alkReading.phReading.calibratedPH_mavg;

        publishMessage(Topic(alkRead), updateDoc);
    }

    void publishMeasureAlk(const std::string& title, const unsigned long asOfMS) {
        DynamicJsonDocument updateDoc(128);

        updateDoc["asOf"] = asOfMS;
        updateDoc["title"] = title;
        Serial.print("publishMeasureAlk ");
        Serial.println(title.c_str());
        Serial.println();
        return;

        publishMessage(Topic(measureAlk), updateDoc);
    }

   private:
    std::shared_ptr<MqttClient> _mqttClient;
};

}  // namespace mqtt
}  // namespace buff

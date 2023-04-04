#pragma once

#include <memory>

// Arduino Libraries
#include <ArduinoJson.h>
#include <TinyMqtt.h>

// Buff Libraries
#include "alk-measure.h"
#include "mqtt-common.h"
#include "ph-common.h"

namespace buff {
namespace mqtt {

class MQTTPublisher : public Publisher {
   public:
    MQTTPublisher(std::shared_ptr<MqttClient> mqttClient) : _mqttClient(mqttClient) {}

    void publishMessage(const Topic& topic, const DynamicJsonDocument& doc) {
        String serializedDoc;
        serializeJson(doc, serializedDoc);

        _mqttClient->publish(topic, serializedDoc);
    }

    void publishPH(const ph::PHReading& phReading) {
        DynamicJsonDocument updateDoc(1024);

        updateDoc["asOf"] = phReading.asOfMS;
        updateDoc["rawPH"] = phReading.rawPH;
        updateDoc["rawPH_mavg"] = phReading.rawPH_mavg;
        updateDoc["calibratedPH"] = phReading.calibratedPH;
        updateDoc["calibratedPH_mavg"] = phReading.calibratedPH_mavg;

        publishMessage(Topic(phRead), updateDoc);
    }

    void publishAlkReading(const alk_measure::AlkReading& alkReading) {
        DynamicJsonDocument updateDoc(1024);

        updateDoc["asOf"] = alkReading.asOfMS;
        updateDoc["reagentVolumeML"] = alkReading.reagentVolumeML;
        updateDoc["tankWaterVolumeML"] = alkReading.tankWaterVolumeML;
        updateDoc["alkReadingDKH"] = alkReading.alkReadingDKH;

        updateDoc["calibratedPH_mavg"] = alkReading.phReading.calibratedPH_mavg;

        publishMessage(Topic(alkRead), updateDoc);
    }

   private:
    std::shared_ptr<MqttClient> _mqttClient;
};

}  // namespace mqtt
}  // namespace buff

#pragma once

#include <map>
#include <memory>
// Arduino Libraries
#include <ArduinoJson.h>
#include <TinyMqtt.h>

namespace richiev {
namespace mqtt {
/*******************************
 * Handlers
 *******************************/
using TopicProcessorMap = std::map<std::string,
                                   std::function<void(const std::string& payload)>>;
std::shared_ptr<TopicProcessorMap> topicsToProcessor = nullptr;

StaticJsonDocument<200> parseInput(const std::string payload) {
    StaticJsonDocument<200> doc;
    DeserializationError error = deserializeJson(doc, payload);

    if (error) {
        Serial.print(F("deserializeJson() failed: "));
        Serial.println(error.f_str());
    }
    return doc;
}

void onPublish(const MqttClient* /* srce */, const Topic& topic, const char* payloadC, size_t payloadLength) {
    std::string payload = payloadC;
    Serial << "Received msg on topic=" << topic.c_str() << ", payload=" << payload << "\n";

    if (topicsToProcessor->count(topic.c_str())) {
        (*topicsToProcessor)[topic.c_str()](payload);
    } else {
        Serial << "Not handled topic, ignoring" << endl;
    }
}

void setupMQTT(std::shared_ptr<MqttBroker> mqttBroker, std::shared_ptr<MqttClient> mqttClient, const std::shared_ptr<TopicProcessorMap> topProcessor) {
    Serial.print("Starting MQTT broker");
    Serial.print("...");

    mqttBroker->begin();

    Serial.println(" done");

    Serial.print("Starting MQTT client on topic_count=");
    topicsToProcessor = topProcessor;
    Serial.println(topicsToProcessor->size());

    mqttClient->setCallback(onPublish);
    for (const auto& topicAndProcessor : *topicsToProcessor) {
        mqttClient->subscribe(topicAndProcessor.first);
    }
}

void loopMQTT(std::shared_ptr<MqttBroker> mqttBroker, std::shared_ptr<MqttClient> mqttClient) {
    mqttBroker->loop();
    mqttClient->loop();
}
}  // namespace mqtt
}  // namespace richiev

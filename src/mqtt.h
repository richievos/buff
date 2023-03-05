#include <map>
#include <memory>

// Arduino Libraries
#include <ArduinoJson.h>
#include <TinyMqtt.h>

#include "ZzzMovingAvg.h"

// Buff Libraries
#include "alk-measure.h"
#include "doser.h"
#include "inputs.h"
#include "monitoring-display.h"

namespace buff {
namespace mqtt {
/*******************************
 * Handlers
 *******************************/
const std::string phRead("sensors/ph/read");

using TopicProcessorMap = std::map<std::string,
                                   std::function<void(const std::string& payload)>>;
TopicProcessorMap topicsToProcessor;

StaticJsonDocument<200> parseInput(const std::string payload) {
    StaticJsonDocument<200> doc;
    DeserializationError error = deserializeJson(doc, payload);

    if (error) {
        Serial.print(F("deserializeJson() failed: "));
        Serial.println(error.f_str());
    }
    return doc;
}

void debugOutputPH(const float pH, const float convertedPH, const float rawPH_mvag, const float calibratedPH_mvag, const ulong asOf) {
    Serial.print(F("pH="));
    Serial.print(pH, 3);

    Serial.print(F(", pH_mvag="));
    Serial.print(rawPH_mvag, 3);

    Serial.print(F(", calibrated="));
    Serial.print(convertedPH, 3);

    Serial.print(F(", calibrated_mvag="));
    Serial.print(calibratedPH_mvag, 3);

    Serial.print(F(", asOf="));
    Serial.println(asOf);

    monitoring_display::displayPH(pH, convertedPH, rawPH_mvag, calibratedPH_mvag, asOf);
}

doser::Doser& selectDoser(doser::BuffDosers& buffDosers, const StaticJsonDocument<200>& doc) {
    auto measurementDoserType = doser::lookupMeasurementDoserType(doc["doser"].as<std::string>());
    return buffDosers.selectDoser(measurementDoserType);
}

TopicProcessorMap setupHandlers(doser::BuffDosers& buffDosers) {
    topicsToProcessor["debug/triggerML"] = [&](const std::string& payload) {
        auto doc = parseInput(payload);
        auto& doser = selectDoser(buffDosers, doc);

        auto outputML = doc.containsKey("ml") ? doc["ml"].as<float>() : DEFAULT_TRIGGER_OUTPUT_ML;
        if (doc.containsKey("mlPerFullRotation")) {
            doser::Calibrator calibrator(doc["mlPerFullRotation"].as<float>());
            doser.doseML(outputML, &calibrator);
        } else {
            doser.doseML(outputML);
        }
    };

    topicsToProcessor["debug/triggerRotations"] = [&](const std::string& payload) {
        auto doc = parseInput(payload);
        auto& doser = selectDoser(buffDosers, doc);

        const auto degreesRotation = doc["rotations"].as<int>();

        Serial << "Outputting via degreesRotation=" << degreesRotation << endl;
        doser.stepper->rotate(degreesRotation);
    };

    topicsToProcessor["config/mlPerFullRotation"] = [&](const std::string& payload) {
        auto doc = parseInput(payload);
        auto& doser = selectDoser(buffDosers, doc);

        const auto newML = doc["ml"].as<float>();
        Serial << "Switching mlPerFullRotation from=" << doser.calibrator->getMlPerFullRotation()
               << " to=" << newML << endl;
        auto calibr = std::make_unique<doser::Calibrator>(newML);
        doser.calibrator = std::move(calibr);
    };

    topicsToProcessor["config/stepSize"] = [&](const std::string& payload) {
        auto doc = parseInput(payload);
        auto& doser = selectDoser(buffDosers, doc);

        auto stepType = doc["stepType"].as<std::string>();

        int newMicroStepType = FULL;
        if (stepType == "full") {
            newMicroStepType = FULL;
        } else if (stepType == "quarter") {
            newMicroStepType = QUARTER;
        } else if (stepType == "eighth") {
            newMicroStepType = EIGHTH;
        } else if (stepType == "sixteenth") {
            newMicroStepType = SIXTEENTH;
        }

        Serial << "Switching step type from=" << doser.config.microStepType << " to=" << newMicroStepType << "\n";
        doser.config.microStepType = newMicroStepType;
        doser.stepper->begin(doser.config.motorRPM, doser.config.microStepType);
    };

    topicsToProcessor[phRead] = [](const std::string& payload) {
        auto doc = parseInput(payload);

        const auto asOf = doc["asOf"].as<ulong>();
        const auto rawPH = doc["rawPH"].as<float>();
        const auto rawPH_mavg = doc["rawPH_mavg"].as<float>();
        const auto calibratedPH = doc["calibratedPH"].as<float>();
        const auto calibratedPH_mvag = doc["calibratedPH_mavg"].as<float>();

        debugOutputPH(rawPH, calibratedPH, rawPH_mavg, calibratedPH_mvag, asOf);
    };

    Serial << "Initialized topic_processor_count=" << topicsToProcessor.size() << endl;
    return topicsToProcessor;
}

void onPublish(const MqttClient* /* srce */, const Topic& topic, const char* payloadC, size_t payloadLength) {
    std::string payload = payloadC;
    Serial << "Received msg on topic=" << topic.c_str() << ", payload=" << payload << "\n";

    if (topicsToProcessor.count(topic.c_str())) {
        topicsToProcessor[topic.c_str()](payload);
    } else {
        Serial << "Not handled topic, ignoring" << endl;
    }
}

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

void mqttSetup(MqttBroker& mqttBroker, MqttClient& mqttClient, doser::BuffDosers& buffDosers) {
    Serial.print("Starting MQTT broker on port=");
    Serial.print(MQTT_BROKER_PORT);
    Serial.print("...");

    mqttBroker.begin();

    Serial.println(" done");

    Serial.print("Starting MQTT client on topic_count=");
    Serial.println(topicsToProcessor.size());

    mqttClient.setCallback(onPublish);
    auto setupTopicsToProcessor = setupHandlers(buffDosers);
    for (const auto& topicAndProcessor : setupTopicsToProcessor) {
        mqttClient.subscribe(topicAndProcessor.first);
    }
}

void mqttLoop(MqttBroker& mqttBroker, MqttClient& mqttClient) {
    mqttBroker.loop();
    mqttClient.loop();
}
}  // namespace mqtt
}  // namespace buff

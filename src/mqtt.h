#include <map>

// Arduino Libraries
#include <ArduinoJson.h>
#include <TinyMqtt.h>

// My Libraries
#include "doser.h"
#include "inputs.h"

/*******************************
 * Handlers
 *******************************/
const std::string phRead("sensors/ph/read");

typedef std::map<std::string,
                 std::function<void(const std::string& payload)>>
    TopicProcessorMap;
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

void debugOutputPH(const float pH, const float convertedPH, const ulong asOf) {
    Serial.print(F("pH="));
    Serial.print(pH, 3);

    Serial.print(F(", calibrated="));
    Serial.print(convertedPH, 3);

    Serial.print(F(", asOf="));
    Serial.println(asOf);

    displayPH(pH, convertedPH, asOf);
}

TopicProcessorMap setupHandlers() {
    topicsToProcessor["debug/triggerML"] = [](const std::string& payload) {
        auto doc = parseInput(payload);
        auto& doser = selectDoser(doc["doser"].as<std::string>());

        auto outputML = doc.containsKey("ml") ? doc["ml"].as<float>() : DEFAULT_TRIGGER_OUTPUT_ML;
        auto outputCalibrator = doc.containsKey("mlPerFullRotation") ? Calibrator(doc["mlPerFullRotation"].as<float>()) : *doser.calibrator;

        const int degreesRotation = outputCalibrator.degreesForMLOutput(outputML);

        Serial << "Outputting mlToOutput=" << outputML << "ml,"
               << " via degreesRotation=" << degreesRotation
               << " t=" << doc["mlPerFullRotation"].as<float>()
               << " with mlPerFullRotation=" << outputCalibrator.getMlPerFullRotation() << endl;
        doser.stepper->rotate(degreesRotation);
    };

    topicsToProcessor["debug/triggerRotations"] = [](const std::string& payload) {
        auto doc = parseInput(payload);
        auto& doser = selectDoser(doc["doser"].as<std::string>());

        const auto degreesRotation = doc["rotations"].as<int>();

        Serial << "Outputting via degreesRotation=" << degreesRotation << endl;
        doser.stepper->rotate(degreesRotation);
    };

    topicsToProcessor["config/mlPerFullRotation"] = [](const std::string& payload) {
        auto doc = parseInput(payload);
        auto& doser = selectDoser(doc["doser"].as<std::string>());

        const auto newML = doc["ml"].as<float>();
        Serial << "Switching mlPerFullRotation from=" << doser.calibrator->getMlPerFullRotation()
               << " to=" << newML << endl;
        auto calibr = std::make_unique<Calibrator>(newML);
        doser.calibrator = std::move(calibr);
    };

    topicsToProcessor["config/stepSize"] = [](const std::string& payload) {
        auto doc = parseInput(payload);
        auto& doser = selectDoser(doc["doser"].as<std::string>());

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
        const auto calibratedPH = doc["calibratedPH"].as<float>();

        debugOutputPH(rawPH, calibratedPH, asOf);
    };

    Serial << "final size " << topicsToProcessor.size();
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

void publishMessage(MqttClient &mqttClient, const Topic &topic, const DynamicJsonDocument &doc) {
    String serializedDoc;
    serializeJson(doc, serializedDoc);

    mqttClient.publish(topic, serializedDoc);
}

void publishPH(MqttClient &mqttClient, const ulong asOf, const float rawPH, const float calibratedPH) {
    DynamicJsonDocument updateDoc(1024);

    updateDoc["asOf"] = asOf;
    updateDoc["rawPH"] = rawPH;
    updateDoc["calibratedPH"] = calibratedPH;

    publishMessage(mqttClient, Topic(phRead), updateDoc);
}

void mqttSetup(MqttBroker& mqttBroker, MqttClient& mqttClient) {
    Serial << "Starting MQTT broker on port=" << MQTT_BROKER_PORT << "...";
    mqttBroker.begin();
    Serial << " done\n";

    Serial << "Starting MQTT client on topic_count=" << topicsToProcessor.size() << endl;
    mqttClient.setCallback(onPublish);
    auto setupTopicsToProcessor = setupHandlers();
    for (const auto& topicAndProcessor : setupTopicsToProcessor) {
        mqttClient.subscribe(topicAndProcessor.first);
    }
}

void mqttLoop(MqttBroker& mqttBroker, MqttClient& mqttClient) {
    mqttBroker.loop();
    mqttClient.loop();
}
#pragma once

#include <map>
#include <memory>
// Arduino Libraries
#include <ArduinoJson.h>
#include <TinyMqtt.h>

// Buff Libraries
#include "alk-measure.h"
#include "doser.h"
#include "mqtt.h"
// #include "inputs.h"
#include "monitoring-display.h"
#include "mqtt-common.h"

namespace buff {
namespace controller {
/*******************************
 * Handlers
 *******************************/
using TopicProcessorMap = std::map<std::string,
                                   std::function<void(const std::string& payload)>>;
TopicProcessorMap topicsToProcessor;

std::shared_ptr<alk_measure::AlkMeasurer> alkMeasurer = nullptr;
std::shared_ptr<doser::BuffDosers> buffDosersPtr = nullptr;
std::shared_ptr<mqtt::Publisher> publisher = nullptr;
std::unique_ptr<alk_measure::MeasurementStepResult<>> manualStepResult = nullptr;

StaticJsonDocument<200> parseInput(const std::string payload) {
    StaticJsonDocument<200> doc;
    DeserializationError error = deserializeJson(doc, payload);

    if (error) {
        Serial.print(F("deserializeJson() failed: "));
        Serial.println(error.f_str());
    }
    return doc;
}

void debugOutputPH(const StaticJsonDocument<200> doc) {
    const auto asOf = doc["asOf"].as<ulong>();
    const auto rawPH = doc["rawPH"].as<float>();
    const auto rawPH_mavg = doc["rawPH_mavg"].as<float>();
    const auto calibratedPH = doc["calibratedPH"].as<float>();
    const auto calibratedPH_mvag = doc["calibratedPH_mavg"].as<float>();

    monitoring_display::displayPH(rawPH, calibratedPH, rawPH_mavg, calibratedPH_mvag, asOf);
}

void debugOutputAlk(const StaticJsonDocument<200> doc) {
    // Serial.println("Calculated alk ");
    // Serial.print("tankWaterVolumeUsed=");
    // Serial.print(doc["tankWaterVolumeML"].as<float>());
    // Serial.print(", reagentVolumeUsed=");
    // Serial.print(doc["reagentVolumeUsed"].as<float>());
    // Serial.print(", calibratedPH_mavg=");
    // Serial.print(doc["calibratedPH_mavg"].as<float>());
}

doser::Doser& selectDoser(doser::BuffDosers& buffDosers, const StaticJsonDocument<200>& doc) {
    auto doserString = doc["doser"].as<std::string>();
    auto measurementDoserType = doser::lookupMeasurementDoserType(doserString);
    return buffDosers.selectDoser(measurementDoserType);
}

void triggerAlkMeasurement() {
    if (alkMeasurer != nullptr) {
        // alkMeasurer->measureAlk(prevStep);
    }
}

std::unique_ptr<richiev::mqtt::TopicProcessorMap> buildHandlers(doser::BuffDosers& buffDosers) {
    auto topicsToProcessorPtr = std::make_unique<richiev::mqtt::TopicProcessorMap>();
    auto& topicsToProcessor = *topicsToProcessorPtr;

    // std::unique_ptr<richiev::mqtt::TopicProcessorMap> buildHandlers() {
    topicsToProcessor["debug/triggerML"] = [&](const std::string& payload) {
        auto doc = parseInput(payload);
        auto& doser = selectDoser(*buffDosersPtr, doc);

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
        auto& doser = selectDoser(*buffDosersPtr, doc);

        int degreesRotation = 0;
        if (doc.containsKey("rotations")) {
            degreesRotation = round(doc["rotations"].as<float>() * 360);
        } else if (doc.containsKey("degrees")) {
            degreesRotation = doc["degrees"].as<int>();
        }
        Serial << "Outputting via degreesRotation=" << degreesRotation << endl;
        doser.stepper->rotate(degreesRotation);
    };

    topicsToProcessor["execute/measure_alk"] = [&](const std::string& payload) {
        auto doc = parseInput(payload);

        Serial.println("Executing an alk measurement");
    };

    topicsToProcessor["execute/measure_alk/manual/begin"] = [&](const std::string& payload) {
        Serial.println("Preparing to begin a manual alk measurement");
        if (alkMeasurer == nullptr) return;  // TODO: raise

        auto doc = parseInput(payload);

        auto result = alkMeasurer->begin();
        manualStepResult = std::unique_ptr<alk_measure::MeasurementStepResult<>>(new alk_measure::MeasurementStepResult<>(result));

        Serial.print("Alk measurement begin completed, nextAction=");
        Serial.print(manualStepResult->nextAction);
        Serial.print(", nextMeasurementStepAction=");
        Serial.println(manualStepResult->nextMeasurementStepAction);
    };

    topicsToProcessor["execute/measure_alk/manual/next_step"] = [&](const std::string& payload) {
        if (manualStepResult == nullptr) return;  // TODO: raise

        Serial.print("Performing next alk measurement step, nextAction=");
        Serial.print(manualStepResult->nextAction);
        Serial.print(", nextMeasurementStepAction=");
        Serial.println(manualStepResult->nextMeasurementStepAction);

        auto result = alkMeasurer->measureAlk(publisher, *manualStepResult);
        manualStepResult.reset(new alk_measure::MeasurementStepResult<>(result));

        Serial.print("Alk measurement step completed, nextAction=");
        Serial.print(manualStepResult->nextAction);
        Serial.print(", nextMeasurementStepAction=");
        Serial.println(manualStepResult->nextMeasurementStepAction);
    };

    // topicsToProcessor["execute/measure_alk/manual/prime"] = [&](const std::string& payload) {
    //     if (alkMeasurer == nullptr) return;  // TODO: raise

    //     auto doc = parseInput(payload);

    //     Serial.println("Alk Measurement/prime");
    // };

    // topicsToProcessor["execute/measure_alk/manual/clean_fill"] = [&](const std::string& payload) {
    //     if (alkMeasurer == nullptr) return;  // TODO: raise

    //     auto doc = parseInput(payload);

    //     Serial.println("Alk Measurement/clean_fill");
    // };

    topicsToProcessor["config/mlPerFullRotation"] = [&](const std::string& payload) {
        auto doc = parseInput(payload);
        auto& doser = selectDoser(*buffDosersPtr, doc);

        const auto newML = doc["ml"].as<float>();
        Serial << "Switching mlPerFullRotation from=" << doser.calibrator->getMlPerFullRotation()
               << " to=" << newML << endl;
        auto calibr = std::make_unique<doser::Calibrator>(newML);
        doser.calibrator = std::move(calibr);
    };

    topicsToProcessor["config/stepSize"] = [&](const std::string& payload) {
        auto doc = parseInput(payload);
        auto& doser = selectDoser(*buffDosersPtr, doc);

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

    topicsToProcessor[mqtt::phRead] = [](const std::string& payload) {
        auto doc = parseInput(payload);

        debugOutputPH(doc);
    };

    topicsToProcessor[mqtt::alkRead] = [](const std::string& payload) {
        auto doc = parseInput(payload);

        debugOutputAlk(doc);
    };

    Serial << "Initialized topic_processor_count=" << topicsToProcessor.size() << endl;
    return std::move(topicsToProcessorPtr);
}

void setupController(std::shared_ptr<MqttBroker> mqttBroker, std::shared_ptr<MqttClient> mqttClient, std::shared_ptr<doser::BuffDosers> buffDosers, std::shared_ptr<alk_measure::AlkMeasurer> measurer, std::shared_ptr<mqtt::Publisher> pub) {
    std::shared_ptr<richiev::mqtt::TopicProcessorMap> handlers = std::move(buildHandlers(*buffDosers));

    richiev::mqtt::setupMQTT(mqttBroker, mqttClient, handlers);

    alkMeasurer = measurer;
    buffDosersPtr = buffDosers;
    publisher = pub;
}

void loopController() {
    alk_measure::alkMeasureLoop(publisher, alkMeasurer);
}

}  // namespace controller
}  // namespace buff

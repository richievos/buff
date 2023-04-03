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
#include "web-server.h"

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
#define MANUAL_STEP_PH_CHECKS 5
std::unique_ptr<alk_measure::MeasurementStepResult<MANUAL_STEP_PH_CHECKS>> manualStepResult = nullptr;

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

void debugOutputAlk(const StaticJsonDocument<200> doc, const std::string& payload) {
    Serial.println("Calculated alk ");
    Serial.print("alkReadingDKH=");
    Serial.print(doc["alkReadingDKH"].as<float>());
    Serial.print(", ");
    Serial.print(payload.c_str());
    Serial.println();
}

std::shared_ptr<doser::Doser> selectDoser(doser::BuffDosers& buffDosers, const StaticJsonDocument<200>& doc) {
    auto doserString = doc["doser"].as<std::string>();
    auto measurementDoserType = doser::lookupMeasurementDoserType(doserString);
    return buffDosers.selectDoser(measurementDoserType);
}

void triggerAlkMeasurement() {
    if (alkMeasurer != nullptr) {
        // alkMeasurer->measureAlk(prevStep);
    }
}

void nextActionPrint(const alk_measure::MeasurementStepResult<MANUAL_STEP_PH_CHECKS>& stepResult) {
    Serial.print("nextAction=");
    Serial.print(alk_measure::MEASUREMENT_ACTION_TO_NAME.at(stepResult.nextAction).c_str());
    Serial.print("(");
    Serial.print(stepResult.nextAction);
    Serial.print("), nextMeasurementStepAction=");
    Serial.print(alk_measure::MEASUREMENT_STEP_ACTION_TO_NAME.at(stepResult.nextMeasurementStepAction).c_str());
    Serial.print("(");
    Serial.print(stepResult.nextMeasurementStepAction);
    Serial.print("), numPHReadings=");
    if (stepResult.measuredPHStats) {
        Serial.print(stepResult.measuredPHStats->readingCount());
    } else {
        Serial.print(0);
    }
}

std::unique_ptr<richiev::mqtt::TopicProcessorMap> buildHandlers(doser::BuffDosers& buffDosers) {
    auto topicsToProcessorPtr = std::make_unique<richiev::mqtt::TopicProcessorMap>();
    auto& topicsToProcessor = *topicsToProcessorPtr;

    // std::unique_ptr<richiev::mqtt::TopicProcessorMap> buildHandlers() {
    topicsToProcessor["debug/triggerML"] = [&](const std::string& payload) {
        auto doc = parseInput(payload);
        auto doser = selectDoser(*buffDosersPtr, doc);

        auto outputML = doc.containsKey("ml") ? doc["ml"].as<float>() : DEFAULT_TRIGGER_OUTPUT_ML;
        if (doc.containsKey("mlPerFullRotation")) {
            doser::Calibrator calibrator(doc["mlPerFullRotation"].as<float>());
            doser->doseML(outputML, &calibrator);
        } else {
            doser->doseML(outputML);
        }
    };

    topicsToProcessor["debug/triggerRotations"] = [&](const std::string& payload) {
        auto doc = parseInput(payload);
        auto doser = selectDoser(*buffDosersPtr, doc);

        int degreesRotation = 0;
        if (doc.containsKey("rotations")) {
            degreesRotation = round(doc["rotations"].as<float>() * 360);
        } else if (doc.containsKey("degrees")) {
            degreesRotation = doc["degrees"].as<int>();
        }
        Serial << "Outputting via degreesRotation=" << degreesRotation << endl;
        doser->stepper->rotate(degreesRotation);
    };

    topicsToProcessor["execute/measure_alk"] = [&](const std::string& payload) {
        auto doc = parseInput(payload);

        Serial.println("Executing an alk measurement");
    };

    topicsToProcessor["execute/measure_alk/manual/begin"] = [&](const std::string& payload) {
        Serial.println("Preparing to begin a manual alk measurement");
        if (alkMeasurer == nullptr) return;  // TODO: raise

        auto doc = parseInput(payload);

        auto beginAlkMeasureConf = alkMeasurer->getDefaultAlkMeasurementConfig();
        if (doc.containsKey("primeTankWaterFillVolumeML")) {
            beginAlkMeasureConf.primeTankWaterFillVolumeML = doc["primeTankWaterFillVolumeML"].as<float>();
        }
        if (doc.containsKey("primeReagentVolumeML")) {
            beginAlkMeasureConf.primeReagentVolumeML = doc["primeReagentVolumeML"].as<float>();
        }
        if (doc.containsKey("measurementTankWaterVolumeML")) {
            beginAlkMeasureConf.measurementTankWaterVolumeML = doc["measurementTankWaterVolumeML"].as<float>();
        }
        if (doc.containsKey("extraPurgeVolumeML")) {
            beginAlkMeasureConf.extraPurgeVolumeML = doc["extraPurgeVolumeML"].as<float>();
        }
        if (doc.containsKey("initialReagentDoseVolumeML")) {
            beginAlkMeasureConf.initialReagentDoseVolumeML = doc["initialReagentDoseVolumeML"].as<float>();
        }
        if (doc.containsKey("incrementalReagentDoseVolumeML")) {
            beginAlkMeasureConf.incrementalReagentDoseVolumeML = doc["incrementalReagentDoseVolumeML"].as<float>();
        }
        if (doc.containsKey("stirAmountML")) {
            beginAlkMeasureConf.stirAmountML = doc["stirAmountML"].as<float>();
        }
        if (doc.containsKey("stirTimes")) {
            beginAlkMeasureConf.stirTimes = doc["stirTimes"].as<int>();
        }
        if (doc.containsKey("reagentStrengthMoles")) {
            beginAlkMeasureConf.reagentStrengthMoles = doc["reagentStrengthMoles"].as<float>();
        }

        auto result = alkMeasurer->begin<MANUAL_STEP_PH_CHECKS>(beginAlkMeasureConf);
        manualStepResult = std::unique_ptr<alk_measure::MeasurementStepResult<MANUAL_STEP_PH_CHECKS>>(new alk_measure::MeasurementStepResult<MANUAL_STEP_PH_CHECKS>(result));

        Serial.print("Alk measurement begin completed, ");
        nextActionPrint(*manualStepResult);
        Serial.print(", ");
        Serial.print(payload.c_str());
        Serial.println();
    };

    topicsToProcessor["execute/measure_alk/manual/next_step"] = [&](const std::string& payload) {
        if (manualStepResult == nullptr) return;  // TODO: raise

        Serial.print("Performing next alk measurement step, ");
        nextActionPrint(*manualStepResult);
        Serial.println();

        auto result = alkMeasurer->measureAlk(publisher, *manualStepResult);
        manualStepResult.reset(new alk_measure::MeasurementStepResult<MANUAL_STEP_PH_CHECKS>(result));

        Serial.print("Alk measurement step completed, ");
        nextActionPrint(*manualStepResult);
        Serial.println();
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
        auto doser = selectDoser(*buffDosersPtr, doc);

        const auto newML = doc["ml"].as<float>();
        Serial << "Switching mlPerFullRotation from=" << doser->calibrator->getMlPerFullRotation()
               << " to=" << newML << endl;
        auto calibr = std::make_unique<doser::Calibrator>(newML);
        doser->calibrator = std::move(calibr);
    };

    topicsToProcessor["config/stepSize"] = [&](const std::string& payload) {
        auto doc = parseInput(payload);
        auto doser = selectDoser(*buffDosersPtr, doc);

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

        Serial << "Switching step type from=" << doser->config.microStepType << " to=" << newMicroStepType << "\n";
        doser->config.microStepType = newMicroStepType;
        doser->stepper->begin(doser->config.motorRPM, doser->config.microStepType);
    };

    topicsToProcessor[mqtt::phRead] = [](const std::string& payload) {
        auto doc = parseInput(payload);

        debugOutputPH(doc);
    };

    topicsToProcessor[mqtt::alkRead] = [](const std::string& payload) {
        auto doc = parseInput(payload);

        debugOutputAlk(doc, payload);

        // TODO: do this better
        alk_measure::AlkReading alkReading;
        alkReading.asOfMS = doc["asOfMS"].as<unsigned long>();
        alkReading.alkReadingDKH = doc["alkReadingDKH"].as<float>();
        web_server::addReading(alkReading);
    };

    Serial << "Initialized topic_processor_count=" << topicsToProcessor.size() << endl;
    return std::move(topicsToProcessorPtr);
}

void setupController(std::shared_ptr<MqttBroker> mqttBroker, std::shared_ptr<MqttClient> mqttClient, std::shared_ptr<doser::BuffDosers> buffDosers, std::shared_ptr<alk_measure::AlkMeasurer> measurer, std::shared_ptr<mqtt::Publisher> pub) {
    alkMeasurer = measurer;
    buffDosersPtr = buffDosers;
    publisher = pub;

    std::shared_ptr<richiev::mqtt::TopicProcessorMap> handlers = std::move(buildHandlers(*buffDosers));

    richiev::mqtt::setupMQTT(mqttBroker, mqttClient, handlers);
    web_server::setupWebServer();
}

void loopController() {
    alk_measure::alkMeasureLoop(publisher, alkMeasurer);
    web_server::loopWebServer();
}

}  // namespace controller
}  // namespace buff

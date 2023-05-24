#pragma once

#include <map>
#include <memory>
// Arduino Libraries
#include <ArduinoJson.h>
#include <TinyMqtt.h>
#include <nvs_flash.h>

// Buff Libraries
#include "buff-displays/monitoring-display.h"
#include "doser/doser.h"
#include "inputs.h"
#include "readings/alk-measure.h"

#ifdef BOARD_MKS_DLC32
#include "mks-bridge.h"
#endif

#include "mqtt-common.h"
#include "mqtt.h"
#include "readings/reading-store.h"
#include "time-common.h"
#include "web-server.h"

namespace buff {
namespace controller {

const unsigned int AUTO_PH_SAMPLE_COUNT = 15;
const unsigned int MANUAL_PH_SAMPLE_COUNT = 10;
const unsigned int ALK_STEP_INTERVAL_MS = 1000;

/*******************************
 * Handlers
 *******************************/
using TopicProcessorMap = std::map<std::string,
                                   std::function<void(const std::string& payload)>>;
TopicProcessorMap topicsToProcessor;

std::shared_ptr<alk_measure::AlkMeasurer> alkMeasurer = nullptr;
std::shared_ptr<doser::BuffDosers> buffDosersPtr = nullptr;
std::shared_ptr<mqtt::Publisher> publisher = nullptr;
std::shared_ptr<buff_time::TimeWrapper> timeClient = nullptr;

std::unique_ptr<web_server::BuffWebServer> webServer;
std::shared_ptr<reading_store::ReadingStore> readingStore;

std::unique_ptr<alk_measure::AlkMeasureLooper<AUTO_PH_SAMPLE_COUNT>> autoMeasureLooper = nullptr;
std::unique_ptr<alk_measure::AlkMeasureLooper<MANUAL_PH_SAMPLE_COUNT>> manualMeasureLooper = nullptr;

StaticJsonDocument<200> parseInput(const std::string payload) {
    StaticJsonDocument<200> doc;
    DeserializationError error = deserializeJson(doc, payload);

    if (error) {
        Serial.print(F("deserializeJson() failed: "));
        Serial.println(error.f_str());
    }
    return doc;
}

ph::PHReading parsePH(const StaticJsonDocument<200> doc) {
    ph::PHReading reading = {
        .asOfMS = doc["asOf"].as<ulong>(),
        .asOfAdjustedSec = doc["asOfAdjustedSec"].as<ulong>(),

        .rawPH = doc["rawPH"].as<float>(),
        .rawPH_mavg = doc["rawPH_mavg"].as<float>(),

        .calibratedPH = doc["calibratedPH"].as<float>(),
        .calibratedPH_mavg = doc["calibratedPH_mavg"].as<float>()};

    return reading;
}

void debugOutputPH(const ph::PHReading& reading) {
    monitoring_display::displayPH(reading.rawPH, reading.calibratedPH, reading.rawPH_mavg, reading.calibratedPH_mavg, reading.asOfMS, reading.asOfAdjustedSec);
}

void debugOutputAlk(const StaticJsonDocument<200> doc, const std::string& payload) {
    Serial.println("Calculated alk ");
    Serial.print("alkReadingDKH=");
    Serial.print(doc["alkReadingDKH"].as<float>());
    Serial.print(", ");
    Serial.print(payload.c_str());
    Serial.println();
}

template <size_t N>
void debugOutputAction(const alk_measure::MeasurementStepResult<N>& stepResult) {
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
    Serial.print("), calibratedPH_mavg=");
    Serial.print(stepResult.alkReading.phReading.calibratedPH_mavg);
    Serial.print(", reagentVolumeML=");
    Serial.print(stepResult.alkReading.reagentVolumeML);
    Serial.print(", alkReadingDKH=");
    Serial.print(stepResult.alkReading.alkReadingDKH);
}

std::shared_ptr<doser::Doser> selectDoser(doser::BuffDosers& buffDosers, const StaticJsonDocument<200>& doc) {
    auto doserString = doc["doser"].as<std::string>();
    auto measurementDoserType = doser::lookupMeasurementDoserType(doserString);
    return buffDosers.selectDoser(measurementDoserType);
}

unsigned long lastMeasureAsOf = 0;

void runAfterIdempotenceCheck(const unsigned long asOf, std::function<void()> f) {
    if (asOf <= lastMeasureAsOf) {
        Serial.print("Refusing to trigger because of time mismatch (idempotence check). asOf=");
        Serial.print(asOf);
        Serial.print("<= lastMeasureAsOf=");
        Serial.print(lastMeasureAsOf);
        Serial.println();
    } else {
        f();
    }
}

#define LOAD_FROM_DOC(target, name, type)    \
    if (doc.containsKey(#name)) {            \
        target.name = doc[#name].as<type>(); \
    }

alk_measure::AlkMeasurementConfig buildAlkMeasureConfig(const StaticJsonDocument<200>& doc) {
    auto beginAlkMeasureConf = alkMeasurer->getDefaultAlkMeasurementConfig();
    LOAD_FROM_DOC(beginAlkMeasureConf, primeTankWaterFillVolumeML, float);
    LOAD_FROM_DOC(beginAlkMeasureConf, primeReagentReverseVolumeML, float);
    LOAD_FROM_DOC(beginAlkMeasureConf, primeReagentVolumeML, float);
    LOAD_FROM_DOC(beginAlkMeasureConf, measurementTankWaterVolumeML, float);
    LOAD_FROM_DOC(beginAlkMeasureConf, extraPurgeVolumeML, float);
    LOAD_FROM_DOC(beginAlkMeasureConf, initialReagentDoseVolumeML, float);
    LOAD_FROM_DOC(beginAlkMeasureConf, incrementalReagentDoseVolumeML, float);
    LOAD_FROM_DOC(beginAlkMeasureConf, stirAmountML, float);
    LOAD_FROM_DOC(beginAlkMeasureConf, stirTimes, int);
    LOAD_FROM_DOC(beginAlkMeasureConf, reagentStrengthMoles, float);
    if (doc.containsKey("reagentStrengthMoles")) {
        beginAlkMeasureConf.reagentStrengthMoles = doc["reagentStrengthMoles"].as<float>();
    }
    return beginAlkMeasureConf;
}

std::unique_ptr<richiev::mqtt::TopicProcessorMap> buildHandlers(doser::BuffDosers& buffDosers) {
    auto topicsToProcessorPtr = std::make_unique<richiev::mqtt::TopicProcessorMap>();
    auto& topicsToProcessor = *topicsToProcessorPtr;

    topicsToProcessor["debug/restart"] = [&](const std::string& payload) {
        Serial.println("Restarting");
        ESP.restart();
    };

    topicsToProcessor["debug/clear"] = [&](const std::string& payload) {
        Serial.println("Clearing settings out");
        nvs_flash_erase();
        nvs_flash_init();
    };

    topicsToProcessor["debug/dosers/disable"] = [&](const std::string& payload) {
        Serial.println("Disabling doser stepper");
        buffDosersPtr->disableDosers();
    };

    topicsToProcessor["debug/dosers/enable"] = [&](const std::string& payload) {
        Serial.println("Enabling doser stepper");
        buffDosersPtr->enableDosers();
    };

    topicsToProcessor["debug/triggerML"] = [&](const std::string& payload) {
        auto doc = parseInput(payload);
        auto doser = selectDoser(*buffDosersPtr, doc);

        buffDosersPtr->enableDosers();

        auto outputML = doc.containsKey("ml") ? doc["ml"].as<float>() : inputs::DEFAULT_TRIGGER_OUTPUT_ML;
        if (doc.containsKey("mlPerFullRotation")) {
            doser::Calibrator calibrator(doc["mlPerFullRotation"].as<float>());
            doser->doseML(outputML, &calibrator);
        } else {
            doser->doseML(outputML);
        }
    };

    topicsToProcessor["debug/triggerSteps"] = [&](const std::string& payload) {
        auto doc = parseInput(payload);
        auto doser = selectDoser(*buffDosersPtr, doc);

        buffDosersPtr->enableDosers();

        auto steps = doc.containsKey("steps") ? doc["steps"].as<int>() : 200;
        doser->debugRotateSteps(steps);
    };

    topicsToProcessor["debug/stirrer/disable"] = [&](const std::string& payload) {
        analogWrite(inputs::PIN_CONFIG.STIRRER_PIN, 0);
    };

    topicsToProcessor["debug/stirrer/enable"] = [&](const std::string& payload) {
        auto doc = parseInput(payload);
        int value = inputs::PIN_CONFIG.STIRRER_PWM_VALUE;
        if (doc.containsKey("value")) {
            value = doc["value"].as<int>();
        }

        analogWrite(inputs::PIN_CONFIG.STIRRER_PIN, value);
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

        buffDosersPtr->enableDosers();

        Serial << "Outputting via degreesRotation=" << degreesRotation << endl;
        doser->debugRotateDegrees(degreesRotation);
    };

    topicsToProcessor[mqtt::measureAlk] = [&](const std::string& payload) {
        Serial.println("Executing an alk measurement");
        if (alkMeasurer == nullptr) return;        // TODO: raise
        if (autoMeasureLooper != nullptr) return;  // TODO: should this work this way? Should I reset?

        auto doc = parseInput(payload);
        auto beginAlkMeasureConf = buildAlkMeasureConfig(doc);

        auto title = doc["title"].as<std::string>();
        title = title.substr(0, reading_store::MAX_TITLE_LEN);

        auto asOf = millis();
        if (doc.containsKey("asOf")) {
            asOf = doc["asOf"].as<unsigned long>();
        }
        runAfterIdempotenceCheck(asOf, [&]() {
            autoMeasureLooper = std::move(alk_measure::beginAlkMeasureLoop<AUTO_PH_SAMPLE_COUNT>(alkMeasurer, publisher, timeClient, beginAlkMeasureConf, title));
        });
    };

    topicsToProcessor["execute/measure_alk/manual/begin"] = [&](const std::string& payload) {
        Serial.println("Preparing to begin a manual alk measurement");
        if (alkMeasurer == nullptr) return;  // TODO: raise

        auto doc = parseInput(payload);
        auto beginAlkMeasureConf = buildAlkMeasureConfig(doc);

        auto title = doc["title"].as<std::string>();
        title = title.substr(0, reading_store::MAX_TITLE_LEN);

        manualMeasureLooper = std::move(alk_measure::beginAlkMeasureLoop<MANUAL_PH_SAMPLE_COUNT>(alkMeasurer, publisher, timeClient, beginAlkMeasureConf, title));

        Serial.print("Alk measurement begin completed, ");
        debugOutputAction(manualMeasureLooper->getLastStepResult());
        Serial.print(", ");
        Serial.print(payload.c_str());
        Serial.println();
    };

    topicsToProcessor["execute/measure_alk/manual/next_step"] = [&](const std::string& payload) {
        if (manualMeasureLooper == nullptr) return;  // TODO: raise

        Serial.print("Performing next alk measurement step, ");
        debugOutputAction(manualMeasureLooper->getLastStepResult());
        Serial.println();

        auto result = manualMeasureLooper->nextStep();
        Serial.print("Alk measurement step completed, ");
        debugOutputAction(result);
        Serial.println();
    };

    topicsToProcessor["config/mlPerFullRotation"] = [&](const std::string& payload) {
        auto doc = parseInput(payload);
        auto doser = selectDoser(*buffDosersPtr, doc);

        const auto newML = doc["ml"].as<float>();
        Serial << "Switching mlPerFullRotation from=" << doser->calibrator->getMlPerFullRotation()
               << " to=" << newML << endl;
        auto calibr = std::make_unique<doser::Calibrator>(newML);
        doser->calibrator = std::move(calibr);
    };

    topicsToProcessor[mqtt::phRead] = [](const std::string& payload) {
        auto doc = parseInput(payload);

        ph::PHReading reading = parsePH(doc);
        debugOutputPH(reading);
        readingStore->addPHReading(reading);
    };

    topicsToProcessor[mqtt::alkRead] = [](const std::string& payload) {
        auto doc = parseInput(payload);

        debugOutputAlk(doc, payload);

        alk_measure::PersistedAlkReading alkReading;
        LOAD_FROM_DOC(alkReading, asOfAdjustedSec, unsigned long);
        LOAD_FROM_DOC(alkReading, alkReadingDKH, float);
        alkReading.title = doc["title"].as<std::string>();
        readingStore->addAlkReading(alkReading);
        persistReadingStore(readingStore);

        monitoring_display::updateDisplay(readingStore);
    };

    Serial << "Initialized topic_processor_count=" << topicsToProcessor.size() << endl;
    return std::move(topicsToProcessorPtr);
}

std::unique_ptr<alk_measure::AlkMeasurer> alkMeasureSetup(std::shared_ptr<doser::BuffDosers> buffDosers, const alk_measure::AlkMeasurementConfig alkMeasureConf, const std::shared_ptr<ph::controller::PHReader> phReader) {
    return std::make_unique<alk_measure::AlkMeasurer>(buffDosers, alkMeasureConf, phReader);
}

void setupController(std::shared_ptr<MqttBroker> mqttBroker, std::shared_ptr<MqttClient> mqttClient, std::shared_ptr<doser::BuffDosers> buffDosers, std::shared_ptr<ph::controller::PHReader> phReader, const alk_measure::AlkMeasurementConfig& alkMeasureConf, std::shared_ptr<mqtt::Publisher> pub, std::shared_ptr<buff_time::TimeWrapper> t) {
    buffDosersPtr = buffDosers;
    publisher = pub;
    timeClient = t;
    alkMeasurer = std::move(alkMeasureSetup(buffDosers, alkMeasureConf, phReader));

    std::shared_ptr<richiev::mqtt::TopicProcessorMap> handlers = std::move(buildHandlers(*buffDosers));

    readingStore = std::move(reading_store::setupReadingStore(reading_store::READINGS_TO_KEEP));
    webServer = std::make_unique<web_server::BuffWebServer>(timeClient);

    richiev::mqtt::setupMQTT(mqttBroker, mqttClient, handlers);
    webServer->setupWebServer(readingStore);

    monitoring_display::setupDisplay(readingStore, publisher);

#ifdef BOARD_MKS_DLC32
    setup_mks();
#endif
}

void loopAlkMeasurement(unsigned long loopAsOf) {
    if (autoMeasureLooper != nullptr &&
        (autoMeasureLooper->getLastStepResult().asOfMS + ALK_STEP_INTERVAL_MS) <= loopAsOf) {
        Serial.print(loopAsOf);
        Serial.print(" Performing measurement step");
        auto& result = autoMeasureLooper->nextStep();
        Serial.print(loopAsOf);
        Serial.println(" Completed measurement step");
        debugOutputAction(result);
        if (result.nextAction == alk_measure::MeasurementAction::MEASURE_DONE) {
            Serial.println("Completed measurement loop");
            autoMeasureLooper.reset();
        }
    }
}

void loopController() {
    auto pendingRequest = webServer->retrievePendingFeedRequest();
    if (pendingRequest) {
        runAfterIdempotenceCheck(pendingRequest->asOf, [&]() {
            autoMeasureLooper = std::move(alk_measure::beginAlkMeasureLoop<AUTO_PH_SAMPLE_COUNT>(alkMeasurer, publisher, timeClient, alkMeasurer->getDefaultAlkMeasurementConfig(), pendingRequest->title));
        });
    }
    unsigned long currentDurationMS = 0;
    if (autoMeasureLooper) {
        currentDurationMS = autoMeasureLooper->getLastStepResult().asOfMS -
                            autoMeasureLooper->getLastStepResult().measurementStartedAtMS;
    }
    webServer->loopWebServer(currentDurationMS);
    loopAlkMeasurement(millis());

    monitoring_display::loopDisplay();
}

}  // namespace controller
}  // namespace buff

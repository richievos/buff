#pragma once

// Buff Libraries
#include "alk-measure-common.h"
#include "doser.h"
#include "mqtt-publish.h"
#include "ph-controller.h"
#include "ph.h"

namespace buff {
namespace alk_measure {

enum MeasurementAction {
    PRIME,
    CLEAN_FILL,
    MEASURE,
    CLEANUP
};

enum MeasurementStepAction {
    STEP_INITIALIZE,
    MEASURE_PH,
    DOSE,
    DONE
};

void stirForABit(doser::BuffDosers &buffDosers, const AlkMeasurementConfig &alkMeasureConf, AlkReading &alkReading) {
    doser::Doser &drainDoser = buffDosers.selectDoser(doser::MeasurementDoserType::DRAIN);

    // drainDoser.doseML(alkMeasureConf.measurementTankWaterVolumeML + alkMeasureConf.extraPurgeVolumeML);

    // this is kinda hacky, but since I don't currently have a stirrer
    // just stir it by sucking a bit of water out, then push it back in
    // and repeat a couple times. This uses the drain doser because I probably
    // won't have the fill doser line submerged.
    const float extraDrainageML = 1.0;
    for (int i = 0; i < alkMeasureConf.stirTimes; i++) {
        drainDoser.doseML(alkMeasureConf.stirAmountML);
        drainDoser.doseML(-(alkMeasureConf.stirAmountML + extraDrainageML));
    }

    // one more extra drain to make sure it all got out
    // assumes the tube starts empty
    drainDoser.doseML(-alkMeasureConf.stirAmountML);
}

// Pushes a bit of fluid out of the fill dosers, to make sure when we begin
// using them for measurement that we don't miss some initial drops. This
// helps counteract the effects of any back-siphoning.
void primeDosers(doser::BuffDosers &buffDosers, const AlkMeasurementConfig &alkMeasureConf) {
    doser::Doser &waterFillDoser = buffDosers.selectDoser(doser::MeasurementDoserType::FILL);
    doser::Doser &reagentDoser = buffDosers.selectDoser(doser::MeasurementDoserType::REAGENT);

    waterFillDoser.doseML(alkMeasureConf.primeTankWaterFillVolumeML / 2.0);
    reagentDoser.doseML(alkMeasureConf.primeReagentVolumeML);
    waterFillDoser.doseML(alkMeasureConf.primeTankWaterFillVolumeML / 2.0);
}

void drainMeasurementVessel(doser::BuffDosers &buffDosers, const AlkMeasurementConfig &alkMeasureConf) {
    doser::Doser &drainDoser = buffDosers.selectDoser(doser::MeasurementDoserType::DRAIN);

    drainDoser.doseML(alkMeasureConf.measurementTankWaterVolumeML + alkMeasureConf.extraPurgeVolumeML);
}

void fillMeasurementVessel(doser::BuffDosers &buffDosers, const AlkMeasurementConfig &alkMeasureConf, AlkReading &alkReading) {
    doser::Doser &waterFillDoser = buffDosers.selectDoser(doser::MeasurementDoserType::FILL);

    waterFillDoser.doseML(alkMeasureConf.measurementTankWaterVolumeML);
    alkReading.tankWaterVolumeML += alkMeasureConf.measurementTankWaterVolumeML;
}

void addReagentDose(doser::BuffDosers &buffDosers, const float amountML, AlkReading &alkReading) {
    doser::Doser &reagentDoser = buffDosers.selectDoser(doser::MeasurementDoserType::REAGENT);

    reagentDoser.doseML(amountML);
    alkReading.reagentVolumeML += amountML;
}

bool checkIfHitTarget(const float ph) {
    const float phMeasurementEpsilon = 0.05;
    const float targetPH = 4.5;
    const float practicalTargetPH = targetPH + phMeasurementEpsilon;

    return ph > practicalTargetPH;
}

void executeMeasurementLoop(doser::BuffDosers &buffDosers, const AlkMeasurementConfig &alkMeasureConf, AlkReading &alkReading) {
    // Note: per research on the topic (eg https://link.springer.com/chapter/10.1007/978-1-4615-2580-6_14)
    // the stirrer should be stopped before attempting to measure the pH
    addReagentDose(buffDosers, alkMeasureConf.incrementalReagentDoseVolumeML, alkReading);
    stirForABit(buffDosers, alkMeasureConf, alkReading);
}

#define PH_SAMPLE_COUNT 10

template <size_t NUM_SAMPLES = PH_SAMPLE_COUNT>
class MeasurementStepResult {
   public:
    MeasurementAction nextAction;
    MeasurementStepAction nextMeasurementStepAction;

    AlkReading alkReading;
    AlkReading primeAndCleanupScratchData;

    std::shared_ptr<ph::controller::PHReadingStats<NUM_SAMPLES>> measuredPHStats;
};

class AlkMeasurer {
   private:
    MqttClient &_mqttClient;
    doser::BuffDosers &_buffDosers;
    const AlkMeasurementConfig _alkMeasureConf;
    const ph::controller::PHReader &_phReader;

   public:
    AlkMeasurer(MqttClient &mqttClient, doser::BuffDosers &buffDosers, const AlkMeasurementConfig &alkMeasureConf, const ph::controller::PHReader &phReader) : _mqttClient(mqttClient), _buffDosers(buffDosers), _alkMeasureConf(alkMeasureConf), _phReader(phReader) {}

    template <size_t NUM_SAMPLES = PH_SAMPLE_COUNT>
    MeasurementStepResult<NUM_SAMPLES> begin() {
        MeasurementStepResult<NUM_SAMPLES> r;
        r.nextAction = PRIME;
        return r;
    }

    template <size_t NUM_SAMPLES>
    MeasurementStepResult<NUM_SAMPLES> measureAlk(const MeasurementStepResult<NUM_SAMPLES> &prevResult) {
        // TODO: wrap this in a transaction/finally equivalent
        if (prevResult.nextAction == PRIME) {
            MeasurementStepResult<NUM_SAMPLES> r = prevResult;

            // Get everything primed and cleared out
            primeDosers(_buffDosers, _alkMeasureConf);
            drainMeasurementVessel(_buffDosers, _alkMeasureConf);
            fillMeasurementVessel(_buffDosers, _alkMeasureConf, r.primeAndCleanupScratchData);
            stirForABit(_buffDosers, _alkMeasureConf, r.alkReading);

            r.nextAction = CLEAN_FILL;
            return r;
        } else if (prevResult.nextAction == CLEAN_FILL) {
            MeasurementStepResult<NUM_SAMPLES> r = prevResult;

            // Start the measurement
            drainMeasurementVessel(_buffDosers, _alkMeasureConf);
            fillMeasurementVessel(_buffDosers, _alkMeasureConf, r.alkReading);

            addReagentDose(_buffDosers, _alkMeasureConf.initialReagentDoseVolumeML, r.alkReading);
            stirForABit(_buffDosers, _alkMeasureConf, r.alkReading);

            r.primeAndCleanupScratchData.asOfMS = r.alkReading.asOfMS = millis();
            r.nextAction = MEASURE;
            r.nextMeasurementStepAction = STEP_INITIALIZE;
            return r;
        } else if (prevResult.nextAction == MEASURE) {
            const float sleepDurationBetweenSamples = 1000;

            MeasurementStepResult<NUM_SAMPLES> r = prevResult;

            if (prevResult.nextMeasurementStepAction == MeasurementStepAction::STEP_INITIALIZE) {
                r.measuredPHStats = std::make_shared<ph::controller::PHReadingStats<NUM_SAMPLES>>();

                r.primeAndCleanupScratchData.asOfMS = r.alkReading.asOfMS = millis();
                r.nextMeasurementStepAction = MeasurementStepAction::MEASURE_PH;
            } else if (prevResult.nextMeasurementStepAction == MeasurementStepAction::MEASURE_PH) {
                auto initialPHReading = _phReader.readNewPHSignal();
                auto phReading = r.measuredPHStats->addReading(initialPHReading);
                r.alkReading.phReading = phReading;

                Serial.print("[alkmeasure] Gathered ph readings readingCount=");
                Serial.println(r.measuredPHStats->readingCount());
                if (r.measuredPHStats->receivedMinReadings()) {
                    r.nextMeasurementStepAction = MeasurementStepAction::DOSE;
                }
                r.primeAndCleanupScratchData.asOfMS = r.alkReading.asOfMS = millis();
            } else if (prevResult.nextMeasurementStepAction == MeasurementStepAction::DOSE) {
                if (checkIfHitTarget(r.measuredPHStats->mostRecentReading().calibratedPH_mavg)) {
                    r.nextAction = CLEANUP;
                } else {
                    executeMeasurementLoop(_buffDosers, _alkMeasureConf, r.alkReading);
                }

                r.primeAndCleanupScratchData.asOfMS = r.alkReading.asOfMS = millis();
                r.nextMeasurementStepAction = STEP_INITIALIZE;
            } else {
                assert(false);
            }

            r.primeAndCleanupScratchData.asOfMS = r.alkReading.asOfMS = millis();
            return r;
        } else if (prevResult.nextAction == CLEANUP) {
            MeasurementStepResult<NUM_SAMPLES> r = begin();
            
            mqtt::publishAlkReading(_mqttClient, prevResult.alkReading);

            // Clear out all the reagent and refill with fresh tank water
            drainMeasurementVessel(_buffDosers, _alkMeasureConf);
            fillMeasurementVessel(_buffDosers, _alkMeasureConf, r.primeAndCleanupScratchData);
            stirForABit(_buffDosers, _alkMeasureConf, r.alkReading);

            r.primeAndCleanupScratchData.asOfMS = r.alkReading.asOfMS = millis();
            return r;
        }

        assert(false);
    }
};

std::unique_ptr<MeasurementStepResult<PH_SAMPLE_COUNT>> measureStepResult = nullptr;

uint alkStepIntervalMS = 1000;

AlkMeasurer alkMeasureSetup(MqttClient &mqttClient, doser::BuffDosers &buffDosers, const AlkMeasurementConfig &alkMeasureConf, const ph::controller::PHReader &phReader) {
    auto measurer = AlkMeasurer(mqttClient, buffDosers, alkMeasureConf, phReader);
    return measurer;
}

void alkMeasureLoop(AlkMeasurer &alkMeasurer) {
    if (measureStepResult == nullptr) {
        return;
    }

    ulong currentMillis = millis();

    if (measureStepResult->primeAndCleanupScratchData.asOfMS + alkStepIntervalMS > currentMillis) {
        return;
    }

    // auto next = alkMeasurer.measureAlk(*measureStepResult);
    // measureStepResult = std::make_unique<MeasurementStepResult<PH_SAMPLE_COUNT>>(next);
}
}  // namespace alk_measure
}  // namespace buff

#pragma once

// Buff Libraries
#include "alk-measure-common.h"
#include "doser.h"
#include "mqtt-common.h"
#include "ph-controller.h"
#include "ph.h"

namespace buff {
namespace alk_measure {

enum MeasurementAction {
    PRIME,
    CLEAN_AND_FILL,
    MEASURE,
    CLEANUP,
    MEASURE_DONE
};

static const std::map<MeasurementAction, std::string> MEASUREMENT_ACTION_TO_NAME =
    {{PRIME, "PRIME"},
     {CLEAN_AND_FILL, "CLEAN_AND_FILL"},
     {MEASURE, "MEASURE"},
     {CLEANUP, "CLEANUP"},
     {MEASURE_DONE, "MEASURE_DONE"}};

enum MeasurementStepAction {
    STEP_INITIALIZE,
    MEASURE_PH,
    DOSE,
    STEP_DONE
};

static const std::map<MeasurementStepAction, std::string> MEASUREMENT_STEP_ACTION_TO_NAME =
    {{STEP_INITIALIZE, "STEP_INITIALIZE"},
     {MEASURE_PH, "MEASURE_PH"},
     {DOSE, "DOSE"},
     {STEP_DONE, "STEP_DONE"}};

void stirForABit(doser::BuffDosers &buffDosers, const AlkMeasurementConfig &alkMeasureConf) {
    std::shared_ptr<doser::Doser> drainDoser = buffDosers.selectDoser(MeasurementDoserType::DRAIN);

    // drainDoser.doseML(alkMeasureConf.measurementTankWaterVolumeML + alkMeasureConf.extraPurgeVolumeML);

    // this is kinda hacky, but since I don't currently have a stirrer
    // just stir it by sucking a bit of water out, then push it back in
    // and repeat a couple times. This uses the drain doser because I probably
    // won't have the fill doser line submerged.
    const float extraDrainageML = 1.0;
    for (int i = 0; i < alkMeasureConf.stirTimes; i++) {
        drainDoser->doseML(alkMeasureConf.stirAmountML);
        drainDoser->doseML(-(alkMeasureConf.stirAmountML + extraDrainageML));
    }

    // one more extra drain to make sure it all got out
    // assumes the tube starts empty
    drainDoser->doseML(-alkMeasureConf.stirAmountML);
}

// Pushes a bit of fluid out of the fill dosers, to make sure when we begin
// using them for measurement that we don't miss some initial drops. This
// helps counteract the effects of any back-siphoning.
void primeDosers(std::shared_ptr<doser::BuffDosers> buffDosers, const AlkMeasurementConfig &alkMeasureConf) {
    std::shared_ptr<doser::Doser> waterFillDoser = buffDosers->selectDoser(MeasurementDoserType::FILL);
    std::shared_ptr<doser::Doser> reagentDoser = buffDosers->selectDoser(MeasurementDoserType::REAGENT);

    waterFillDoser->doseML(alkMeasureConf.primeTankWaterFillVolumeML / 2.0);
    reagentDoser->doseML(alkMeasureConf.primeReagentVolumeML);
    waterFillDoser->doseML(alkMeasureConf.primeTankWaterFillVolumeML / 2.0);
}

void drainMeasurementVessel(doser::BuffDosers &buffDosers, const AlkMeasurementConfig &alkMeasureConf) {
    std::shared_ptr<doser::Doser> drainDoser = buffDosers.selectDoser(MeasurementDoserType::DRAIN);

    drainDoser->doseML(alkMeasureConf.measurementTankWaterVolumeML + alkMeasureConf.extraPurgeVolumeML);
}

void fillMeasurementVessel(doser::BuffDosers &buffDosers, const AlkMeasurementConfig &alkMeasureConf, AlkReading &alkReading) {
    std::shared_ptr<doser::Doser> waterFillDoser = buffDosers.selectDoser(MeasurementDoserType::FILL);

    waterFillDoser->doseML(alkMeasureConf.measurementTankWaterVolumeML);
    alkReading.tankWaterVolumeML += alkMeasureConf.measurementTankWaterVolumeML;
}

void addReagentDose(doser::BuffDosers &buffDosers, const float amountML, AlkReading &alkReading) {
    std::shared_ptr<doser::Doser> reagentDoser = buffDosers.selectDoser(MeasurementDoserType::REAGENT);

    reagentDoser->doseML(amountML);
    alkReading.reagentVolumeML += amountML;
}

bool checkIfHitTarget(const float ph) {
    const float phMeasurementEpsilon = 0.05;
    const float targetPH = 4.5;
    const float practicalTargetPH = targetPH + phMeasurementEpsilon;

    return ph < practicalTargetPH;
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

    AlkMeasurementConfig alkMeasureConf;
};

class AlkMeasurer {
   private:
    std::shared_ptr<doser::BuffDosers> _buffDosers;
    const AlkMeasurementConfig _defaultAlkMeasurementConf;
    const std::shared_ptr<ph::controller::PHReader> _phReader;

   public:
    AlkMeasurer(std::shared_ptr<doser::BuffDosers> buffDosers, const AlkMeasurementConfig alkMeasureConf, const std::shared_ptr<ph::controller::PHReader> phReader) : _buffDosers(buffDosers), _defaultAlkMeasurementConf(alkMeasureConf), _phReader(phReader) {}

    template <size_t NUM_SAMPLES = PH_SAMPLE_COUNT>
    MeasurementStepResult<NUM_SAMPLES> begin() {
        return begin<NUM_SAMPLES>(_defaultAlkMeasurementConf);
    }

    template <size_t NUM_SAMPLES = PH_SAMPLE_COUNT>
    MeasurementStepResult<NUM_SAMPLES> begin(const AlkMeasurementConfig &alkMeasureConf) {
        MeasurementStepResult<NUM_SAMPLES> r;
        r.nextAction = PRIME;
        r.nextMeasurementStepAction = STEP_INITIALIZE;
        r.alkMeasureConf = alkMeasureConf;
        return r;
    }


    template <size_t NUM_SAMPLES>
    MeasurementStepResult<NUM_SAMPLES> measureAlk(std::shared_ptr<mqtt::Publisher> publisher, const MeasurementStepResult<NUM_SAMPLES> &prevResult) {
        // TODO: wrap this in a transaction/finally equivalent
        if (prevResult.nextAction == PRIME) {
            MeasurementStepResult<NUM_SAMPLES> r = prevResult;

            // Get everything primed and cleared out
            primeDosers(_buffDosers, r.alkMeasureConf);
            drainMeasurementVessel(*_buffDosers, r.alkMeasureConf);
            fillMeasurementVessel(*_buffDosers, r.alkMeasureConf, r.primeAndCleanupScratchData);
            stirForABit(*_buffDosers, r.alkMeasureConf);

            r.nextAction = CLEAN_AND_FILL;

            return r;
        } else if (prevResult.nextAction == CLEAN_AND_FILL) {
            MeasurementStepResult<NUM_SAMPLES> r = prevResult;

            // Start the measurement
            drainMeasurementVessel(*_buffDosers, r.alkMeasureConf);
            fillMeasurementVessel(*_buffDosers, r.alkMeasureConf, r.alkReading);

            addReagentDose(*_buffDosers, r.alkMeasureConf.initialReagentDoseVolumeML, r.alkReading);
            stirForABit(*_buffDosers, r.alkMeasureConf);

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
                auto initialPHReading = _phReader->readNewPHSignal();
                auto phReading = r.measuredPHStats->addReading(initialPHReading);
                r.alkReading.phReading = phReading;

                if (r.measuredPHStats->receivedMinReadings()) {
                    if (checkIfHitTarget(r.measuredPHStats->mostRecentReading().calibratedPH_mavg)) {
                        r.nextAction = CLEANUP;
                        r.nextMeasurementStepAction = STEP_DONE;
                        r.alkReading.alkReadingDKH = (r.alkReading.reagentVolumeML / r.alkReading.tankWaterVolumeML * 280.0) * (r.alkMeasureConf.reagentStrengthMoles / 0.1);
                    } else {
                        r.nextMeasurementStepAction = MeasurementStepAction::DOSE;
                    }
                } else {
                    r.nextMeasurementStepAction = MEASURE_PH;
                }
                r.primeAndCleanupScratchData.asOfMS = r.alkReading.asOfMS = millis();
            } else if (prevResult.nextMeasurementStepAction == MeasurementStepAction::DOSE) {
                // Note: per research on the topic (eg https://link.springer.com/chapter/10.1007/978-1-4615-2580-6_14)
                // the stirrer should be stopped before attempting to measure the pH
                addReagentDose(*_buffDosers, r.alkMeasureConf.incrementalReagentDoseVolumeML, r.alkReading);
                stirForABit(*_buffDosers, r.alkMeasureConf);

                r.primeAndCleanupScratchData.asOfMS = r.alkReading.asOfMS = millis();
                r.nextMeasurementStepAction = STEP_INITIALIZE;
            } else {
                assert(false);
            }

            r.primeAndCleanupScratchData.asOfMS = r.alkReading.asOfMS = millis();
            return r;
        } else if (prevResult.nextAction == CLEANUP) {
            MeasurementStepResult<NUM_SAMPLES> r = prevResult;
            r.nextAction = MEASURE_DONE;

            publisher->publishAlkReading(prevResult.alkReading);

            // Clear out all the reagent and refill with fresh tank water
            drainMeasurementVessel(*_buffDosers, r.alkMeasureConf);
            fillMeasurementVessel(*_buffDosers, r.alkMeasureConf, r.primeAndCleanupScratchData);
            stirForABit(*_buffDosers, r.alkMeasureConf);

            r.primeAndCleanupScratchData.asOfMS = r.alkReading.asOfMS = millis();
            return r;
        } else if (prevResult.nextAction == MEASURE_DONE) {
            return prevResult;
        }

        assert(false);
    }

    AlkMeasurementConfig getDefaultAlkMeasurementConfig() {
        return _defaultAlkMeasurementConf;
    }
};

std::unique_ptr<MeasurementStepResult<PH_SAMPLE_COUNT>> measureStepResult = nullptr;

uint alkStepIntervalMS = 1000;

std::unique_ptr<AlkMeasurer> alkMeasureSetup(std::shared_ptr<doser::BuffDosers> buffDosers, const AlkMeasurementConfig &alkMeasureConf, const std::shared_ptr<ph::controller::PHReader> &phReader) {
    return std::make_unique<AlkMeasurer>(buffDosers, alkMeasureConf, phReader);
}

void alkMeasureLoop(std::shared_ptr<mqtt::Publisher> publisher, std::shared_ptr<AlkMeasurer> alkMeasurer) {
    if (measureStepResult == nullptr) {
        return;
    }

    unsigned long currentMillis = millis();

    if (measureStepResult->primeAndCleanupScratchData.asOfMS + alkStepIntervalMS > currentMillis) {
        return;
    }

    // auto next = alkMeasurer.measureAlk(*measureStepResult);
    // measureStepResult = std::make_unique<MeasurementStepResult<PH_SAMPLE_COUNT>>(next);
}
}  // namespace alk_measure
}  // namespace buff

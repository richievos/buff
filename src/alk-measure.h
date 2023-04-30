#pragma once

#include <Arduino.h>

// Buff Libraries
#include "alk-measure-common.h"
#include "doser.h"
#include "mqtt-common.h"
#include "ph-controller.h"
#include "ph.h"
#include "time-common.h"

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

    // just blow some liquid out to cause some bubbles
    drainDoser->doseML(-alkMeasureConf.stirAmountML);
}
// TODO: this doesn't really work by using the drain. Need to switch to a proper stirrer
void stirForABitInAndOut(doser::BuffDosers &buffDosers, const AlkMeasurementConfig &alkMeasureConf) {
    std::shared_ptr<doser::Doser> drainDoser = buffDosers.selectDoser(MeasurementDoserType::DRAIN);

    // this is kinda hacky, but since I don't currently have a stirrer
    // just stir it by sucking a bit of water out, then push it back in
    // and repeat a couple times. This uses the drain doser because I probably
    // won't have the fill doser line submerged.
    // The extraDrainageML also causes bubbles in the container, which by
    // themselves move water around.
    const float extraDrainageML = 2.0;
    for (int i = 0; i < alkMeasureConf.stirTimes; i++) {
        drainDoser->doseML(alkMeasureConf.stirAmountML);
        drainDoser->doseML(-(alkMeasureConf.stirAmountML + extraDrainageML));
    }
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

bool hitPHTarget(const float ph) {
    const float phMeasurementEpsilon = 0.05;
    const float targetPH = 4.5;
    const float practicalTargetPH = targetPH + phMeasurementEpsilon;

    return ph < practicalTargetPH;
}

float round2Decimals(const float f) {
    return roundf(f*100.0) / 100.0;
}
float calcAlkReading(const AlkReading& alkReading, const AlkMeasurementConfig &alkMeasureConf) {
    float dkh = (alkReading.reagentVolumeML / alkReading.tankWaterVolumeML * 280.0) * (alkMeasureConf.reagentStrengthMoles / 0.1);

    return round2Decimals(dkh);
}

template <size_t NUM_SAMPLES>
class MeasurementStepResult {
   public:
    unsigned long measurementStartedAtMS;

    unsigned long asOfMS;
    unsigned long asOfAdjustedSec;
    MeasurementAction nextAction;
    MeasurementStepAction nextMeasurementStepAction;

    AlkReading alkReading;
    AlkReading primeAndCleanupScratchData;

    std::shared_ptr<ph::controller::PHReadingStats<NUM_SAMPLES>> measuredPHStats;

    AlkMeasurementConfig alkMeasureConf;

    void setTime(const unsigned long asOf, const unsigned long asOfAdjustedSec) {
        this->asOfMS = alkReading.asOfMS = primeAndCleanupScratchData.asOfMS = asOf;
        this->asOfAdjustedSec = alkReading.asOfAdjustedSec = primeAndCleanupScratchData.asOfAdjustedSec = asOfAdjustedSec;
    }
};

class AlkMeasurer {
   private:
    std::shared_ptr<doser::BuffDosers> _buffDosers;
    const AlkMeasurementConfig _defaultAlkMeasurementConf;
    const std::shared_ptr<ph::controller::PHReader> _phReader;

   public:
    AlkMeasurer(std::shared_ptr<doser::BuffDosers> buffDosers, const AlkMeasurementConfig alkMeasureConf, const std::shared_ptr<ph::controller::PHReader> phReader) : _buffDosers(buffDosers), _defaultAlkMeasurementConf(alkMeasureConf), _phReader(phReader) {}

    template <size_t NUM_SAMPLES>
    MeasurementStepResult<NUM_SAMPLES> begin(const unsigned long asOfMS, const unsigned long asOfAdjustedSec, const std::string &title) {
        return begin<NUM_SAMPLES>(_defaultAlkMeasurementConf, asOfMS, asOfAdjustedSec, title);
    }

    template <size_t NUM_SAMPLES>
    MeasurementStepResult<NUM_SAMPLES> begin(const AlkMeasurementConfig &alkMeasureConf, const unsigned long asOfMS, const unsigned long asOfAdjustedSec, const std::string &title) {
        MeasurementStepResult<NUM_SAMPLES> r;
        r.nextAction = PRIME;
        r.nextMeasurementStepAction = STEP_INITIALIZE;
        r.alkMeasureConf = alkMeasureConf;
        r.measurementStartedAtMS = asOfMS;
        r.setTime(asOfMS, asOfAdjustedSec);
        r.alkReading.title = title;
        return r;
    }

    template <size_t NUM_SAMPLES>
    MeasurementStepResult<NUM_SAMPLES> measureAlk(std::shared_ptr<mqtt::Publisher> publisher, std::shared_ptr<buff_time::TimeWrapper> timeClient, const MeasurementStepResult<NUM_SAMPLES> &prevResult) {
        // TODO: wrap this in a transaction/finally equivalent
        if (prevResult.nextAction == PRIME) {
            MeasurementStepResult<NUM_SAMPLES> r = prevResult;

            _buffDosers->enableDosers();

            // Get everything primed and cleared out
            primeDosers(_buffDosers, r.alkMeasureConf);
            drainMeasurementVessel(*_buffDosers, r.alkMeasureConf);
            fillMeasurementVessel(*_buffDosers, r.alkMeasureConf, r.primeAndCleanupScratchData);
            stirForABit(*_buffDosers, r.alkMeasureConf);

            r.nextAction = CLEAN_AND_FILL;

            r.setTime(millis(), timeClient->getAdjustedTimeSeconds());
            return r;
        } else if (prevResult.nextAction == CLEAN_AND_FILL) {
            MeasurementStepResult<NUM_SAMPLES> r = prevResult;

            // Start the measurement
            drainMeasurementVessel(*_buffDosers, r.alkMeasureConf);
            fillMeasurementVessel(*_buffDosers, r.alkMeasureConf, r.alkReading);

            addReagentDose(*_buffDosers, r.alkMeasureConf.initialReagentDoseVolumeML, r.alkReading);
            stirForABit(*_buffDosers, r.alkMeasureConf);

            r.nextAction = MEASURE;
            r.nextMeasurementStepAction = STEP_INITIALIZE;
            r.setTime(millis(), timeClient->getAdjustedTimeSeconds());
            return r;
        } else if (prevResult.nextAction == MEASURE) {
            MeasurementStepResult<NUM_SAMPLES> r = prevResult;

            if (prevResult.nextMeasurementStepAction == MeasurementStepAction::STEP_INITIALIZE) {
                r.measuredPHStats = std::make_shared<ph::controller::PHReadingStats<NUM_SAMPLES>>();
                r.nextMeasurementStepAction = MeasurementStepAction::MEASURE_PH;
            } else if (prevResult.nextMeasurementStepAction == MeasurementStepAction::MEASURE_PH) {
                auto newPHReading = _phReader->readNewPHSignal();
                auto phReading = r.measuredPHStats->addAlkReading(newPHReading);
                r.alkReading.phReading = phReading;

                if (r.measuredPHStats->receivedMinReadings()) {
                    if (hitPHTarget(r.alkReading.phReading.calibratedPH_mavg)) {
                        r.nextAction = CLEANUP;
                        r.nextMeasurementStepAction = STEP_DONE;
                    } else if (r.alkReading.reagentVolumeML >= r.alkMeasureConf.maxReagentDoseML) {
                        Serial.println("[WARNING] Hit max reagent dose!");
                        r.nextAction = CLEANUP;
                        r.nextMeasurementStepAction = STEP_DONE;
                    } else {
                        r.nextMeasurementStepAction = MeasurementStepAction::DOSE;
                    }
                } else {
                    r.nextMeasurementStepAction = MEASURE_PH;
                }
            } else if (prevResult.nextMeasurementStepAction == MeasurementStepAction::DOSE) {
                // Note: per research on the topic (eg https://link.springer.com/chapter/10.1007/978-1-4615-2580-6_14)
                // the stirrer should be stopped before attempting to measure the pH
                addReagentDose(*_buffDosers, r.alkMeasureConf.incrementalReagentDoseVolumeML, r.alkReading);
                stirForABit(*_buffDosers, r.alkMeasureConf);

                r.nextMeasurementStepAction = STEP_INITIALIZE;
            } else {
                assert(false);
            }

            r.alkReading.alkReadingDKH = calcAlkReading(r.alkReading, r.alkMeasureConf);

            r.setTime(millis(), timeClient->getAdjustedTimeSeconds());
            return r;
        } else if (prevResult.nextAction == CLEANUP) {
            MeasurementStepResult<NUM_SAMPLES> r = prevResult;

            publisher->publishAlkReading(prevResult.alkReading);

            // Clear out all the reagent and refill with fresh tank water
            drainMeasurementVessel(*_buffDosers, r.alkMeasureConf);
            fillMeasurementVessel(*_buffDosers, r.alkMeasureConf, r.primeAndCleanupScratchData);
            stirForABit(*_buffDosers, r.alkMeasureConf);

            r.nextAction = MEASURE_DONE;
            r.setTime(millis(), timeClient->getAdjustedTimeSeconds());
            _buffDosers->disableDosers();
            return r;
        } else if (prevResult.nextAction == MEASURE_DONE) {
            return prevResult;
        }

        assert(false);
    }

    const AlkMeasurementConfig getDefaultAlkMeasurementConfig() {
        return _defaultAlkMeasurementConf;
    }
};

template <size_t NUM_SAMPLES>
class AlkMeasureLooper {
   private:
    alk_measure::MeasurementStepResult<NUM_SAMPLES> _lastStepResult;
    const std::shared_ptr<mqtt::Publisher> _publisher;
    const std::shared_ptr<AlkMeasurer> _alkMeasurer;
    const std::shared_ptr<buff_time::TimeWrapper> _timeClient;

   public:
    AlkMeasureLooper(std::shared_ptr<AlkMeasurer> alkMeasurer, std::shared_ptr<mqtt::Publisher> publisher, std::shared_ptr<buff_time::TimeWrapper> timeClient, MeasurementStepResult<NUM_SAMPLES> initialStep) : _alkMeasurer(alkMeasurer), _publisher(publisher), _timeClient(timeClient), _lastStepResult(initialStep) {}

    const MeasurementStepResult<NUM_SAMPLES> &getLastStepResult() { return _lastStepResult; }

    const MeasurementStepResult<NUM_SAMPLES> &nextStep() {
        _lastStepResult = _alkMeasurer->measureAlk(_publisher, _timeClient, _lastStepResult);
        return _lastStepResult;
    }
};

template <size_t NUM_SAMPLES>
static std::unique_ptr<AlkMeasureLooper<NUM_SAMPLES>> beginAlkMeasureLoop(std::shared_ptr<AlkMeasurer> alkMeasurer, std::shared_ptr<mqtt::Publisher> publisher, std::shared_ptr<buff_time::TimeWrapper> timeClient, const AlkMeasurementConfig &beginAlkMeasureConf, const std::string &title) {
    auto beginResult = alkMeasurer->begin<NUM_SAMPLES>(beginAlkMeasureConf, millis(), timeClient->getAdjustedTimeSeconds(), title);
    auto looper = std::make_unique<AlkMeasureLooper<NUM_SAMPLES>>(alkMeasurer, publisher, timeClient, beginResult);

    return std::move(looper);
};

}  // namespace alk_measure
}  // namespace buff

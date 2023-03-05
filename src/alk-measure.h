#pragma once

// Buff Libraries
#include "doser.h"
#include "ph.h"
#include "ph-controller.h"

namespace buff {
namespace alkmeasure {

struct AlkMeasurementConfig {
    float primeTankWaterFillVolumeML = 5;
    float primeReagentVolumeML = 0.5;

    float measurementTankWaterVolumeML = 100;
    float extraPurgeVolumeML = 10;

    float initialReagentDoseVolumeML = 2.0;
    float incrementalReagentDoseVolumeML = 0.1;
};

struct MeasurementData {
    float tankWaterVolumeML = 0.0;
    float reagentVolumeML = 0.0;
};

void stirForABit(const AlkMeasurementConfig &alkMeasureConf) {}
void enableStirrer(const AlkMeasurementConfig &alkMeasureConf) {}
void disableStirrer(const AlkMeasurementConfig &alkMeasureConf) {}

void primeFillDosers(doser::BuffDosers &buffDosers, const AlkMeasurementConfig &alkMeasureConf) {
    doser::Doser &fillDoser = buffDosers.selectDoser(doser::MeasurementDoserType::MAIN);
    doser::Doser &reagentDoser = buffDosers.selectDoser(doser::MeasurementDoserType::REAGENT);

    fillDoser.doseML(alkMeasureConf.primeTankWaterFillVolumeML);
    fillDoser.doseML(alkMeasureConf.primeReagentVolumeML);
}

void drainMeasurementVessel(doser::BuffDosers &buffDosers, const AlkMeasurementConfig &alkMeasureConf) {
    doser::Doser &drainDoser = buffDosers.selectDoser(doser::MeasurementDoserType::MAIN);
    doser::Doser &fillDoser = buffDosers.selectDoser(doser::MeasurementDoserType::DRAIN);

    drainDoser.doseML(alkMeasureConf.measurementTankWaterVolumeML + alkMeasureConf.extraPurgeVolumeML);
}

void fillMeasurementVessel(doser::BuffDosers &buffDosers, const AlkMeasurementConfig &alkMeasureConf, MeasurementData &measurementData) {
    doser::Doser &fillDoser = buffDosers.selectDoser(doser::MeasurementDoserType::DRAIN);

    fillDoser.doseML(alkMeasureConf.measurementTankWaterVolumeML);
    measurementData.tankWaterVolumeML += alkMeasureConf.measurementTankWaterVolumeML;
}

void addReagentDose(doser::BuffDosers &buffDosers, const float amountML, MeasurementData &measurementData) {
    doser::Doser &reagentDoser = buffDosers.selectDoser(doser::MeasurementDoserType::REAGENT);

    reagentDoser.doseML(amountML);
    measurementData.reagentVolumeML += amountML;
}

void addInitialReagentDose(doser::BuffDosers &buffDosers, const AlkMeasurementConfig &alkMeasureConf, MeasurementData &measurementData) {
    addReagentDose(buffDosers, alkMeasureConf.initialReagentDoseVolumeML, measurementData);
}

void addIncrementalReagentDose(doser::BuffDosers &buffDosers, const AlkMeasurementConfig &alkMeasureConf, MeasurementData &measurementData) {
    addReagentDose(buffDosers, alkMeasureConf.incrementalReagentDoseVolumeML, measurementData);
}

ph::PHReading measurePH(const ph::controller::PHReader &phReader) {
    const uint sample_count = 10;
    const float sleepDurationBetweenSamples = 1000;

    ph::PHReading lastPHReading;

    ph::controller::PHReadingStats<sample_count> measuredPHStats;

    for (int i = 0; i < sample_count; i++) {
        auto phReading = phReader.readNewPHSignal();
        lastPHReading = measuredPHStats.addReading(phReading);
        sleep(sleepDurationBetweenSamples);
    }
    
    return lastPHReading;
}

void executeMeasurementLoop(doser::BuffDosers &buffDosers, const AlkMeasurementConfig &alkMeasureConf, MeasurementData &measurementData, const ph::controller::PHReader &phReader) {
    const float phMeasurementEpsilon = 0.05;
    const float targetPH = 4.5;
    const float practicalTargetPH = targetPH + phMeasurementEpsilon;

    float currentPH = measurePH(phReader).calibratedPH_mavg;

    while (currentPH > practicalTargetPH) {
        // Note: per research on the topic (eg https://link.springer.com/chapter/10.1007/978-1-4615-2580-6_14)
        // the stirrer should be stopped before attempting to measure the pH
        addIncrementalReagentDose(buffDosers, alkMeasureConf, measurementData);
        stirForABit(alkMeasureConf);

        currentPH = measurePH(phReader).calibratedPH_mavg;
    }
}

void measureAlk(doser::BuffDosers &buffDosers, const AlkMeasurementConfig &alkMeasureConf, const ph::controller::PHReader &phReader) {
    MeasurementData primeAndCleanupScratchData;
    MeasurementData measurementData;
    // TODO: wrap this in a transaction/finally equivalent

    // Get everything primed and cleared out
    primeFillDosers(buffDosers, alkMeasureConf);
    drainMeasurementVessel(buffDosers, alkMeasureConf);
    fillMeasurementVessel(buffDosers, alkMeasureConf, primeAndCleanupScratchData);
    stirForABit(alkMeasureConf);
    drainMeasurementVessel(buffDosers, alkMeasureConf);

    // Start the measurement
    fillMeasurementVessel(buffDosers, alkMeasureConf, measurementData);
    measurementData.reagentVolumeML = alkMeasureConf.incrementalReagentDoseVolumeML;
    addInitialReagentDose(buffDosers, alkMeasureConf, measurementData);
    enableStirrer(alkMeasureConf);

    executeMeasurementLoop(buffDosers, alkMeasureConf, measurementData, phReader);

    // Clear out all the reagent and refill with fresh tank water
    disableStirrer(alkMeasureConf);
    drainMeasurementVessel(buffDosers, alkMeasureConf);
    fillMeasurementVessel(buffDosers, alkMeasureConf, primeAndCleanupScratchData);
    stirForABit(alkMeasureConf);
}

}  // namespace alkmeasure
}  // namespace buff

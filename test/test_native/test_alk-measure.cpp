#include <Arduino.h>
#include <unity.h>

#include <iterator>
#include <vector>

#include "alk-measure.h"
#include "doser.h"
#include "mqtt-common.h"
#include "ph-mock.h"

namespace test_alk_measure {
using namespace buff;

class MockDoser : public doser::Doser {
   public:
    virtual void doseML(const float outputML, doser::Calibrator *aCalibrator = nullptr) {
    }
};

#define mockptrize(mockPtr) &mockPtr->get(), [](...) {}

std::unique_ptr<doser::BuffDosers> buildMockDosers() {
    using namespace fakeit;

    auto buffDosers = std::make_unique<doser::BuffDosers>();
    for (auto i : buff::MEASUREMENT_DOSER_TYPE_NAME_TO_MEASUREMENT_DOSER) {
        auto doser = std::make_shared<MockDoser>();
        buffDosers->emplace(i.second, std::move(doser));
    }

    return std::move(buffDosers);
};

auto buildPublisherMock() {
    using namespace fakeit;
    auto publisherMock = std::make_shared<Mock<mqtt::Publisher>>();
    When(Method((*publisherMock), publishAlkReading)).AlwaysReturn();
    return publisherMock;
}

void stubs() {
    using namespace fakeit;
    When(OverloadedMethod(ArduinoFake(Serial), print, size_t(const __FlashStringHelper *))).AlwaysReturn();
    When(OverloadedMethod(ArduinoFake(Serial), print, size_t(const String &))).AlwaysReturn();
    When(OverloadedMethod(ArduinoFake(Serial), print, size_t(const char[]))).AlwaysReturn();
    When(OverloadedMethod(ArduinoFake(Serial), print, size_t(char))).AlwaysReturn();
    When(OverloadedMethod(ArduinoFake(Serial), print, size_t(unsigned char, int))).AlwaysReturn();
    When(OverloadedMethod(ArduinoFake(Serial), print, size_t(int, int))).AlwaysReturn();
    When(OverloadedMethod(ArduinoFake(Serial), print, size_t(unsigned int, int))).AlwaysReturn();
    When(OverloadedMethod(ArduinoFake(Serial), print, size_t(long, int))).AlwaysReturn();
    When(OverloadedMethod(ArduinoFake(Serial), print, size_t(unsigned long, int))).AlwaysReturn();
    When(OverloadedMethod(ArduinoFake(Serial), print, size_t(double, int))).AlwaysReturn();
    When(OverloadedMethod(ArduinoFake(Serial), print, size_t(const Printable &))).AlwaysReturn();

    When(OverloadedMethod(ArduinoFake(Serial), println, size_t(const __FlashStringHelper *))).AlwaysReturn();
    When(OverloadedMethod(ArduinoFake(Serial), println, size_t(const String &s))).AlwaysReturn();
    When(OverloadedMethod(ArduinoFake(Serial), println, size_t(const char[]))).AlwaysReturn();
    When(OverloadedMethod(ArduinoFake(Serial), println, size_t(char))).AlwaysReturn();
    When(OverloadedMethod(ArduinoFake(Serial), println, size_t(unsigned char, int))).AlwaysReturn();
    When(OverloadedMethod(ArduinoFake(Serial), println, size_t(int, int))).AlwaysReturn();
    When(OverloadedMethod(ArduinoFake(Serial), println, size_t(unsigned int, int))).AlwaysReturn();
    When(OverloadedMethod(ArduinoFake(Serial), println, size_t(long, int))).AlwaysReturn();
    When(OverloadedMethod(ArduinoFake(Serial), println, size_t(unsigned long, int))).AlwaysReturn();
    When(OverloadedMethod(ArduinoFake(Serial), println, size_t(double, int))).AlwaysReturn();
    When(OverloadedMethod(ArduinoFake(Serial), println, size_t(const Printable &))).AlwaysReturn();
    When(OverloadedMethod(ArduinoFake(Serial), println, size_t(void))).AlwaysReturn();

    When(Method(ArduinoFake(), millis)).AlwaysReturn(200000);
}

void testBeginStartsEmpty() {
    stubs();
    auto buffDosers = buildMockDosers();
    auto x = std::vector<float>({5.1, 4.9, 4.5});
    std::shared_ptr<ph::controller::PHReader> phReader = std::move(buildPHReader(x));

    alk_measure::AlkMeasurementConfig alkMeasureConf = {};
    auto publisherMock = buildPublisherMock();
    std::shared_ptr<mqtt::Publisher> publisher(mockptrize(publisherMock));

    buff::alk_measure::AlkMeasurer measurer(std::move(buffDosers), alkMeasureConf, phReader);

    auto beginStepResult = measurer.begin<1>();
    TEST_ASSERT_EQUAL(alk_measure::PRIME, beginStepResult.nextAction);
}

void testSequenceWithSingleDose() {
    using namespace fakeit;
    stubs();

    auto buffDosers = buildMockDosers();
    auto fillDoser = buffDosers->selectDoser(MeasurementDoserType::FILL);

    auto x = std::vector<float>({5.1, 5.1, 4.5, 4.5});
    std::shared_ptr<ph::controller::PHReader> phReader = std::move(buildPHReader(x));

    alk_measure::AlkMeasurementConfig alkMeasureConf = {
        .primeTankWaterFillVolumeML = 10,
        .primeReagentVolumeML = 0.5,

        .measurementTankWaterVolumeML = 200,
        .extraPurgeVolumeML = 50,

        .initialReagentDoseVolumeML = 3.0,
        .incrementalReagentDoseVolumeML = 0.1,

        .reagentStrengthMoles = 0.1};

    auto publisherMock = buildPublisherMock();
    std::shared_ptr<mqtt::Publisher> publisher(mockptrize(publisherMock));

    buff::alk_measure::AlkMeasurer measurer(std::move(buffDosers), alkMeasureConf, phReader);

    // Initial setup & fill steps
    auto beginStepResult = measurer.begin<2>();
    TEST_ASSERT_EQUAL(alk_measure::PRIME, beginStepResult.nextAction);

    auto primeStepResult = measurer.measureAlk<2>(publisher, beginStepResult);
    TEST_ASSERT_EQUAL(alk_measure::CLEAN_AND_FILL, primeStepResult.nextAction);
    TEST_ASSERT_EQUAL_FLOAT(alkMeasureConf.measurementTankWaterVolumeML, primeStepResult.primeAndCleanupScratchData.tankWaterVolumeML);
    TEST_ASSERT_EQUAL(0, primeStepResult.primeAndCleanupScratchData.reagentVolumeML);

    auto cleanAndFillStepResult = measurer.measureAlk<2>(publisher, primeStepResult);
    TEST_ASSERT_EQUAL(alk_measure::MEASURE, cleanAndFillStepResult.nextAction);
    TEST_ASSERT_EQUAL(alk_measure::STEP_INITIALIZE, cleanAndFillStepResult.nextMeasurementStepAction);
    TEST_ASSERT_EQUAL_FLOAT(200.0, cleanAndFillStepResult.alkReading.tankWaterVolumeML);
    TEST_ASSERT_EQUAL_FLOAT(3.0, cleanAndFillStepResult.alkReading.reagentVolumeML);

    // Measurement steps
    // Measurement1: 5.1
    auto measureStepResult = measurer.measureAlk<2>(publisher, cleanAndFillStepResult);
    TEST_ASSERT_EQUAL(alk_measure::MEASURE, measureStepResult.nextAction);
    TEST_ASSERT_EQUAL(alk_measure::MEASURE_PH, measureStepResult.nextMeasurementStepAction);

    measureStepResult = measurer.measureAlk<2>(publisher, measureStepResult);
    TEST_ASSERT_EQUAL(alk_measure::MEASURE, measureStepResult.nextAction);
    TEST_ASSERT_EQUAL(alk_measure::MEASURE_PH, measureStepResult.nextMeasurementStepAction);
    TEST_ASSERT_EQUAL_FLOAT(5.1, measureStepResult.alkReading.phReading.calibratedPH);

    measureStepResult = measurer.measureAlk<2>(publisher, measureStepResult);
    TEST_ASSERT_EQUAL(alk_measure::MEASURE, measureStepResult.nextAction);
    TEST_ASSERT_EQUAL(alk_measure::DOSE, measureStepResult.nextMeasurementStepAction);
    TEST_ASSERT_EQUAL_FLOAT(5.1, measureStepResult.alkReading.phReading.calibratedPH);

    measureStepResult = measurer.measureAlk<2>(publisher, measureStepResult);
    TEST_ASSERT_EQUAL(alk_measure::MEASURE, measureStepResult.nextAction);
    TEST_ASSERT_EQUAL(alk_measure::STEP_INITIALIZE, measureStepResult.nextMeasurementStepAction);
    TEST_ASSERT_EQUAL_FLOAT(3.1, measureStepResult.alkReading.reagentVolumeML);

    // Measurement2: 4.5
    // STEP_INITIALIZE
    measureStepResult = measurer.measureAlk<2>(publisher, measureStepResult);
    TEST_ASSERT_EQUAL(alk_measure::MEASURE, measureStepResult.nextAction);
    TEST_ASSERT_EQUAL(alk_measure::MEASURE_PH, measureStepResult.nextMeasurementStepAction);

    // MEASURE_PH
    measureStepResult = measurer.measureAlk<2>(publisher, measureStepResult);
    TEST_ASSERT_EQUAL_FLOAT(4.5, measureStepResult.alkReading.phReading.calibratedPH);
    TEST_ASSERT_EQUAL(alk_measure::MEASURE, measureStepResult.nextAction);
    TEST_ASSERT_EQUAL(alk_measure::MEASURE_PH, measureStepResult.nextMeasurementStepAction);

    measureStepResult = measurer.measureAlk<2>(publisher, measureStepResult);
    TEST_ASSERT_EQUAL(alk_measure::CLEANUP, measureStepResult.nextAction);
    TEST_ASSERT_EQUAL(alk_measure::STEP_DONE, measureStepResult.nextMeasurementStepAction);
    TEST_ASSERT_EQUAL_FLOAT(4.34, measureStepResult.alkReading.alkReadingDKH);

    // CLEANUP
    auto cleanupResult = measurer.measureAlk<2>(publisher, measureStepResult);
    TEST_ASSERT_EQUAL(alk_measure::MEASURE_DONE, cleanupResult.nextAction);
    TEST_ASSERT_EQUAL(alk_measure::STEP_DONE, cleanupResult.nextMeasurementStepAction);
    TEST_ASSERT_EQUAL_FLOAT(200.0, cleanupResult.alkReading.tankWaterVolumeML);
    TEST_ASSERT_EQUAL_FLOAT(3.1, cleanupResult.alkReading.reagentVolumeML);
    // =3.1/200*280
    TEST_ASSERT_EQUAL_FLOAT(4.34, cleanupResult.alkReading.alkReadingDKH);
    Verify(Method((*publisherMock), publishAlkReading).Matching([](const alk_measure::AlkReading &alkReading) { return abs(alkReading.alkReadingDKH - 4.34) < 0.01; })).Exactly(Once);
}

}  // namespace test_alk_measure

void runAlkMeasureTests() {
    // When(OverloadedMethod(ArduinoFake(Serial), print, size_t(char))).AlwaysReturn();
    // When(OverloadedMethod(ArduinoFake(Serial), print, size_t(int, int))).AlwaysReturn();

    RUN_TEST(test_alk_measure::testBeginStartsEmpty);
    RUN_TEST(test_alk_measure::testSequenceWithSingleDose);
}

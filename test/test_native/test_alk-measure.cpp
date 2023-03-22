#include "ArduinoFake.h"

// #include "Arduino.h"

#include <unity.h>

#include <iterator>
#include <vector>

#include "ph-controller.h"
#include "ph.h"

using namespace buff;

ph::PHCalibrator::CalibrationPoint NoOpHighPoint = {.actualPH = 7.0, .readPH = 7.0};
ph::PHCalibrator::CalibrationPoint NoOpLowPoint = {.actualPH = 4.0, .readPH = 4.0};
ph::PHCalibrator NoOpPHCalibrator(NoOpLowPoint, NoOpHighPoint);

struct callable_iter {
    callable_iter(const std::vector<float> &vals) {
        _iter = vals.begin();
    }

    float operator()() {
        auto val = *_iter;
        _iter++;
        return val;
    }

   private:
    std::vector<float>::const_iterator _iter;
};

ph::controller::PHReader buildPHReader(std::vector<float> &vals, ph::PHCalibrator &calibrator = NoOpPHCalibrator) {
    std::function<float()> readerFunc = callable_iter(vals);

    const ph::PHReadConfig phReadConfig = {
        .readIntervalMS = 1000,
        .phReadFunc = readerFunc};

    ph::controller::PHReader phReader(phReadConfig, calibrator);

    return phReader;
}

void testPHReaderHelper() {
    auto x = std::vector<float>({5.1, 4.9, 4.5});
    auto reader = buildPHReader(x);

    auto signal = reader.readNewPHSignal(1000);
    TEST_ASSERT_EQUAL_FLOAT(5.1, signal.rawPH);

    auto signal2 = reader.readNewPHSignal(2000);
    auto signal3 = reader.readNewPHSignal(3000);
    TEST_ASSERT_EQUAL(4.5, signal3.rawPH);
}

void testPHCalibration() {
    ph::PHCalibrator::CalibrationPoint highPoint = {.actualPH = 7.0, .readPH = 14.0};
    ph::PHCalibrator::CalibrationPoint lowPoint = {.actualPH = 4.0, .readPH = 4.0};
    ph::PHCalibrator calib(lowPoint, highPoint);

    auto x = std::vector<float>({14.0});
    auto reader = buildPHReader(x, calib);

    auto signal = reader.readNewPHSignal(1000);
    TEST_ASSERT_EQUAL_FLOAT(14.0, signal.rawPH);
    TEST_ASSERT_EQUAL_FLOAT(7.0, signal.calibratedPH);
}

void tearDown(void) {
}

void setUp() {
    ArduinoFakeReset();
}

int main(int argc, char **argv) {
    UNITY_BEGIN();
    RUN_TEST(testPHReaderHelper);
    RUN_TEST(testPHCalibration);
    UNITY_END();

    return 0;
}

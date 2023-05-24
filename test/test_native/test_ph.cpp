#include <unity.h>

#include <vector>

#include "ph-controller.h"
#include "ph-mock.h"
#include "readings/ph.h"

namespace test_ph {
using namespace buff;

void testPHReaderHelper() {
    auto x = std::vector<float>({5.1, 4.9, 4.5});
    auto reader = *buildPHReader(x);

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
    auto reader = *buildPHReader(x, calib);

    auto signal = reader.readNewPHSignal(1000);
    TEST_ASSERT_EQUAL_FLOAT(14.0, signal.rawPH);
    TEST_ASSERT_EQUAL_FLOAT(7.0, signal.calibratedPH);
}

}  // namespace test_ph

void runPHTests() {
    RUN_TEST(test_ph::testPHReaderHelper);
    RUN_TEST(test_ph::testPHCalibration);
}

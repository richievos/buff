#include <unity.h>

#include <vector>

#include "ph-controller.h"
#include "ph-mock.h"
#include "ph.h"
#include "numeric.h"

namespace test_numeric {
using namespace buff;

void testExtractBasic() {
    auto result = numeric::smallFloatToStorableParts(15.9);
    TEST_ASSERT_EQUAL(15, result.integerPart);
    TEST_ASSERT_EQUAL(9, result.decimalPart);
    TEST_ASSERT_EQUAL_FLOAT(15.9, storablePartsToFloat(result));
}

void testExtractZeroes() {
    auto result = numeric::smallFloatToStorableParts(1.0);
    TEST_ASSERT_EQUAL(1, result.integerPart);
    TEST_ASSERT_EQUAL(0, result.decimalPart);
    TEST_ASSERT_EQUAL_FLOAT(1.0, storablePartsToFloat(result));

    result = numeric::smallFloatToStorableParts(0.1);
    TEST_ASSERT_EQUAL(0, result.integerPart);
    TEST_ASSERT_EQUAL(1, result.decimalPart);
    TEST_ASSERT_EQUAL_FLOAT(0.1, storablePartsToFloat(result));

    result = numeric::smallFloatToStorableParts(0.0);
    TEST_ASSERT_EQUAL(0, result.integerPart);
    TEST_ASSERT_EQUAL(0, result.decimalPart);
    TEST_ASSERT_EQUAL_FLOAT(0.0, storablePartsToFloat(result));
}

void testStorageBothWays() {
    auto encoded = numeric::smallFloatToByte(1.0);
    TEST_ASSERT_EQUAL_HEX8(0x10, encoded & 0xF0);
    TEST_ASSERT_EQUAL_HEX8(0x0, encoded & 0xF);
    TEST_ASSERT_EQUAL_FLOAT(1.0, numeric::byteToSmallFloat(encoded));

    encoded = numeric::smallFloatToByte(1.1);
    TEST_ASSERT_EQUAL_HEX8(0x10, encoded & 0xF0);
    TEST_ASSERT_EQUAL_HEX8(0x1, encoded & 0xF);
    TEST_ASSERT_EQUAL_FLOAT(1.1, numeric::byteToSmallFloat(encoded));

    encoded = numeric::smallFloatToByte(11.8);
    TEST_ASSERT_EQUAL_FLOAT(11.8, numeric::byteToSmallFloat(encoded));
}

}  // namespace test_numeric

void runNumericTests() {
    RUN_TEST(test_numeric::testExtractBasic);
    RUN_TEST(test_numeric::testExtractZeroes);
    RUN_TEST(test_numeric::testStorageBothWays);
}

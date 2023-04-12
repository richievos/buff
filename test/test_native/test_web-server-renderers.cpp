#include <unity.h>

#include <string>
#include <vector>

#include "web-server-renderers.h"

namespace web_server {
using namespace buff;

#include <iostream>
void TEST_CONTAINS_SUBSTRING(const std::string &expected, const std::string &actual) {
    TEST_ASSERT_GREATER_OR_EQUAL_MESSAGE(0, actual.find(expected), actual.c_str());
}

void testFormDefaultsToLatestTitle() {
    auto readings = std::vector<std::reference_wrapper<alk_measure::PersistedAlkReading>>();
    alk_measure::PersistedAlkReading first = {.title = "first"};
    alk_measure::PersistedAlkReading last = {.title = "last "};
    readings.push_back(first);
    readings.push_back(last);
    std::string out;
    ::buff::web_server::renderRoot(out, 1, "", 1111, 2222, readings);
    TEST_CONTAINS_SUBSTRING(R"(value="first")", out);
}

}  // namespace web_server

void runWebServerTests() {
    RUN_TEST(web_server::testFormDefaultsToLatestTitle);
}

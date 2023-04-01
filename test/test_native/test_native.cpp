// #include "./test_alk-measure.cpp"
// #include "./test_ph.cpp"

extern void runPHTests();
extern void runAlkMeasureTests();

#include "ArduinoFake.h"
#include <unity.h>

void tearDown(void) {
}

void setUp() {
    ArduinoFakeReset();
}

int main(int argc, char **argv) {
    UNITY_BEGIN();
    runPHTests();
    runAlkMeasureTests();
    UNITY_END();

    return 0;
}

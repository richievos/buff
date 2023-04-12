extern void runPHTests();
extern void runAlkMeasureTests();
extern void runNumericTests();
extern void runWebServerTests();

#include <unity.h>

#include "ArduinoFake.h"

void tearDown(void) {
}

void setUp() {
    ArduinoFakeReset();
}

int main(int argc, char **argv) {
    UNITY_BEGIN();
    runPHTests();
    runNumericTests();
    runAlkMeasureTests();
    runWebServerTests();
    return UNITY_END();
}

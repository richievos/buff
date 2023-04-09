extern void runPHTests();
extern void runAlkMeasureTests();
extern void runNumericTests();

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
    return UNITY_END();
}

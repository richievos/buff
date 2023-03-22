#include <unity.h>

void tearDown(void) {
    // clean stuff up here
}

void setUp() {
}

void testNoop() { }

int main(int argc, char **argv) {
    UNITY_BEGIN();
    RUN_TEST(testNoop);
    UNITY_END();

    return 0;
}

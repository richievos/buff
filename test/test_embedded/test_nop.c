#include <unity.h>

void tearDown(void) {
    // clean stuff up here
}

void setUp() {
}


int main(int argc, char **argv) {
    UNITY_BEGIN();
    RUN_TEST(testFoo);
    UNITY_END();

    return 0;
}

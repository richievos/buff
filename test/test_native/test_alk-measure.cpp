#include "ArduinoFake.h"

// #include "Arduino.h"

#include <unity.h>

#include <iterator>
#include <vector>

#include "ph-controller.h"
#include "ph.h"

template <typename Iterator>
class FakePHReader {
    typedef typename Iterator::value_type value_type;

   private:
    Iterator _readings;

    buff::ph::PHReadConfig buildPHReadConfig() {
        const buff::ph::PHReadConfig ph_read_config = {
            .readIntervalMS = 1000,
            .phReadFunc = readerFunc};

        return ph_read_config;
    }

   public:
    FakePHReader(Iterator readings) : _readings(readings) { }

    value_type readerFunc() {
        using value_type = typename std::iterator_traits<Iterator>::value_type;
        value_type s = *_readings;
        _readings++;
        return s;
    }
    // #define nameForRoboTankSignalReaderFunc(i2cAddress) roboTankSignalReaderFunc##i2cAddress

    // #define defineRoboTankSignalReaderFunc(i2cAddress)        \
    //     float nameForRoboTankSignalReaderFunc(i2cAddress)() { \
    //         return readPHSignal_RoboTankPHBoard(i2cAddress);  \
    //     }
};

void testFoo() {
    auto x = std::vector<float>({5.0, 5.0, 4.5});
    FakePHReader<std::vector<float>> phReader(x);
}

// void setup(void) {
//     // NOTE!!! Wait for >2 secs
//     // if board doesn't support software reset via Serial.DTR/RTS
//     // delay(2000);

//     // pinMode(LED_BUILTIN, OUTPUT);

//     UNITY_BEGIN();  // IMPORTANT LINE!
//     // RUN_TEST(testFoo);
// }

void tearDown(void) {
    // clean stuff up here
}

void setUp() {
    ArduinoFakeReset();
}

void loop() {
    //   if (i < max_blinks)
    //   {
    //     RUN_TEST(test_led_state_high);
    //     delay(500);
    //     RUN_TEST(test_led_state_low);
    //     delay(500);
    //     i++;
    //   }
    //   else if (i == max_blinks)
    //   {
    //     UNITY_END(); // stop unit testing
    //   }
}

int main(int argc, char **argv) {
    UNITY_BEGIN();
    RUN_TEST(testFoo);
    UNITY_END();

    return 0;
}

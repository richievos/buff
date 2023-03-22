#pragma once

// #include <Arduino.h>
#include <Wire.h>

/*******************************
 * RoboTank PH Sensor Integration
 *******************************/
void setupPH_RoboTankPHBoard() {
    ::Wire.begin();
    ::Wire.setClock(10000);  // set I2C bus to 10 KHz - this is important!
}

// read the current ph directly from the board
float readPHSignal_RoboTankPHBoard(const uint8_t i2cAddress) {
    ::Wire.beginTransmission(i2cAddress);  // start transmission
    ::Wire.write("R");                     // ask for pH
    ::Wire.write(0);                       // send closing byte
    ::Wire.endTransmission();              // end transmission

    uint8_t count = 0;  // count imcoming bytes
    uint8_t byteCountToRead = 8;
    String i = "";                                  // create string of incoming data
    ::Wire.requestFrom(i2cAddress, byteCountToRead);  // request 8 bytes from pH circuit
    while (::Wire.available())                        // read the 8 incoming bytes
    {
        char c = Wire.read();  // receive a byte as character
        if (count > 0) {
            i = i + c;
        }         // ignore first btye and combine remaining into a string
        count++;  // count byte
    }
    float pH = i.toFloat();  // convert pH string to float
    return pH;               // return pH
}

#define nameForRoboTankSignalReaderFunc(i2cAddress) roboTankSignalReaderFunc##i2cAddress

#define defineRoboTankSignalReaderFunc(i2cAddress)        \
    float nameForRoboTankSignalReaderFunc(i2cAddress)() { \
        return readPHSignal_RoboTankPHBoard(i2cAddress);  \
    }

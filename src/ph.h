#pragma once

#include <SPI.h>
#include <Wire.h>

// My Libraries
#include "inputs.h"

class PHCalibrator {
  public:
  struct CalibrationPoint {
    const float actualPH;
    const float readPH;
  };

  private:
  const CalibrationPoint _lowPoint;
  const CalibrationPoint _highPoint;

  public:
  PHCalibrator(CalibrationPoint lowPoint, CalibrationPoint highPoint): _lowPoint(lowPoint), _highPoint(highPoint) {};

  float convert(float reading) {
    // pHx = pHref1 + (pHref2 – pHref1) * (Mx – Mref1) / (Mref2 - Mref1)
    // pHx = 4 + 3 * (Mx – M4) / (M7 - M4)

    return _lowPoint.actualPH +
            (_highPoint.actualPH - _lowPoint.actualPH) *
              (reading - _lowPoint.readPH) /
              (_highPoint.readPH - _lowPoint.readPH);
  }
};

/*******************************
 * Useful functions
 *******************************/
// get the current pH
float getPH(const uint8_t i2cAddress) {
    Wire.beginTransmission(i2cAddress);  // start transmission
    Wire.write("R");                       // ask for pH
    Wire.write(0);                         // send closing byte
    Wire.endTransmission();                // end transmission

    uint8_t count = 0;  // count imcoming bytes
    uint8_t byteCountToRead = 8;
    String i = "";                                    // create string of incoming data
    Wire.requestFrom(i2cAddress, byteCountToRead);  // request 8 bytes from pH circuit
    while (Wire.available())                          // read the 8 incoming bytes
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

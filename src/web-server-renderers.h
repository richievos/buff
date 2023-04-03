#pragma once

#include "Arduino.h"

#include <list>
#include <string>

#include "alk-measure-common.h"

namespace buff {
namespace web_server {

std::string renderMeasurementList(char *temp, size_t temp_size, const std::list<alk_measure::AlkReading> mostRecentReadings) {
    std::string measurementString = "<ol>\n";
    const auto alkMeasureTemplate = R"(
      <li class="measurement">
        <span class="asOf">%u:</span> <span class="alkReadingDH">%f</span>
      </li>
    )";
    for (auto measurement : mostRecentReadings) {
        snprintf(temp, temp_size, alkMeasureTemplate, measurement.asOfMS, measurement.alkReadingDKH);
        measurementString += temp;
    }
    measurementString += "\n</ol>";
    return measurementString;
}

std::string renderFooter(char *temp, size_t temp_size) {
    unsigned long time = millis();
    int sec = time / 1000;
    int min = sec / 60;
    int hr = min / 60;


    snprintf(temp, temp_size,
             "<footer>Uptime: %02d:%02d:%02d</footer>",
             hr, min % 60, sec % 60);
    return temp;
}

void renderRoot(std::string &out, const std::list<alk_measure::AlkReading> mostRecentReadings) {
    char temp[400];
    memset(temp, 0, 400);

    out += R"(
<html>
  <head>
    <title>Buff</title>
    <style>
      body { background-color: #cccccc; font-family: Arial, Helvetica, Sans-Serif; Color: #000088; }
    </style>
  </head>
  <body>
    <header>Buff</header>
    )";
    out += renderMeasurementList(temp, 400, mostRecentReadings);
    out += renderFooter(temp, 400);
    out += R"(
  </body>
</html>
    )";
}

}  // namespace web_server
}  // namespace buff

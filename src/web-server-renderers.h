#pragma once

#include <list>
#include <string>

#include "Arduino.h"
#include "alk-measure-common.h"
#include "reading-store.h"

namespace buff {
namespace web_server {

std::string renderMeasurementList(char *temp, size_t temp_size, const std::vector<std::reference_wrapper<reading_store::PersistedAlkReading>> mostRecentReadings) {
    std::string measurementString = "<table>\n";
    const auto alkMeasureTemplate = R"(
      <tr class="measurement">
        <td class="asOf">%u:</td>
        <td class="title">%s</td>
        <td class="alkReadingDKH">%.1f</td>
      </tr>
    )";
    for (auto &measurementRef : mostRecentReadings) {
        auto &measurement = measurementRef.get();
        snprintf(temp, temp_size, alkMeasureTemplate, measurement.asOfMSAdjusted, measurement.title.c_str(), measurement.alkReadingDKH);
        measurementString += temp;
    }
    measurementString += "\n</table>";
    return measurementString;
}

std::string renderFooter(char *temp, size_t temp_size, const unsigned long renderTimeMS) {
    int renderTimeSec = renderTimeMS / 1000;
    int renderTimeMin = renderTimeSec / 60;
    int renderTimeHr = renderTimeMin / 60;

    int millisSec = millis() / 1000;
    int millisMin = millisSec / 60;
    int millisHr = millisMin / 60;

    snprintf(temp, temp_size,
             "<footer>Current time: %02d:%02d:%02d, Uptime: %02d:%02d:%02d</footer>",
             renderTimeHr, renderTimeMin % 60, renderTimeSec % 60,
             millisHr, millisMin % 60, millisSec % 60);
    return temp;
}

void renderRoot(std::string &out, const unsigned long renderTimeMS, const std::vector<std::reference_wrapper<reading_store::PersistedAlkReading>> mostRecentReadings) {
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
    out += renderFooter(temp, 400, renderTimeMS);
    out += R"(
  </body>
</html>
    )";
}

}  // namespace web_server
}  // namespace buff

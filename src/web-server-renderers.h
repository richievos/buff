#pragma once

#include "Arduino.h"

#include <list>
#include <string>

#include "alk-measure-common.h"
#include "reading-store.h"


namespace buff {
namespace web_server {

std::string renderMeasurementList(char *temp, size_t temp_size, const std::vector<reading_store::PersistedAlkReading> mostRecentReadings) {
    std::string measurementString = "<table>\n";
    const auto alkMeasureTemplate = R"(
      <tr class="measurement">
        <td class="asOf">%u:</td>
        <td class="title">%s</td>
        <td class="alkReadingDKH">%.1f</td>
      </tr>
    )";
    for (auto measurement : mostRecentReadings) {
        snprintf(temp, temp_size, alkMeasureTemplate, measurement.asOfMSAdjusted, measurement.alkReadingDKH);
        measurementString += temp;
    }
    measurementString += "\n</table>";
    return measurementString;
}

std::string renderFooter(char *temp, size_t temp_size, const unsigned long renderTimeMS) {
    int sec = renderTimeMS / 1000;
    int min = sec / 60;
    int hr = min / 60;


    snprintf(temp, temp_size,
             "<footer>Uptime: %02d:%02d:%02d</footer>",
             hr, min % 60, sec % 60);
    return temp;
}

void renderRoot(std::string &out, const unsigned long renderTimeMS, const std::vector<reading_store::PersistedAlkReading> mostRecentReadings) {
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

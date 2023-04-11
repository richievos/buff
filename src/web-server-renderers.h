#pragma once

#include <ctime>
#include <list>
#include <string>

#include "Arduino.h"
#include "alk-measure-common.h"
#include "reading-store.h"

namespace buff {
namespace web_server {

std::string renderTime(char *temp, size_t bufferSize, const unsigned long timeInSec) {
    // millis to time
    const time_t rawtime = (time_t)timeInSec;
    struct tm *dt = gmtime(&rawtime);

    // format
    strftime(temp, bufferSize, "%Y-%m-%d %H:%M:%S", dt);
    return temp;
}

std::string renderTriggerForm(char *temp, size_t bufferSize, const unsigned long renderTimeMS, const std::string &mostRecentTitle) {
    std::string formTemplate = R"(
      <section class="row">
        <div class="col-md-1">
          <form class="form-inline" action="/execute/measure_alk">
            <input type="hidden" name="asOf" id="asOf" value="%u"/>

            <div class="form-group">
              <label for="title">Title</label>
              <input class="form-control" type="text" name="title" id="title" value="%s" />
            </div>
            <button class="btn btn-default" type="submit">Start a Measurement</button>
          </form>
        </div>
      </section>
    )";

    snprintf(temp, bufferSize, formTemplate.c_str(), mostRecentTitle.c_str(), renderTimeMS);

    return temp;
}

std::string renderMeasurementList(char *temp, size_t bufferSize, const std::vector<std::reference_wrapper<reading_store::PersistedAlkReading>> mostRecentReadings) {
    std::string measurementString = R"(<table class="table table-striped">)";
    const auto alkMeasureTemplate = R"(
      <tr class="measurement">
        <td class="asOf">%s</td>
        <td class="title">%s</td>
        <td class="alkReadingDKH">%.1f</td>
      </tr>
    )";
    for (auto &measurementRef : mostRecentReadings) {
        auto &measurement = measurementRef.get();
        if (measurement.alkReadingDKH != 0) {
            snprintf(temp, bufferSize, alkMeasureTemplate,
                     renderTime(temp, bufferSize, measurement.asOfAdjustedSec).c_str(),
                     measurement.title.c_str(), measurement.alkReadingDKH);
            measurementString += temp;
        }
    }
    measurementString += "</table>";
    return measurementString;
}

std::string renderFooter(char *temp, size_t bufferSize, const unsigned long renderTimeMS) {
    int millisSec = millis();
    int millisMin = millisSec / 60;
    int millisHr = millisMin / 60;

    snprintf(temp, bufferSize,
             R"(<footer class="row"><div class="col-md-1">Current time: %s, Uptime: %02d:%02d:%02d</div></footer>)",
             renderTime(temp, bufferSize, renderTimeMS).c_str(),
             millisHr, millisMin % 60, millisSec % 60);
    return temp;
}

void renderRoot(std::string &out, const std::string triggered, const unsigned long renderTimeMS, const std::vector<std::reference_wrapper<reading_store::PersistedAlkReading>> mostRecentReadings) {
    const size_t bufferSize = 1023;
    char temp[bufferSize];
    memset(temp, 0, bufferSize);

    std::string mostRecentTitle = "";
    if (mostRecentReadings.size() > 0) {
        mostRecentTitle = mostRecentReadings.front().get().title;
    }

    std::string triggeredContent = "";
    if (triggered == "true") {
        triggeredContent = R"(<section id="alert success"><div>Successfully triggered a measurement!</div></section>)";
    } else if (triggered == "false") {
        triggeredContent = R"(<section id="alert failure"><div>Failed to trigger a measurement!</div></section>)";
    }

    out += R"(
<!doctype html>
<html lang="en">
  <head>
    <title>Buff</title>
    <!--<link rel="stylesheet" href="https://cdn.jsdelivr.net/npm/bootstrap@5.2.3/dist/css/bootstrap.min.css" />-->
    <link rel="stylesheet" href="https://cdn.jsdelivr.net/npm/bootstrap@4.3.1/dist/css/bootstrap.min.css" />
    <meta name="viewport" content="width=device-width, initial-scale=1, maximum-scale=1, user-scalable=no">
  </head>
  <body>
    <div class="container-fluid">
      <header class="row">
        <div class="col-md-1">
          <h1 id="pageTitle">Buff</h1>
        </div>
      </header>
    )";
    out += triggeredContent;

    out += renderTriggerForm(temp, bufferSize, renderTimeMS, mostRecentTitle);
    out += renderMeasurementList(temp, bufferSize, mostRecentReadings);
    out += renderFooter(temp, bufferSize, renderTimeMS);
    out += R"(
      </div>
  </body>
</html>
    )";
}

}  // namespace web_server
}  // namespace buff

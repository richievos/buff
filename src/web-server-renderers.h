#pragma once

#include <ctime>
#include <list>
#include <string>

#include "Arduino.h"
#include "alk-measure-common.h"

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

std::string renderTriggerForm(char *temp, size_t bufferSize, const unsigned long renderTimeSec, const std::string &mostRecentTitle) {
    std::string formTemplate = R"(
      <section class="row">
        <form class="form-inline row row-cols-lg-auto align-items-center" action="/execute/measure_alk">
          <input type="hidden" name="asOf" id="asOf" value="%lu"/>

          <div class="col-12 form-floating">
            <input class="form-control" type="text" name="title" id="title" value="%s" />
            <label for="title">Title</label>
          </div>

          <div class="col-12">
            <button class="btn btn-primary" type="submit">Start a Measurement</button>
          </div>
        </form>
      </section>
    )";

    snprintf(temp, bufferSize, formTemplate.c_str(), renderTimeSec, mostRecentTitle.c_str());
    return temp;
}

std::string renderMeasurementList(char *temp, size_t bufferSize, const std::vector<std::reference_wrapper<alk_measure::PersistedAlkReading>> &mostRecentReadings) {
    std::string measurementString = R"(<section class="row mt-3"><div class="col"><table class="table table-striped">)";
    const auto alkMeasureTemplate = R"(
      <tr class="measurement">
        <td class="asOf converted-time" data-epoch-sec="%lu">%s</td>
        <td class="title">%s</td>
        <td class="alkReadingDKH">%.1f</td>
      </tr>
    )";
    for (auto &measurementRef : mostRecentReadings) {
        auto &measurement = measurementRef.get();
        if (measurement.alkReadingDKH != 0) {
            snprintf(temp, bufferSize, alkMeasureTemplate,
                     measurement.asOfAdjustedSec,
                     renderTime(temp, bufferSize, measurement.asOfAdjustedSec).c_str(),
                     measurement.title.c_str(), measurement.alkReadingDKH);
            measurementString += temp;
        }
    }
    measurementString += "</table></div></section>";
    return measurementString;
}

std::string renderHeader(char *temp, size_t bufferSize, const ph::PHReading &reading) {
    const auto headerTemplate = R"(<header class="navbar">
    <div class="navbar-brand">Buff</div>
    <div class="navbar-text">pH: %.1f</div>
  </header>)";

    snprintf(temp, bufferSize,
             headerTemplate,
             reading.calibratedPH_mavg);
    return temp;
}

std::string renderFooter(char *temp, size_t bufferSize, const unsigned long renderTimeSec, const unsigned long uptimeMS) {
    const auto footerTemplate = R"(
        <footer class="row">
          <div class="col">
            Current time:
            <span class="converted-time" data-epoch-sec="%lu">
              %s
            </span>, Uptime: %02d:%02d:%02d
          </div>
        </footer>)";

    int millisSec = uptimeMS / 1000;
    int millisMin = millisSec / 60;
    int millisHr = millisMin / 60;

    snprintf(temp, bufferSize,
             footerTemplate,
             renderTimeSec,
             renderTime(temp, bufferSize, renderTimeSec).c_str(),
             millisHr, millisMin % 60, millisSec % 60);
    return temp;
}

std::string renderAlerts(char *temp, size_t bufferSize, const unsigned long elapsedMeasurementTime, const std::string &triggered) {
    std::string alertContent = "";
    if (triggered == "true") {
        alertContent = R"(<section class="alert alert-success">Successfully triggered a measurement!</section>)";
    } else if (triggered == "false") {
        alertContent = R"(<section class="alert alert-warning">Failed to trigger a measurement!</section>)";
    }

    if (elapsedMeasurementTime != 0) {
        const std::string measuringTemplate = R"(<section class="alert alert-primary">Currently measuring (for %us)</section>)";
        snprintf(temp, bufferSize, measuringTemplate.c_str(), elapsedMeasurementTime);
        alertContent += temp;
    }
    return alertContent;
}

void renderRoot(std::string &out, const unsigned long elapsedMeasurementTime, const std::string &triggered, const unsigned long renderTimeSec, const unsigned long uptimeMS, const std::vector<std::reference_wrapper<alk_measure::PersistedAlkReading>> &mostRecentReadings, const ph::PHReading &phReading) {
    const size_t bufferSize = 1023;
    char temp[bufferSize];
    memset(temp, 0, bufferSize);

    std::string mostRecentTitle = "";
    if (mostRecentReadings.size() > 0) {
        mostRecentTitle = mostRecentReadings.front().get().title;
    }

    std::string triggeredContent = "";
    if (triggered == "true") {
        triggeredContent = R"(<section class="alert alert-success">Successfully triggered a measurement!</section>)";
    } else if (triggered == "false") {
        triggeredContent = R"(<section class="alert alert-warning">Failed to trigger a measurement!</section>)";
    }

    if (elapsedMeasurementTime != 0) {
        triggeredContent = R"(<section class="alert alert-primary">Currently measuring (for %us)</section>)";
    }

    out += R"(
<!doctype html>
<html lang="en">
  <head>
    <title>Buff</title>
    <link rel="stylesheet" href="https://cdn.jsdelivr.net/npm/bootstrap@5.2.3/dist/css/bootstrap.min.css" />
    <meta name="viewport" content="width=device-width, initial-scale=1, maximum-scale=1, user-scalable=no">
    <meta charset="utf-8">
  </head>
  <body>
    <div class="container-fluid">
    )";
    out += renderHeader(temp, bufferSize, phReading);
    out += renderAlerts(temp, bufferSize, elapsedMeasurementTime, triggered);
    out += renderTriggerForm(temp, bufferSize, renderTimeSec, mostRecentTitle);
    out += renderMeasurementList(temp, bufferSize, mostRecentReadings);
    out += renderFooter(temp, bufferSize, renderTimeSec, uptimeMS);
    out += R"(
      </div>
      <script src="https://code.jquery.com/jquery-3.6.4.slim.min.js" integrity="sha256-a2yjHM4jnF9f54xUQakjZGaqYs/V1CYvWpoqZzC2/Bw=" crossorigin="anonymous"></script>
      <script src="https://cdn.jsdelivr.net/npm/bootstrap@5.3.0-alpha3/dist/js/bootstrap.bundle.min.js" integrity="sha384-ENjdO4Dr2bkBIFxQpeoTz1HIcje39Wm4jDKdf19U8gI4ddQ3GYNS7NTKfAdVQSZe" crossorigin="anonymous"></script>
      <script src="https://cdnjs.cloudflare.com/ajax/libs/luxon/3.3.0/luxon.min.js" integrity="sha512-KKbQg5o92MwtJKR9sfm/HkREzfyzNMiKPIQ7i7SZOxwEdiNCm4Svayn2DBq7MKEdrqPJUOSIpy1v6PpFlCQ0YA==" crossorigin="anonymous" referrerpolicy="no-referrer"></script>
      <script>
        $('.converted-time').each(function(index, item) {
          const { DateTime } = luxon;
          var epochSec = $(item).data("epoch-sec");
          var timeString = DateTime.fromSeconds(epochSec).toFormat('yyyy-MM-dd HH:mm:ss');
          $(item).text(timeString)
        })
      </script>
  </body>
</html>
    )";
}

}  // namespace web_server
}  // namespace buff

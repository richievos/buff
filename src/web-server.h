#pragma once

#include <Arduino.h>
#include <WebServer.h>  // Built into ESP32

#include <string>

#include "readings/alk-measure-common.h"
#include "readings/reading-store.h"
#include "string-manip.h"
#include "time-common.h"
#include "web-server-renderers.h"

namespace buff {
namespace web_server {

struct TriggerRequest {
    std::string title;
    unsigned long asOf;
};

class BuffWebServer {
   private:
    std::shared_ptr<reading_store::ReadingStore> _readingStore = nullptr;
    std::shared_ptr<buff_time::TimeWrapper> _timeClient = nullptr;
    WebServer _server;

    unsigned long _currentElapsedMeasurementTimeMS = 0;

    std::unique_ptr<TriggerRequest> _pendingTrigger;

   public:
    BuffWebServer(std::shared_ptr<buff_time::TimeWrapper> timeClient, int port=80) : _server(port), _timeClient(timeClient) {}

    void handleRoot() {
        std::string bodyText;
        auto readings = _readingStore->getReadingsSortedByAsOf();
        renderRoot(bodyText, _currentElapsedMeasurementTimeMS, TriggerVal::NA,
                   _timeClient->getAdjustedTimeSeconds(), millis(),
                   readings, _readingStore->getRecentTitles(readings),
                   _readingStore->getMostRecentPHReading());

        _server.send(200, "text/html", bodyText.c_str());
    }

    void handleTrigger() {
        String asOfString = _server.arg("asOf");
        unsigned long asOf = 0;

        if (asOfString == "") {
            Serial.println("Missing asOf!");
        } else {
            asOf = atol(_server.arg("asOf").c_str());
        }

        TriggerVal triggered = TriggerVal::FAIL;

        if (asOf > 0) {
            auto trigger = std::make_unique<TriggerRequest>();
            trigger->title = _server.arg("title").c_str();
            richiev::strings::trim(trigger->title);
            trigger->asOf = asOf;
            _pendingTrigger = std::move(trigger);

            triggered = TriggerVal::SUCCESS;
        }

        std::string bodyText;
        auto readings = _readingStore->getReadingsSortedByAsOf();
        renderRoot(bodyText, _currentElapsedMeasurementTimeMS, triggered,
                   _timeClient->getAdjustedTimeSeconds(), millis(),
                   readings, _readingStore->getRecentTitles(readings),
                   _readingStore->getMostRecentPHReading());
        _server.send(200, "text/html", bodyText.c_str());
    }

    void handleNotFound() {
        String message = "File Not Found\n\n";
        message += "URI: ";
        message += _server.uri();
        message += "\nMethod: ";
        message += (_server.method() == HTTP_GET) ? "GET" : "POST";
        message += "\nArguments: ";
        message += _server.args();
        message += "\n";

        for (uint8_t i = 0; i < _server.args(); i++) {
            message += " " + _server.argName(i) + ": " + _server.arg(i) + "\n";
        }

        _server.send(404, "text/plain", message);
    }

    void setupWebServer(std::shared_ptr<reading_store::ReadingStore> rs) {
        _readingStore = rs;

        _server.on("/", [&]() { handleRoot(); });
        _server.on("/execute/measure_alk", [&]() { handleTrigger(); });
        _server.onNotFound([&]() { handleNotFound(); });
        _server.begin();
        Serial.println("HTTP server started");
    }

    void loopWebServer(const unsigned long currentElapsedMeasurementTimeMS) {
        _currentElapsedMeasurementTimeMS = currentElapsedMeasurementTimeMS;
        _server.handleClient();
    }

    std::unique_ptr<TriggerRequest> retrievePendingFeedRequest() {
        if (_pendingTrigger) {
            auto feedReq = std::move(_pendingTrigger);
            _pendingTrigger = nullptr;
            return std::move(feedReq);
        }
        return nullptr;
    }
};

}  // namespace web_server
}  // namespace buff
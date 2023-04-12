#pragma once

#include <Arduino.h>
#include <WebServer.h>  // Built into ESP32

#include <string>

#include "alk-measure-common.h"
#include "reading-store.h"
#include "time-common.h"
#include "web-server-renderers.h"

namespace buff {
namespace web_server {

struct TriggerRequest {
    std::string title;
    unsigned long asOf;
};

std::unique_ptr<TriggerRequest> pendingTrigger;

template <size_t N, int PORT = 80>
class BuffWebServer {
   private:
    std::shared_ptr<reading_store::ReadingStore<N>> _readingStore = nullptr;
    std::shared_ptr<buff_time::TimeWrapper> _timeClient = nullptr;
    WebServer _server;

    unsigned long _currentElapsedMeasurementTime = 0;

   public:
    BuffWebServer(std::shared_ptr<buff_time::TimeWrapper> timeClient) : _server(PORT), _timeClient(timeClient) {}

    void handleRoot() {
        std::string bodyText;
        renderRoot(bodyText, _currentElapsedMeasurementTime, _server.arg("triggered").c_str(), _timeClient->getAdjustedTimeSeconds(), millis(), _readingStore->getReadingsSortedByAsOf());

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

        if (asOf > 0) {
            auto trigger = std::make_unique<TriggerRequest>();
            trigger->title = _server.arg("title").c_str();
            trigger->asOf = asOf;
            pendingTrigger = std::move(trigger);

            _server.sendHeader("Location", "/?triggered=true", true);
            _server.send(302, "text/plain", "triggered=true");
        } else {
            _server.sendHeader("Location", "/?triggered=false", true);
            _server.send(302, "text/plain", "triggered=false");
        }
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

    void setupWebServer(std::shared_ptr<reading_store::ReadingStore<N>> rs) {
        _readingStore = rs;

        _server.on("/", [&]() { handleRoot(); });
        _server.on("/execute/measure_alk", [&]() { handleTrigger(); });
        _server.onNotFound([&]() { handleNotFound(); });
        _server.begin();
        Serial.println("HTTP server started");
    }

    void loopWebServer(const unsigned long time) {
        _currentElapsedMeasurementTime = time;
        _server.handleClient();
    }

    std::unique_ptr<TriggerRequest> retrievePendingFeedRequest() {
        if (pendingTrigger) {
            auto feedReq = std::move(pendingTrigger);
            pendingTrigger = nullptr;
            return std::move(feedReq);
        }
        return nullptr;
    }
};

}  // namespace web_server
}  // namespace buff
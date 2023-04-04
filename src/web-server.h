#pragma once

#include <Arduino.h>
#include <WebServer.h>  // Built into ESP32

#include <list>
#include <string>

#include "alk-measure-common.h"
#include "web-server-renderers.h"

namespace buff {
namespace web_server {

const size_t MEASUREMENTS_TO_COUNT = 10;
std::list<alk_measure::AlkReading> mostRecentReadings;

WebServer server(80);

void addReading(const alk_measure::AlkReading &reading) {
    mostRecentReadings.push_front(reading);
    if (mostRecentReadings.size() == MEASUREMENTS_TO_COUNT - 1) {
        mostRecentReadings.pop_back();
    }
}

void handleRoot() {
    std::string bodyText;
    renderRoot(bodyText, mostRecentReadings);

    server.send(200, "text/html", bodyText.c_str());
}

void handleNotFound() {
    String message = "File Not Found\n\n";
    message += "URI: ";
    message += server.uri();
    message += "\nMethod: ";
    message += (server.method() == HTTP_GET) ? "GET" : "POST";
    message += "\nArguments: ";
    message += server.args();
    message += "\n";

    for (uint8_t i = 0; i < server.args(); i++) {
        message += " " + server.argName(i) + ": " + server.arg(i) + "\n";
    }

    server.send(404, "text/plain", message);
}

void setupWebServer() {
    // To enable chunking
    server.setContentLength(CONTENT_LENGTH_UNKNOWN);

    server.on("/", handleRoot);
    // server.on("/test.svg", drawGraph);
    // server.on("/inline", []() {
    //     server.send(200, "text/plain", "this works as well");
    // });
    server.onNotFound(handleNotFound);
    server.begin();
    Serial.println("HTTP server started");
}

void loopWebServer() {
    server.handleClient();
}

}  // namespace web_server
}  // namespace buff
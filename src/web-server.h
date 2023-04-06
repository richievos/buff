#pragma once

#include <Arduino.h>
#include <WebServer.h>  // Built into ESP32

#include <string>

#include "alk-measure-common.h"
#include "reading-store.h"
#include "web-server-renderers.h"

namespace buff {
namespace web_server {

template <size_t N, int PORT = 80>
class BuffWebServer {
   private:
    std::shared_ptr<reading_store::ReadingStore<N>> _readingStore = nullptr;
    WebServer _server;

   public:
    BuffWebServer() : _server(PORT) {}

    void handleRoot() {
        std::string bodyText;
        renderRoot(bodyText, _readingStore->getReadings());

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

    void setupWebServer(std::shared_ptr<reading_store::ReadingStore<N>> rs) {
        _readingStore = rs;

        _server.on("/", [&]() { handleRoot(); });
        // _server.on("/test.svg", drawGraph);
        // _server.on("/inline", []() {
        //     _server.send(200, "text/plain", "this works as well");
        // });
        _server.onNotFound([&]() { handleNotFound(); });
        _server.begin();
        Serial.println("HTTP server started");
    }

    void loopWebServer() {
        _server.handleClient();
    }
};

}  // namespace web_server
}  // namespace buff
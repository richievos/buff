#pragma once

#include <WiFi.h>
#include <ESPmDNS.h>

#include <string>

namespace richiev {

void connectWifi(const std::string hostname, const std::string wifiSSID, const std::string wifiPassword) {
    WiFi.config(INADDR_NONE, INADDR_NONE, INADDR_NONE, INADDR_NONE);
    WiFi.setHostname(hostname.c_str());
    WiFi.mode(WIFI_STA);
    WiFi.begin(wifiSSID.c_str(), wifiPassword.c_str());

    Serial.print("Connecting to WiFi ..");
    while (WiFi.status() != WL_CONNECTED) {
        Serial.print('.');
        delay(1000);
    }

    Serial.print("Connected ip=");
    Serial.print(WiFi.localIP());
    Serial.println();

    if (MDNS.begin(hostname.c_str())) {
        Serial.print("MDNS responder started with hostname=");
        Serial.println(hostname.c_str());
    }
}
}  // namespace richiev

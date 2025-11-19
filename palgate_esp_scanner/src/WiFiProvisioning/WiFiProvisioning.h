#ifndef WIFI_PROVISIONING_H
#define WIFI_PROVISIONING_H

#include <Arduino.h>
#include <WebServer.h>
#include <WiFi.h>
#include "WiFiCredsManager.h"

// Expose the WebServer object so main.cpp can call handleClient()
extern WebServer g_webServer;

// Flag for main loop (optional reboot)
extern bool g_should_reboot;


// Expose the AP starter so main.cpp can call it
void startAPMode();

// HTML page provider (used internally but OK to expose)
String htmlWiFiConfigPage();

// WiFi event callback (exposed to main.cpp)
void WiFiEventHandler(WiFiEvent_t event);


#endif // #ifndef WIFI_PROVISIONING_H


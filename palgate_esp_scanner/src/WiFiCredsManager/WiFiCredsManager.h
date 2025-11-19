#ifndef WIFI_CREDS_MANAGER_H
#define WIFI_CREDS_MANAGER_H

#include <Arduino.h>
#include <Preferences.h>

class WiFiCredsManager
{
public:
    bool load(String& ssid, String& pass);
    void save(const String& ssid, const String& pass);
    void clear();
};

#endif

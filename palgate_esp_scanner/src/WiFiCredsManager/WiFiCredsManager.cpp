#include "WiFiCredsManager.h"

bool WiFiCredsManager::load(String& ssid, String& pass)
{
    Preferences prefs;
    
    prefs.begin("wifi", true);  // read-only
    ssid = prefs.getString("ssid", "");
    pass = prefs.getString("pass", "");
    prefs.end();

    return (ssid.length() > 0 && pass.length() > 0);
}

void WiFiCredsManager::save(const String& ssid, const String& pass)
{
    Preferences prefs;

    prefs.begin("wifi", false); // read-write
    prefs.putString("ssid", ssid);
    prefs.putString("pass", pass);
    prefs.end();
}

void WiFiCredsManager::clear()
{
    Preferences prefs;

    prefs.begin("wifi", false);
    prefs.clear();
    prefs.end();
}

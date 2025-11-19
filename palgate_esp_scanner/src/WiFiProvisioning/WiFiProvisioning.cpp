#include "WiFiProvisioning.h"

// HTTP server instance
WebServer g_webServer(80);

// Reboot flag (used by main loop)
bool g_should_reboot = false;

// External reference to WiFiCredsManager instance
extern WiFiCredsManager wifi_creds;

// HTML page for WiFi configuration
String htmlWiFiConfigPage()
{
    return R"rawliteral(
        <html>
        <head>
            <meta name="viewport" content="width=device-width, initial-scale=1" />
            <style>
                body { font-family: Arial; padding: 20px; }
                input { width: 100%; padding: 12px; margin: 8px 0; }
                button { padding: 12px; width: 100%; background: #4CAF50; color: white; border: none; }
            </style>
        </head>
        <body>
            <h3>Configure WiFi</h3>
            <form action="/save" method="POST">
                <label>WiFi SSID:</label>
                <input name="ssid" length="32" required>
                <label>Password:</label>
                <input name="pass" length="64" type="password" required>
                <button type="submit">Save & Restart</button>
            </form>
        </body>
        </html>
    )rawliteral";
}

// Start AP mode + define the routes
void startAPMode()
{
    Serial.println("Starting AP Mode for WiFi configuration...");

    WiFi.mode(WIFI_AP);
    WiFi.softAP("ESP_Wifi", "12345678");  

    Serial.print("AP IP: ");
    Serial.println(WiFi.softAPIP());

    // Route for main page
    g_webServer.on("/", HTTP_GET, []() {
        g_webServer.send(200, "text/html", htmlWiFiConfigPage());
    });

    // Route for saving WiFi credentials
    g_webServer.on("/save", HTTP_POST, []() {
        String ssid = g_webServer.arg("ssid");
        String pass = g_webServer.arg("pass");

        wifi_creds.save(ssid, pass);
        g_webServer.send(200, "text/plain", "Saved. Rebooting...");
        g_should_reboot = true;
    });

	// Route for clearing saved WiFi credentials
	g_webServer.on("/reset", HTTP_GET, [](){

		wifi_creds.clear();
		g_webServer.send(200, "text/plain", "WiFi cleared. Rebooting...");
		g_should_reboot = true;
	});

    g_webServer.begin();
    Serial.println("AP WebServer started.");
}



/**
 * WiFiEvent Event Handler (callback)
 * Called automatically when any WiFi-related event occurs.
 */

void WiFiEventHandler(WiFiEvent_t event)
{
    switch (event)
    {
        // Connected to the access point (no IP yet)
        case SYSTEM_EVENT_STA_CONNECTED:
    		Serial.println("Connected to WiFi. Waiting for IP...");
            break;

        // Received IP address from the router (DHCP)
        case SYSTEM_EVENT_STA_GOT_IP:
            Serial.print("Got IP: ");
            Serial.println(WiFi.localIP());
            break;

        // Lost connection to the access point
        case SYSTEM_EVENT_STA_DISCONNECTED:
            Serial.println("WiFi disconnected, reconnecting...");
            WiFi.reconnect();
            break;

		case SYSTEM_EVENT_WIFI_READY:
		break;

		case SYSTEM_EVENT_STA_START:
		break;

        // Any other unhandled WiFi event
        default:
            Serial.print("Unhandled WiFi event: ");
            Serial.println(event);
            break;
    }
}
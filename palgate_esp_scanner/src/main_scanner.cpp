#include <Arduino.h>				// Core Arduino/ESP32 APIs used here (Serial, pinMode, digitalWrite, delay).
#include <BLEDevice.h>            	// BLEDevice for BLE init and scan object (BLEDevice::init, BLEDevice::getScan).
#include <BLEUtils.h>             	// BLE utilities (kept for potential UUID/helpers; e.g. BLEUUID if used).
#include <BLEScan.h>              	// BLEScan API used: BLEScan class, setInterval(), setWindow(), start(), stop(), clearResults().
#include <BLEAdvertisedDevice.h>  	// BLEAdvertisedDevice: getManufacturerData(), getRSSI(), getAddress().
#include <atomic>                 	// std::atomic types used for cross-task flags/timestamps (std::atomic_bool, std::atomic<uint64_t>).
#include <cstring>                	// C string helpers used: std::memcpy(), std::strncpy().
#include "esp_sleep.h"            	// Light-sleep helpers: esp_sleep_enable_timer_wakeup(), esp_light_sleep_start().
#include "esp_timer.h"            	// RTC-backed timer: esp_timer_get_time() used to timestamp sightings (microseconds).
#include <WiFi.h> 				  	// ESP32 WiFi STA/AP control, connection handling, events.
#include <HTTPClient.h>				// HTTP client for sending REST API requests.	
#include <WiFiClientSecure.h>		// HTTP client for sending REST API calls (used for PalGate).
#include <time.h>					// NTP time functions for syncing time — needed for token timestamps.
#include <Preferences.h>			// NVS key-value storage — persists WiFi SSID/password across reboots.
#include <WebServer.h>				// Lightweight HTTP server — serves the WiFi configuration portal.

#include "token_generator.h"		// Generates PalGate x-bt-token using session key + timestamp.
#include "WiFiCredsManager.h"		// Loads/saves WiFi credentials from NVS.
#include "WiFiProvisioning.h"		// Handles AP mode + webform for entering new WiFi settings.
#include "config.h"					

#define LED_PIN 2


//===========================================================
// structs, enum declerations
//===========================================================

/**
 * @brief Holds parsed iBeacon fields extracted from BLE manufacturer data.
 *
 * Stores UUID, major/minor identifiers, Tx power, RSSI and the sender's MAC
 * address as a C-string. Used to pass the decoded beacon information from the
 * scan callback into the main application logic.
 */
struct BeaconInfo 
{
  uint8_t uuid[16];
  uint16_t major;
  uint16_t minor;
  int8_t txPower;
  int rssi;
  char addrStr[18]; // e.g. "AA:BB:CC:DD:EE:FF\0"
};



//===========================================================
// global variables 
//===========================================================
static bool g_led_on = false; 							// Tracks whether the gate-indicator LED is currently lit.
static bool g_is_time_synced_ok = false; 				// True once NTP time sync succeeded (required for valid PalGate tokens).
static BeaconInfo g_lastBeacon;							// Stores the most recently parsed iBeacon packet.
BLEScan* g_manage_scan = nullptr;						// BLE scan manager pointer created by BLEDevice::getScan().
static const uint32_t SCAN_AWAKE_MS = 200;   			// Scans for 200ms
static const uint32_t SLEEP_MS      = 2800;  			// Sleeps for 2800ms 
static const uint16_t SCAN_INTERVAL_MS = SCAN_AWAKE_MS; // 200ms
static const uint16_t SCAN_WINDOW_MS   = SCAN_AWAKE_MS; // 200ms window scan
static const uint16_t SCAN_FAKE_DURATION_SEC = 3; 		// Dummy duration for BLEScan.start()
static unsigned long g_last_trigger_ms = 0;				// Timestamp (ms) of the last successful detection-trigger (for debounce logic).
static const unsigned long DEBOUNCE_MS = 10000; 		// ignore detections within 6s
static const unsigned long g_LED_ON_MS = 3000;  		// LED off after this ms without sightings
static const unsigned long LED_ON_US = 3000 * 1000ULL;  // LED on duration in microseconds

// WiFi credentials manager
WiFiCredsManager wifi_creds;
String g_ssid = "";
String g_pass = "";

// User-defined 16-byte UUID — scanner triggers only on beacons matching this value.
static const uint8_t target_uuid[16] = 
{
  0xBC,0x9A,0x78,0x56,0x34,0x12,0x34,0x12,
  0x34,0x12,0x34,0x12,0x78,0x56,0x34,0x12
};

// Apple iBeacon prefix packet / format
static const uint8_t IBEACON_PREFIX[4] = 
{
  0x4C, 0x00, // Apple Company Identifier (0x004C)
  0x02,       // Data type: iBeacon
  0x15        // Length of the remaining payload (21 bytes)
};

// Use an atomic flag for cross-task signalling between BLE callbacks and loop()
static std::atomic_bool m_detected(false);

// Prevent reentrant TriggerGate calls (if loop() triggers while a previous HTTP is in flight)
static std::atomic_bool g_trigger_in_progress(false);

// last time we saw the beacon (updated from callback) in microseconds.
// Use atomic because callback runs on another task.
static std::atomic<uint64_t> g_last_time_gate_opend_us(0);



//===========================================================
// helper function declarations 
//===========================================================
static void printHex(const std::string& s);
static inline void lightSleepMs(uint32_t ms);
static void TriggerGate();
static bool parseIBeacon(const std::string& mfg, BeaconInfo &out);
static void HandleLed();
static bool syncTimeOnce();



//===========================================================
// helper classes defenitions
//===========================================================

/**
 * @brief Handles BLE advertisements during scanning.
 *
 * Parses manufacturer data, checks if the packet matches the target iBeacon,
 * and sets the atomic detection flag so loop() can trigger the gate.
 */
class ScanCallbacks : public BLEAdvertisedDeviceCallbacks
{
	void onResult(BLEAdvertisedDevice dev) override
	{
		if (dev.haveManufacturerData())
		{
			const std::string& mfg = dev.getManufacturerData();
			BeaconInfo info;

			if (parseIBeacon(mfg, info))
			{
				info.rssi = dev.getRSSI();
				std::string addr = dev.getAddress().toString();

				// copy address to C-string
				std::strncpy(info.addrStr, addr.c_str(), sizeof(info.addrStr)-1);
				info.addrStr[sizeof(info.addrStr)-1] = '\0';

				// store into g_lastBeacon (POD copy)
				std::memcpy(&g_lastBeacon, &info, sizeof(BeaconInfo));

						// set atomic flag to notify loop() a target beacon was seen
						// but avoid re-triggering if we recently triggered (debounce)
						unsigned long now_ms = millis();
						if (now_ms - g_last_trigger_ms > DEBOUNCE_MS) 
						{
							// record that we detected now so other callbacks within debounce window won't re-arm
							m_detected.store(true);
						}

			}
			else
			{
				// Debug: print raw manufacturer-data for devices that don't match
				// Serial.printf("Seen manufacturer data from %s RSSI=%d len=%u: ", dev.getAddress().toString().c_str(), dev.getRSSI(), (unsigned)mfg.size());
				// const uint8_t* pb = reinterpret_cast<const uint8_t*>(mfg.data());
				// for (size_t i = 0; i < mfg.size(); ++i) Serial.printf("%02X ", pb[i]);
				// Serial.println();
			}

		}
	}
};



/**
 * @brief Lightweight non-blocking delay helper class the loops.
 *
 * Provides wait(duration_ms) which returns false while the delay
 * is still active, and true once the specified time has elapsed.
 * No blocking - allows loop() to stay responsive while
 * timing operations are spaced apart.
 */
class DelayNonBlocking
{
public:
    // Begins (or continues) a non-blocking delay window.
    // Returns true AFTER the delay has completed.
    // Returns false WHILE waiting.
    bool wait(unsigned long duration_ms)
    {
		// start delay for the first time
        if (!active)
        {
            active = true;
            start_time = millis();
            return false; 
        }

		// check if delay time has elapsed
        if ((millis() - start_time) < duration_ms)
        {
            return false; // still within delay window
        }

        // delay finished
        active = false;

        return true;
    }

private:
    unsigned long start_time = 0;
    bool active = false;
};


// non-blocking delays
DelayNonBlocking g_scan_delay;    
DelayNonBlocking g_loop_delay;  



//===========================================================
// setup
//===========================================================
void setup() 
{
	// ---- FACTORY RESET (WiFi erase) using BOOT button ----
	pinMode(0, INPUT_PULLUP);  // BOOT = GPIO0
	Serial.begin(115200);
	delay(100); // wait for Serial to initialize

	if (digitalRead(0) == LOW)
	{
		Serial.println("BOOT button held on startup — clearing saved WiFi credentials...");
		wifi_creds.clear();
		Serial.println("WiFi credentials cleared. Rebooting...");
		delay(1000);
		ESP.restart();
		return; // should not reach here
	}
	// ---- end of FACTORY RESET ----

	Serial.println("ESP32 Scanner ready.  Connecting to WiFi...");

	// Try loading saved WiFi credentials
	if (false == wifi_creds.load(g_ssid, g_pass))
	{
		Serial.println("No saved WiFi credentials found!");
		startAPMode(); // start AP mode for configuration
		return; // skip rest of setup — wait for user to configure WiFi
	}
	else // credentials found and loaded
	{
		Serial.printf("Loaded WiFi config: SSID=%s\n", g_ssid.c_str());

		WiFi.mode(WIFI_STA);

		// Handle WiFi events
        WiFi.onEvent(WiFiEventHandler);

		WiFi.begin(g_ssid.c_str(), g_pass.c_str());
		Serial.println("Connecting to WiFi...");
	}


	// Enable WiFi sleep to save power between scans
	WiFi.setSleep(true);

	// NTP sync (only if not in AP mode)
	if (WiFi.getMode() == WIFI_STA)
	{
		// Sync time using NTP (Network Time Protocol) to ensure the device has an accurate clock required for generating valid PalGate token timestamps.
		g_is_time_synced_ok = syncTimeOnce();
		if (false == g_is_time_synced_ok)
		{
			Serial.println("NTP sync failed — token timestamps may be invalid!");
			// If we want to halt the system completely in this case:
			// while (true) { delay(1000); }
		}
		else
		{
			Serial.println("NTP time sync OK.");
		}
	}


	pinMode(LED_PIN, OUTPUT);
	digitalWrite(LED_PIN, LOW);

	Serial.println("Looking for iBeacons...");

	BLEDevice::init("");
	g_manage_scan = BLEDevice::getScan();
	g_manage_scan->setAdvertisedDeviceCallbacks(new ScanCallbacks(), /*wantDuplicates=*/true);
	g_manage_scan->setActiveScan(true); // not only scan for names, request more data (like manufacturer data)

	g_manage_scan->setInterval(SCAN_INTERVAL_MS); // e.g. 200 ms
	g_manage_scan->setWindow(SCAN_WINDOW_MS);     // e.g. 200 ms

}


//===========================================================
// loop
//===========================================================
void loop() 
{
	// If in AP mode → handle webserver
	if (WiFi.getMode() == WIFI_AP)
	{
		g_webServer.handleClient();
		if (g_should_reboot)
        {
            delay(500);       
            ESP.restart();
        }

		return;  // do not run scanning when in AP mode
	}

	static bool is_scan_running = false;

	// If no scan is running, start a new scan (non-blocking)
    if (false == is_scan_running)
    {
        g_manage_scan->start(SCAN_FAKE_DURATION_SEC, /*is_continue=*/true);
        is_scan_running = true;

        // reset non-blocking delay timer so wait(80) starts "now"
        g_scan_delay.wait(0); 
    }


	// start(durationSeconds) is blocking unless is_continue=true. therefore:
	// start() is non-blocking because 'is_continue=true' returns immediately; 
	// the scan runs in the background and we manually stop it after a short delay.

	// Wait for 80ms before stopping the scan
    if (false == g_scan_delay.wait(80))
    {
        return; // still waiting — comes back next loop cycle
    }

	// After 80ms → stop scan and process results
    g_manage_scan->stop();            // manually stop BLE scan
    g_manage_scan->clearResults();    // clear internal buffer
    is_scan_running = false;          // allow next scan to start next iteration

	// Beacon detected - Trigger logic (If callback set the flag)
	if (m_detected.load() == true)
	{
		unsigned long now = millis();

		// Handle Debounce: ignore triggers that happen too close together
		// OR trigger if outside debounce window

		unsigned long time_passed_since_last_trigger = now - g_last_trigger_ms;
		if (time_passed_since_last_trigger > DEBOUNCE_MS)
		{
			m_detected.store(false);
			g_last_trigger_ms = now;

			// Log beacon info

			// Serial.printf("Detected iBeacon from %s RSSI=%d UUID=", g_lastBeacon.addrStr, g_lastBeacon.rssi);
			// for (int i = 0; i < 16; ++i)
			//   Serial.printf("%02X", g_lastBeacon.uuid[i]);
			// Serial.printf(" major=%u minor=%u tx=%d\n", (unsigned)g_lastBeacon.major, (unsigned)g_lastBeacon.minor, g_lastBeacon.txPower);
			
			if (false == g_is_time_synced_ok)
			{
				Serial.println("Time not synced; skipping TriggerGate()");
				m_detected.store(false);
			}
			else
			{
				TriggerGate();
			}


		} // if (now - g_last_trigger_ms > DEBOUNCE_MS)
		else
		{
      		// Clear the flag if within debounce window but leave g_last_trigger_ms unchanged
      		m_detected.store(false);
		}
	}

  	// Keep LED on while we have seen the beacon recently. 
	// Turn off after LED_ON_MS without new sightings.
  	HandleLed();

	// flush Serial before sleep to avoid truncating logs
	Serial.flush();

	// sleeps 2.8 seconds and returns to next cycle
	if (false == g_loop_delay.wait(200))
	{
		return; // waiting non-blocking
	}

	// after 200ms - sleep
	lightSleepMs(SLEEP_MS);

} // end of loop()




//===========================================================
// helper function defenitions 
//===========================================================

/**
 * @brief Print a byte buffer as hex for debug visibility.
 */
static void printHex(const std::string& s) 
{
  Serial.print("Manufacturer Data (hex): ");
  for (size_t i = 0; i < s.size(); ++i) {
    Serial.printf("%02X ", static_cast<uint8_t>(s[i]));
  }
  Serial.println();
}



/**
 * @brief Enter ESP32 light-sleep mode for the specified duration (ms).
 *        Wakes up via timer. Used to reduce power between BLE scans.
 */
static inline void lightSleepMs(uint32_t ms)
{
  // set wakeup timer in microseconds
  esp_sleep_enable_timer_wakeup((uint64_t)ms * 1000ULL);

  // start light sleep and wait for timer (keeps RAM, wakes up fast)
  esp_light_sleep_start();
}



// Parse iBeacon manufacturer data. 
/**
 * @brief Parse manufacturer-data payload and extract iBeacon fields.
 *        If it matches target_uuid, fills out `out` and returns true.
 */
static bool parseIBeacon(const std::string& mfg, BeaconInfo &out)
{
	// iBeacon Manufacturer Data layout:
	// [0..1]  Company ID (0x004C, little-endian -> 4C 00)
	// [2]     Type (0x02)
	// [3]     Length (0x15)
	// [4..19] UUID (16B)
	// [20..21] Major (big-endian)
	// [22..23] Minor (big-endian)
	// [24]    Measured Power (signed byte)

	if (mfg.size() < 25) 
		return false;

	const uint8_t* b = reinterpret_cast<const uint8_t*>(mfg.data());

	// Accept Apple company ibeacon id in either endianess byte order (0x4C,0x00 or 0x00,0x4C)
	// checks all packets for valid iBeacon prefix
	bool company_ok = ((b[0] == IBEACON_PREFIX[0] && b[1] == IBEACON_PREFIX[1]) ||
								(b[0] == IBEACON_PREFIX[1] && b[1] == IBEACON_PREFIX[0]));

	if (!company_ok || b[2] != IBEACON_PREFIX[2] || b[3] != IBEACON_PREFIX[3]) 
	{
		return false;
	}

  // Try direct UUID match first (advertised bytes in expected order)
	bool is_matched_uuid = true;
	for (int i = 0; i < 16; ++i) 
	{
		if (b[4 + i] != target_uuid[i]) 
		{ 
			is_matched_uuid = false; 
			break; 
		}
	}

	if (is_matched_uuid == true) 
	{
		for (int i = 0; i < 16; ++i) 
			out.uuid[i] = b[4 + i]; // copy as-is
	} 
	else 
	{
		// Try reversed UUID (advertiser may send UUID bytes in reverse order)
		bool is_matched_uuid_reversed = true;
		for (int i = 0; i < 16; ++i) 
		{
			if (b[4 + i] != target_uuid[15 - i]) 
			{ 
			is_matched_uuid_reversed = false; 
			break; 
			}
		}

		if (is_matched_uuid_reversed == false) 
			return false;

		// Normalize into out.uuid in the canonical target_uuid order
		for (int i = 0; i < 16; ++i) 
			out.uuid[i] = target_uuid[i]; // copy reversed order
	}

	// Parse major, minor, txPower to real numbers and save
	out.major = (uint16_t(b[20]) << 8) | uint16_t(b[21]);
	out.minor = (uint16_t(b[22]) << 8) | uint16_t(b[23]);
	out.txPower = static_cast<int8_t>(b[24]);

	return true;
}



/**
 * @brief Perform the HTTP request to open the gate.
 *        Includes token generation, TLS request, LED indication,
 *        and protection against re-entrant calls.
 */
static void TriggerGate()
{
	// prevent re-entrancy: if a TriggerGate is already running, skip this one
	bool expected = false;
	if (!g_trigger_in_progress.compare_exchange_strong(expected, true)) {
		Serial.println("TriggerGate already in progress, skipping duplicate call.");
		return;
	}

	// RAII guard: ensure g_trigger_in_progress is cleared on every exit path
	struct Guard 
	{
		std::atomic_bool &g;
		Guard(std::atomic_bool &g_) : g(g_) {}
		~Guard() { g.store(false); }
	};
	Guard guard(g_trigger_in_progress);

	Serial.println("Triggering gate open action...");

	if (WiFi.status() != WL_CONNECTED) {
		Serial.println("WiFi disconnected, cannot send request.");
		return;
	}

	// secure client for HTTPS
	WiFiClientSecure client;
	client.setInsecure(); // development only — replace with CA cert in production
	HTTPClient http;
	uint8_t session[16];


	/********** Handle Palgate request **********/
	// Make sure you read the README before running 

	// Convert hex string to binary
	if (!hexStringToBytes(PALGATE_SESSION_TOKEN, session, 16))
	{
		Serial.println("Invalid session token format!");
		return;
	}

	// Generate time-based token
		uint32_t ts = static_cast<uint32_t>(time(nullptr));
		std::string token = generateToken(session, PALGATE_PHONE_NUMBER, PALGATE_TOKEN_TYPE, ts);

	// Construct URL
	String url = "https://api1.pal-es.com/v1/bt/device/4G600106591/open-gate?outputNum=1";

	// Make HTTP GET request with headers using secure client
	Serial.printf("Starting http.begin()...\n");
	uint64_t t0 = esp_timer_get_time();
	if (!http.begin(client, url)) {
		Serial.println("http.begin failed");
		return;
	}
	uint64_t t1 = esp_timer_get_time();
	Serial.printf("http.begin took %llu ms\n", (unsigned long long)((t1 - t0) / 1000ULL));

		// Send only the x-bt-token header (matching the working curl script)
		http.addHeader("x-bt-token", token.c_str());

		// Send the request
		Serial.println("About to call http.GET()...");
		uint64_t t2 = esp_timer_get_time();
		int httpCode = http.GET();
		uint64_t t3 = esp_timer_get_time();
		Serial.printf("http.GET() took %llu ms\n", (unsigned long long)((t3 - t2) / 1000ULL));
	if (httpCode > 0)
	{
		String payload = http.getString();
		Serial.printf("Response [%d]: %s\n", httpCode, payload.c_str());

		// Only treat 2xx as success — light LED and refresh last-seen timestamp
		if (httpCode >= 200 && httpCode < 300)
		{
			g_last_time_gate_opend_us.store(esp_timer_get_time());
			digitalWrite(LED_PIN, HIGH);
			g_led_on = true;
			Serial.println("Gate opened (HTTP success). LED ON.");
		}
		else
		{
			Serial.println("Gate open request returned non-success code; not lighting LED.");
		}
	}
	else
	{
		Serial.printf("HTTP request failed: %s\n", http.errorToString(httpCode).c_str());
	}

	http.end();
	// release re-entrancy guard
	g_trigger_in_progress.store(false);
}



/**
 * @brief Manage LED state based on last successful gate trigger.
 *        Turns LED ON for a fixed window and then OFF automatically.
 */
static void HandleLed()
{
	
	uint64_t now_us = esp_timer_get_time();
  	uint64_t last_time_opened = g_last_time_gate_opend_us.load();
  	uint64_t time_passed_since_last_opened = now_us - last_time_opened;

	if (last_time_opened != 0 && (time_passed_since_last_opened < (uint64_t)LED_ON_US)) 
	{
		if (false == g_led_on) 
		{
			digitalWrite(LED_PIN, HIGH);
			g_led_on = true;
		}
	} 
  	else 
	{
   	if (g_led_on) 
		{
      	digitalWrite(LED_PIN, LOW);
      	g_led_on = false;
    	}
	}
}



 /**
 * @brief Perform one-shot NTP sync to obtain a valid epoch time.
 *        Required for time-based PalGate token generation.
 * 		  Returns true if time was set successfully, false otherwise.
 */
static bool syncTimeOnce()
{
	configTime(0, 0, "pool.ntp.org", "time.google.com");

	for (int i = 0; i < 10; ++i)
	{
		time_t now = time(nullptr);

		if (now > 1000000000)
		{
			Serial.println("Time synchronized successfully via NTP.");
			return true;
		}

		delay(500); 
		yield();    
	}

	Serial.println("Failed to synchronize time via NTP.");
	return false;
}







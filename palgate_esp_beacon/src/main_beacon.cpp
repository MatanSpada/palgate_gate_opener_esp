/*******************************************
  Main file for ESP32-based BLE iBeacon transmitter
  - Press button connected to GPIO0 to start beaconing for 10 seconds
  - Onboard LED (GPIO2) lights up while beaconing

  Uses the ArduinoBLE library:
/*******************************************/

#include <Arduino.h>                // Serial, pinMode, digitalWrite, delay, millis
#include <BLEDevice.h>              // BLEDevice API: BLEDevice::init(), BLEDevice::createServer(), BLEDevice::getAdvertising()
#include <BLEUtils.h>               // BLE and UUID (BLEUUID used to parse/construct UUIDs)
#include <BLEBeacon.h>              // BLEBeacon type and helpers: BLEBeacon, setManufacturerId, setProximityUUID, setMajor, setMinor, setSignalPower, getData

// Additional BLE classes used in this file (provided by the BLE Arduino library headers above):
// BLEAdvertisementData (setFlags, setManufacturerData), BLEAdvertising (setAdvertisementData, setMinInterval, setMaxInterval, start, stop)

#define BUTTON_PIN 0                // GPIO used for the physical button input.
#define LED_PIN    2                // GPIO used for status LED indication.
#define BEACON_DURATION_MS 10000    // Duration (in milliseconds) to broadcast BLE beacon after button press.
#define DEVICE_NAME "ESP32_BEACON"  // BLE device name shown during advertising.



//===========================================================
// global & static variables 
//===========================================================

// iBeacon identifiers (UUID, Major, Minor, Tx Power)
static const char* beacon_uuid = "bc9a7856-3412-3412-3412-341278563412";
static const uint16_t beacon_major = 1;             // Major identifier for iBeacon grouping
static const uint16_t beacon_minor = 1;             // Minor identifier for iBeacon grouping
static const int8_t  beacon_txpower_at_1m = -59;    // Calibrated RSSI measured at 1 meter

// BLE advertising object + runtime state
BLEAdvertising* g_pAdvertising = nullptr;           // Handle to BLE advertising instance
bool g_is_beacon_active = false;                    // Indicates whether the beacon is currently active
uint32_t g_beacon_start_time = 0;                   // Timestamp (ms) when beacon transmission started

// Button debounce management
int g_prev_button_state = HIGH;                     // Previous sampled button state
uint32_t g_last_debounce_ms = 0;                    // Timestamp of last debounce event
const uint16_t g_debounce_limit_ms = 30;            // Debounce time threshold (ms)



//===========================================================
// helper function declarations 
//===========================================================
static void setupBeacon();
static void startBeacon();
static void stopBeacon();



//===========================================================
// setup
//===========================================================
void setup() 
{
  Serial.begin(115200);
  Serial.println("ESP32 ready");

  pinMode(LED_PIN, OUTPUT);
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  digitalWrite(LED_PIN, LOW);

  setupBeacon();
}



//===========================================================
// loop
//===========================================================
void loop() 
{
  // Read current button state and current time
  int current_button_state = digitalRead(BUTTON_PIN);
  uint32_t now = millis();

  // Debounce: detect state change
  if (current_button_state != g_prev_button_state) 
	{
    g_last_debounce_ms = now;
    g_prev_button_state = current_button_state;
  }

  // Stable state check after debounce threshold has passed
  static bool last_stable_high = true;                      // Tracks last stable button state (HIGH = not pressed)
  if ((now - g_last_debounce_ms) > g_debounce_limit_ms)     // if we reached the debounce time limit
	{
    // Detect a *new* press
    if (current_button_state == LOW && last_stable_high && !g_is_beacon_active) 
		{
      digitalWrite(LED_PIN, HIGH);
      startBeacon();
      last_stable_high = false;
    }

    // Button released, update stable state
    if (current_button_state == HIGH) 
		{
      last_stable_high = true;
    }
  }

  // stop beacon after beacon duration
  if (g_is_beacon_active && (now - g_beacon_start_time >= BEACON_DURATION_MS)) 
	{
    stopBeacon();
    digitalWrite(LED_PIN, LOW);
  }
}




//===========================================================
// helper function defenitions 
//===========================================================

static void setupBeacon() 
{
  BLEDevice::init(DEVICE_NAME);
  if (false == BLEDevice::getInitialized()) 
  {
    Serial.println("BLE init failed!");
  }


  BLEServer* pServer = BLEDevice::createServer(); 
	(void)pServer;

  BLEBeacon beacon;
  beacon.setManufacturerId(0x004C);               // iBeacon packet / format
  beacon.setProximityUUID(BLEUUID(beacon_uuid));
  beacon.setMajor(beacon_major);
  beacon.setMinor(beacon_minor);
  beacon.setSignalPower(beacon_txpower_at_1m);

  BLEAdvertisementData advData;
  advData.setFlags(0x04);                         // BLE-only
  advData.setManufacturerData(beacon.getData());

  g_pAdvertising = BLEDevice::getAdvertising();
  g_pAdvertising->setAdvertisementData(advData);

  g_pAdvertising->setMinInterval(0x20);             // 0x20 * 0.625ms = 20ms
  g_pAdvertising->setMaxInterval(0x40);             // 0x40 * 0.625ms = 40ms
}

static void startBeacon() 
{
  if (!g_pAdvertising) 
		return;

  g_pAdvertising->start();
  g_is_beacon_active = true;
  g_beacon_start_time = millis();                     // catch current time

  Serial.println("Beacon started");
}

static void stopBeacon() 
{
  if (!g_pAdvertising) 
		return;

  g_pAdvertising->stop();
  g_is_beacon_active = false;

  Serial.println("Beacon stopped");
}
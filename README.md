<p align="center">
  <img src="docs/logo.png" alt="PalGate Logo" width="160">
</p>

<h3 align="center">PalGate Gate Opener</h3>

<p align="center">
  An easy way to open your gate.
  <br>
  BLE-triggered automation using ESP32.
</p>

---

## Table of contents
- [What's included](#whats-included)
- [Quick start](#quick-start)
- [Purpose](#purpose)
- [Environment](#environment)
- [Battery runtime estimates](#battery-runtime-estimates)
- [How to extend battery life](#how-to-extend-battery-life)
- [Copyright and license](#copyright-and-license)

---

## What's included
This repository contains two independent PlatformIO projects:

- **/scanner** – ESP32 BLE scanner  
  Detects the user's beacon and triggers the PalGate API via HTTPS.

- **/beacon** – ESP32 BLE iBeacon transmitter  
  Runs on low power and advertises when the user approaches the gate.

Both projects include:
- Clean Arduino-based C++ code  
- Non-blocking BLE scanning  
- Light-sleep integration  
- Configurable UUID, advertising rate, and scan window  
- Optional Wi-Fi reconnection logic

---

## Quick start

1. Clone the repository:
   ```bash
   git clone git@github.com:MatanSpada/palgate_gate_opener_esp.git
2. Open either project (scanner or beacon) in PlatformIO.
3. Update your Wi-Fi credentials:
- Stored in Preferences (non-volatile)
- Set via CLI or initial configuration
4. Add your session token:
   static const char* SESSION_TOKEN_HEX = "<your-token-here>";
5. Upload to ESP32 and power the device.

## Purpose
Parking lots often have zero cellular reception, preventing the PalGate app from working.
This project solves it by using two ESP32 units (beacon + scanner) to open the gate reliably even when:
- There is no cellular coverage
- The phone cannot send API requests
- You want a local, offline-capable automation
The scanner detects your beacon and triggers an authenticated request to the PalGate server.

## Environment
- Board: ESP32 (ESP-WROOM-32)
- Framework: PlatformIO + Arduino
- Communication: BLE + Wi-Fi
- Sleep mode: Light Sleep / Deep Sleep (configurable)
- Folders:
/scanner     BLE scanner
/beacon      BLE beacon transmitter

## Battery runtime
Actual runtime depends on:
- Scan window
- Advertising interval
- Wi-Fi reconnect time
- Battery capacity

## How to extend battery life
- Reduce BLE advertising frequency (beacon)
- Lower scan window (SCAN_AWAKE_MS vs SLEEP_MS)
- Use Deep Sleep for the beacon when possible
- Increase Wi-Fi reconnect timeout
- Disable Serial debug logs in production
- Use high-quality Li-ion cells (≥3000mAh)

## Copyright and license
Code released under the MIT License.
Feel free to modify, improve, or use in your own automation projects


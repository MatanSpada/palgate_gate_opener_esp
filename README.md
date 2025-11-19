# 🚪 PALGATE BLE Automation (ESP32)

This repository contains two separate PlatformIO projects - each representing a distinct ESP32-based role in the BLE automation system designed to open parking gates with partial internet connectivity.

## 🗂️ Project Structure:

There are two folders in this repository:

/scanner   
The BLE scanner + Wi-Fi client.
It continuously scans for the beacon and, once detected, connects to Wi-Fi and sends an authenticated HTTP request to the PalGate API.

/beacon   
The BLE beacon transmitter.
It broadcasts an iBeacon advertisement when the user presses a button.

Each folder is an independent PlatformIO project with its own platformio.ini, source files and dependencies.

## ⚙️ Development Environment:
- **Framework**: PlatformIO
- **Board:** ESP32 (tested on ESP-WROOM-32)
- **Language:** C++ (Arduino framework)


## 🎯 Project Goal:
Many underground or closed parking lots have no cellular coverage, which prevents the PalGate mobile app from opening the entry gate.

This project tries to solve that by using two ESP32 units communicating via BLE:
One unit acts as a beacon, installed inside the car.

Another unit acts as a scanner, placed near the gate in an area with Wi-Fi/Cellular coverage.
When the scanner detects the beacon, it sends a cloud-based HTTP request to PalGate — effectively replacing the mobile app.


## 🔄 System Flow Overview:
1. **The beacon unit** broadcasts a BLE advertisement when the user presses the button.
2. **The scanner unit** continuously scans for nearby BLE devices.

3. When the **scanner** identifies the beacon’s advertisement:
- It turns on an LED for feedback.
- It connects to Wi-Fi.
- It loads PalGate credentials from config.h.
- It sends the authenticated HTTP request to open the gate.

4. The scanner may then enter light sleep to save power.


## 🔋 Energy Efficiency:
- Both ESP32 units are optimized for **low power**.
- The **beacon** advertises periodically with long intervals.
- The **scanner** sleeps between scan cycles, extending battery life significantly.
The LED operates only for short bursts when detection occurs.


## ⚡ Power Consumption Summary:
BLE Scan (active): ~80 mA for 200 ms every 2 seconds → 3.2 mA average.
light Sleep (idle): ~0.15 mA → 0.14 mA average.
LED (trigger): ~10 mA for 3 seconds × 4 times/day → 0.03 mA average.
Total average: around 3.4 mA.


## 🔋 Battery runtime estimates:   
3×AA (2000 mAh) → about 11 days   
4×AA (2000 mAh) → about 15 days   
18650 (3000 mAh) → about 18 days   
2×18650 (3000 mAh each, parallel) → about 35–36 days   


## 🕒 How to Extend Battery Life:
- Increase scan interval (e.g., from 3s to 5-10s)
- Reduce scan window (e.g., from 200ms to 100ms)
- Reduce beacon advertising frequency
- Use lithium or LiFePO4 batteries


## 📡 BLE Communication Details:
The beacon uses the iBeacon standard (Apple):   
Bytes 0-1: Company ID (0x004C)   
Byte 2: Data Type (0x02)   
Byte 3: Data Length (0x15)   
Bytes 4-19: UUID   
Bytes 20-21: Major   
Bytes 22-23: Minor   
Byte 24: TX Power   

The scanner parses manufacturer data and compares it to TARGET_UUID.   
When matched, it triggers the placeholder action.   
You may change the UUID, but make sure both projects use the same one.


## 🚘 Real-World Example:

You approach the parking gate.

- You press the button on the beacon ESP32 in your car and it starts transmits BLE advertisement.
- The scanner ESP32 installed near the gate receives it.
- The scanner should be placed in a location with internet access so it can send the POST request, while still remaining within BLE range of the beacon.
- It recognizes the UUID and triggers an HTTP request.
- The gate opens instantly - no app, no internet required for the driver.

## 🧾 Summary:

Beacon: Transmitter, broadcasts BLE signal.   
Scanner: Receiver, detects beacon and triggers gate.   
BLE: Local short-range communication.    
Power: light sleep and optimized timing.   
Trigger: Placeholder, future PalGate API integration.   

## 💡 Final Notes:

This project offers a practical, offline, low-cost solution to automate gates without network dependency.
It can also be adapted for smart home triggers, garage doors, or any local IoT automation.

## 📜 License:

MIT License - free to use, modify, and distribute.

## 💬 Background Story

This project started as a **personal hobby** -  
mainly because my wife *threatened me* to finally find a solution for the **parking app that doesn’t work underground** 😅  

So… I did.  
It only took **one year of thinking** and **5 hours of actual coding**.  

If you have suggestions, improvements, or creative ideas —  
feel free to share them. Contributions and feedback are always welcome!







# 🔧 Setup Instructions

1. Extract your personal PalGate session details.  
You must generate your session token and token type using the QR-code linking process.
Follow the guide inside the directory:
EXTRACT_SESSION_TOKEN/

Session token is the permanent authentication key linked to your PalGate account.  
It is used to generate the temporary request token that actually opens the gate.


2. Configure your credentials  
Open the file CONFIG_TEMPLATE.h, fill in the values you obtained and then rename the file to: config.h
This file is excluded from Git and is already listed in .gitignore.

3. Compile and flash the project  
After the credentials are configured, you can build and upload the firmware normally using PlatformIO.

4. First boot Wi-Fi provisioning  
When the ESP32 scanner boots for the first time, it automatically starts an Access Point and opens a Wi-Fi configuration page.
The default address is:
http://192.168.4.1

(This can be modified in the code.)

After entering your Wi-Fi SSID and password, the scanner will store them in NVS (persist storage).

In order to connect the scanner to a different Wi-Fi network and erase the stored credentials from NVS:
Press the BOOT button (GPIO0) once.
Immediately after that, press the EN (RESET) button to reboot the device.

After the reboot, the ESP32 will start as if it’s booting for the first time:
it will create an Access Point and open the Wi-Fi provisioning page, waiting for new SSID and password input.


Testing the system

Press the button on the beacon → it will transmit a BLE advertisement.

The scanner should detect it and turn on its LED.

You can also open a serial terminal (UART) to the scanner to view logs and debugging messages.


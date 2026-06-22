# WiFi Interlocking System using ESP8266 + ATmega328P + USB Host Shield

This project is a QR-based WiFi Interlocking System using:
 ATmega328P + USB Host Shield
 ESP8266 (ESP-01)
 QR Scanner (USB HID device)
 Relay Module
 MQTT Communication
 HTTP API Validation


The system works as follows:
1. QR scanner scans a QR code.
2. USB Host Shield reads QR data.
3. ATmega328P sends QR to ESP8266.
4. ESP8266 publishes QR through MQTT.
5. ESP8266 sends QR to API.
6. API returns response.
7. ESP8266 sends LOCK/UNLOCK command to ATmega.
8. ATmega controls relay and LEDs.

---

# System Architecture

QR Scanner

↓

USB Host Shield

↓

ATmega328P

↓

SoftwareSerial

↓

ESP8266 (ESP-01)

↓

MQTT Broker + API Server

↓

Relay Control

---

# Hardware Required

| Component                | Quantity    |
| ------------------------ | ----------- |
| ATmega328P / Arduino Uno | 1           |
| ESP8266 ESP-01           | 1           |
| USB Host Shield          | 1           |
| QR Scanner (USB HID)     | 1           |
| Relay Module             | 1           |
| LEDs (Red, Blue, Green)  | 3           |
| Resistors 220Ω           | 3           |
| PCB                      | 1           |
| 5V Power Supply          | 1           |
 

---

# Software Requirements

## Arduino IDE

Recommended:

Arduino IDE Version-2.3.x or latest
Older IDE-1.8.19 also supported

---

# Board Package Installation

Open:

Tools → Board → Boards Manager

Install:

### ESP8266 Board Package

Package:

ESP8266 by ESP8266 Community, Recommended Version: 3.1.2

---

# Required Libraries

Open:

Sketch → Include Library → Manage Libraries

Install the following libraries:

### ESP8266 Side

1. ESP8266WiFi
Author: ESP8266 Community

2. PubSubClient
Author: Nick O'Leary (Recommended: 2.8+)

3. ESP8266HTTPClient
Author:
ESP8266 Community

---

### ATmega Side

1. USB Host Shield Library 2.0
Author: Felis

2. SoftwareSerial
Built into Arduino IDE

3. SPI
Built into Arduino IDE

---


# Pin Connections

## ATmega328P Connections

### Relay

| Device   | ATmega Pin |
| -------- | ---------- |
| Relay IN | D9         |

---

### LEDs

| LED       | Pin |
| --------- | --- |
| Red LED   | D8  |
| Blue LED  | D6  |
| Green LED | D5  |

---

### ESP8266 Serial Connection

| ATmega328P  | ESP-01 |
| ----------- | ------ |
| Pin D7 (TX) | RX     |
| Pin D4 (RX) | TX     |
| GND         | GND    |
| VCC         | 3.3V   |

Important:
ESP-01 works only at 3.3V. Do NOT connect directly to 5V.Use level shifting (AMS117)

---

### USB Host Shield Pins

| USB Host Shield | Arduino Uno |
| --------------- | ----------- |
| MISO            | D12         |
| MOSI            | D11         |
| SCK             | D13         |
| SS              | D10         |

---

# WiFi Configuration

Edit inside ESP code:
const char* ssid1="YOUR_WIFI";
const char* pass1="PASSWORD";

const char* ssid2="BACKUP_WIFI";
const char* pass2="PASSWORD";

---

# MQTT Configuration

Edit:

```cpp
const char* mqtt_server="178.236.185.7";
```

Topics generated automatically:

Publish Topic:

```cpp
pub1/<MAC_ADDRESS>
```

Subscribe Topic:

```cpp
sub1/<MAC_ADDRESS>
```

---

# API Configuration

Edit:

```cpp
String baseURL=
"http://178.236.185.7:2999/v1/";
```

QR automatically appends:

Example:

```cpp
http://178.236.185.7:2999/v1/ABC123
```

---

# Upload Procedure

## Step 1

Connect Arduino/ATmega328P

Open:

Tools → Board
Select-Arduino Uno

---

## Step 2

Select COM Port

Tools → Port- Choose correct COM port

---

## Step 3

Upload Host Shield Code

Upload- wifi-v2-hostshield.ino

---

## Step 4

Connect ESP8266

Select- Generic ESP8266 Module

---

## Step 5

Upload ESP code

Upload- wifi-v2-esp01.ino

---

# Serial Monitor Settings

Baud Rate- 9600
Line Ending- Both NL & CR

---

# Expected Startup Output

ESP Side:

```text
System Starting

Trying WiFi
SSID: MachineWise

WiFi Connected
IP: 192.168.1.120

MQTT Connected

Setup Complete
```

ATmega Side:

```text
System Starting

ESP Serial Started

Pins configured

Relay initialized OFF

Initializing USB Host

USB Host Initialized

Keyboard parser attached

Setup Complete
```

---

# QR Scan Example

When QR scanned:

```text
Character received: A
Character received: B
Character received: C

QR end detected

Complete QR Data:
ABC123

Sending to ESP:
QR:ABC123
```

ESP Side:

```text
Serial Received:
QR:ABC123

Calling API

HTTP Response:
200

QR Success

Unlocking Relay
```

---

# LED Indications

| LED   | Meaning                  |
| ----- | ------------------------ |
| Green | System powered and ready |
| Blue  | MQTT activity            |
| Red   | Relay active             |

---

# Troubleshooting

## USB Host not detected

Check:

* USB Host Shield library installed
* Scanner connected properly
* Shield soldering

---

## WiFi not connecting

Check:

* SSID
* Password
* Signal strength

---

## MQTT not connecting

Check:

* MQTT server IP
* Internet availability
* Broker status

---

## QR not reading

Check:

* Scanner operating in USB HID mode
* Scanner power supply
* USB cable

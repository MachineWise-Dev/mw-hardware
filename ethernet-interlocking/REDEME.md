# Ethernet MQTT Relay Interlock System

## Project Overview

This project implements an Ethernet-based MQTT relay interlock system using:
 ATmega328P
 ENC28J60 Ethernet Module
 MQTT Broker
Relay
 Status LEDs

The system performs the following operations:
1. Initializes Ethernet using static IP
2. Connects to MQTT broker
3. Receives LOCK/UNLOCK MQTT commands
4. Controls relay state
5. Publishes relay status
6. Publishes network details
7. Maintains MQTT connection continuously

---

# Hardware Requirements

| Component                | Quantity |
| ------------------------ | -------- |
| ATmega328P / Arduino Uno | 1        |
| ENC28J60 Ethernet Module | 1        |
| Relay Module             | 1        |
| LEDs                     | 3        |
| Resistors (220Ω)         | 3        |
| USB Cable                | 1        |
| Ethernet Cable           | 1        |
| 5V SMPS                  | 1        |

---

# Software Requirements

## Arduino IDE

Recommended:

Arduino IDE Version-2.3.x or newer
Tested with-Arduino IDE 2.3.6

---

# Required Libraries

Install the following libraries from Arduino Library Manager:

### UIPEthernet

Used for- ENC28J60 Ethernet communication
Library- UIPEthernet by Norbert Truchsess

---

### PubSubClient

Used for- MQTT communication

Library- PubSubClient by Nick O'Leary

---

### SPI

Used for-SPI communication with ENC28J60

---

# Library Installation Steps

Open:

Sketch → Include Library → Manage Libraries
Install:
1. UIPEthernet
2. PubSubClient
SPI is already available.

---

# Pin Connections

## Relay and LEDs

| Arduino Pin | Connected To     |
| ----------- | ---------------- |
| D7          | Relay IN         |
| D5          | Power LED        |
| D6          | MQTT LED         |
| D8          | Relay Status LED |

---

## ENC28J60 Connections

| ENC28J60 | Arduino |
| -------- | ------- |
| VCC      | 3.3V    |
| GND      | GND     |
| SCK      | D13     |
| SO       | D12     |
| SI       | D11     |
| CS       | D10     |

---

# LED Functions

## Power LED

LED_POWER
Purpose:Shows system power status
Behavior:ON = System powered

---

## MQTT LED

LED_MQTT
Purpose:Shows MQTT publish activity
Behavior:Blinks during publish

---

## Relay LED

LED_RELAY
Purpose:Shows relay status
Behavior:
ON = Relay active
OFF = Relay inactive

---

# Network Configuration

Current configuration:
MQTT Server:192.168.4.150
MQTT Port:1883
Static Device IP:192.168.4.101
MAC Address:AC:0A:E7:FF:DC:01

---

# MQTT Topics
Subscribe Topic:sub1/AC0AE7FFDC01
Publish Topic:pub1/AC0AE7FFDC01

---

# MQTT Commands

Supported commands:
LOCK1
Action:Relay ON

---

UNLOCK1
Action:Relay OFF

---

# MQTT Payload Format

Published payload:

```json
{
   "data":{
      "mac":"AC0AE7FFDC01",
      "status":{
         "lock1":1
      },
      "network":{
         "eth0":{
            "ip":"192.168.4.101"
         }
      }
   }
}
```

Where:

lock1 = 1 → Relay ON

lock1 = 0 → Relay OFF

---

# Upload Procedure

Step 1:
Open Arduino IDE
---
Step 2:
Install required libraries

---
Step 3:
Connect Arduino board

---

Step 4:
Select board:
Tools → Board
Select:Arduino Uno

---

Step 5:
Select COM port:
Tools → Port, Choose appropriate COM port

---

Step 6:

Open project file

Example:ethernet-interlocking.ino

---

Step 7:

Click Upload

Wait until Done Uploading appears

---

# Serial Monitor Setup

Open:

Tools → Serial Monitor

Baud Rate:9600

---

# Expected Startup Console Output

```text
System Starting

SPI SS configured

Pins configured

Power LED ON

applyRelay() called

Relay OFF

LED_RELAY OFF

Relay initialized OFF

Starting Ethernet

Ethernet Started

IP Address:
192.168.4.101

MQTT Server Configured

MQTT Callback Attached

MQTT Buffer Set

Setup Complete
```

---

# Expected MQTT Connection Output

```text
MQTT Disconnected

Entering reconnect()

Connecting MQTT...

MQTT Connected

Subscribed Topic:
sub1/AC0AE7FFDC01
```

---

# LOCK Command Output

```text
MQTT Message Received

Topic:
sub1/AC0AE7FFDC01

Command:
LOCK1

Executing LOCK

applyRelay() called

Relay ON

LED_RELAY ON
```

---

# UNLOCK Command Output

```text
MQTT Message Received

Topic:
sub1/AC0AE7FFDC01

Command:
UNLOCK1

Executing UNLOCK

applyRelay() called

Relay OFF

LED_RELAY OFF
```

---

# Periodic MQTT Publish Output

```text
Periodic MQTT Update

publishStatus() called

MQTT Published

MQTT LED Blink
```

---

# Troubleshooting

## Ethernet IP shows 0.0.0.0

Check:

* Ethernet cable connection
* Router connection
* Static IP settings

---

## MQTT not connecting

Check:

* MQTT broker IP
* MQTT broker port
* Broker running status

---

## Relay not changing state

Check:

* Relay wiring
* Relay power supply
* D7 connection

---

## MQTT commands not received

Verify:

Topic names exactly match:

Publish:

pub1/AC0AE7FFDC01

Subscribe:

sub1/AC0AE7FFDC01

---

# Project Flow

Power ON

↓

Initialize pins

↓

Start Ethernet

↓

Connect MQTT

↓

Subscribe topic

↓

Wait for MQTT command

↓

LOCK1 / UNLOCK1 received

↓

Update relay state

↓

Publish status

↓

Repeat forever

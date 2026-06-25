#include <hidboot.h>
#include <usbhub.h>
#include <SPI.h>
#include <SoftwareSerial.h>

USB Usb;
HIDBoot<USB_HID_PROTOCOL_KEYBOARD> HidKeyboard(&Usb);

SoftwareSerial espSerial(4,7);

#define RELAY_PIN 9
#define LED_RED 8
#define LED_BLUE 6
#define LED_GREEN 5

int relayState=0;

String qrData="";
String lastQR="";

unsigned long prevSend=0;

void applyRelay(int state)
{
    relayState=state;

    if(relayState==1)
    {
        digitalWrite(RELAY_PIN,HIGH);
        digitalWrite(LED_RED,HIGH);
    }
    else
    {
        digitalWrite(RELAY_PIN,LOW);
        digitalWrite(LED_RED,LOW);
    }
}

void sendToESP(String data)
{
    espSerial.println(data);
}

class KbdRptParser:
public KeyboardReportParser
{
void OnKeyDown(
uint8_t mod,
uint8_t key)
{
char c=OemToAscii(mod,key);

if(!c) return;

if(c=='\r'||c=='\n')
{
if(qrData.length()>0)
{
lastQR=qrData;

sendToESP(
"QR:"+lastQR);
}

qrData="";
}
else
{
qrData+=c;
}
}
};

KbdRptParser Parser;


void setup()
{
espSerial.begin(9600);

pinMode(RELAY_PIN,OUTPUT);
pinMode(LED_RED,OUTPUT);
pinMode(LED_BLUE,OUTPUT);
pinMode(LED_GREEN,OUTPUT);

digitalWrite(LED_GREEN,HIGH);

applyRelay(0);

if(Usb.Init()==-1)
{
while(1);
}

HidKeyboard.SetReportParser(
0,
&Parser);
}


void loop()
{
Usb.Task();

while(espSerial.available())
{
String cmd=
espSerial.readStringUntil('\n');

cmd.trim();

if(cmd=="LOCK")
{
applyRelay(1);
}
else if(cmd=="UNLOCK")
{
applyRelay(0);
}
else if(cmd=="MQTT_BLINK")
{
digitalWrite(
LED_BLUE,HIGH);

delay(20);

digitalWrite(
LED_BLUE,LOW);
}
else if(
cmd=="QR_OK"||
cmd=="QR_NOT_OK")
{
lastQR="";
qrData="";
}
}


if(millis()-prevSend>1000)
{
prevSend=millis();

if(lastQR.length()>0)
{
sendToESP(
"QR:"+lastQR);
}
}
}

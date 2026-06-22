#include <hidboot.h>
#include <usbhub.h>
#include <SPI.h>
#include <SoftwareSerial.h>

USB Usb;
HIDBoot<USB_HID_PROTOCOL_KEYBOARD> HidKeyboard(&Usb);

// =====================================================
// SOFTWARE SERIAL
// Used for communication between ATmega328P and ESP
// RX = Pin 4
// TX = Pin 7
// =====================================================
SoftwareSerial espSerial(4, 7);


// =====================================================
// PIN DEFINITIONS
// RELAY_PIN : Controls relay ON/OFF
// LED_RED   : Indicates relay state
// LED_BLUE  : Indicates QR activity
// LED_GREEN : Indicates system power/ready
// =====================================================
#define RELAY_PIN 9
#define LED_RED   8
#define LED_BLUE  6
#define LED_GREEN 5


// =====================================================
// GLOBAL VARIABLES
//
// relayState : Current relay state
// qrData      : Temporary QR buffer
// lastQR      : Stores last complete QR
// prevSend    : Used for QR resend timing
// =====================================================
int relayState = 0;

String qrData = "";
String lastQR = "";

unsigned long prevSend = 0;


// =====================================================
// RELAY CONTROL FUNCTION
//
// Purpose:
// Updates relay state and LED state.
//
// Input:
// state=1 → Relay ON
// state=0 → Relay OFF
// =====================================================
void applyRelay(int state)
{
  relayState = state;

  Serial.println("applyRelay() called");

  if (relayState == 1)
  {
    digitalWrite(RELAY_PIN,HIGH);
    digitalWrite(LED_RED,HIGH);

    Serial.println("Relay ON");
  }
  else
  {
    digitalWrite(RELAY_PIN,LOW);
    digitalWrite(LED_RED,LOW);

    Serial.println("Relay OFF");
  }
}


// =====================================================
// SEND DATA TO ESP
//
// Purpose:
// Sends any string data from ATmega to ESP
//
// Example:
// QR:123456
// LOCK
// =====================================================
void sendToESP(String data)
{
  Serial.print("Sending to ESP: ");
  Serial.println(data);

  espSerial.println(data);
}


// =====================================================
// QR KEYBOARD PARSER
//
// Purpose:
// Reads USB keyboard characters from scanner
//
// Working:
//
// Scanner sends:
//
// A
// B
// C
// Enter
//
// Code stores:
//
// qrData=A
// qrData=AB
// qrData=ABC
//
// Enter indicates QR finished
// =====================================================
class KbdRptParser : public KeyboardReportParser
{
  void OnKeyDown(uint8_t mod,uint8_t key)
  {
    char c = OemToAscii(mod,key);

    // Ignore invalid characters
    if(!c)
      return;

    Serial.print("Character received: ");
    Serial.println(c);

    // QR finished
    if(c=='\r' || c=='\n')
    {
      Serial.println("QR end detected");

      if(qrData.length()>0)
      {
        // Store completed QR
        lastQR=qrData;

        Serial.print("Complete QR Data: ");
        Serial.println(lastQR);

        // Send QR to ESP
        sendToESP("QR:"+lastQR);

        // Blink QR LED
        digitalWrite(LED_BLUE,HIGH);
        delay(100);
        digitalWrite(LED_BLUE,LOW);
      }

      // Clear temporary buffer
      qrData="";

      Serial.println("QR buffer cleared");
    }

    else
    {
      // Build QR character by character
      qrData+=c;

      Serial.print("Current QR Buffer: ");
      Serial.println(qrData);
    }
  }
};

KbdRptParser Parser;


// =====================================================
// SETUP FUNCTION
//
// Purpose:
//
// Runs only once after power ON
//
// Performs:
//
// 1.Initialize serial ports
// 2.Configure pins
// 3.Initialize relay
// 4.Initialize USB host
// 5.Attach QR parser
// =====================================================
void setup()
{
  Serial.begin(9600);

  Serial.println();
  Serial.println("System Starting");

  espSerial.begin(9600);

  Serial.println("ESP Serial Started");

  pinMode(RELAY_PIN,OUTPUT);
  pinMode(LED_RED,OUTPUT);
  pinMode(LED_BLUE,OUTPUT);
  pinMode(LED_GREEN,OUTPUT);

  Serial.println("Pins configured");

  digitalWrite(LED_GREEN,HIGH);

  relayState=0;
  digitalWrite(RELAY_PIN,LOW);
  digitalWrite(LED_RED,LOW);

  Serial.println("Relay initialized OFF");

  Serial.println("Initializing USB Host");

  if(Usb.Init()==-1)
  {
    Serial.println("USB Host Initialization Failed");

    while(1)
    {
      Serial.println("USB Error");
      delay(1000);
    }
  }

  Serial.println("USB Host Initialized");

  HidKeyboard.SetReportParser(0,&Parser);

  Serial.println("Keyboard parser attached");

  Serial.println("Setup Complete");
}


// =====================================================
// LOOP FUNCTION
//
// Purpose:
//
// Runs continuously forever
//
// Performs:
//
// 1.Process USB keyboard events
// 2.Receive commands from ESP
// 3.Control relay
// 4.Receive QR acknowledgement
// 5.Resend QR if acknowledgement missing
// =====================================================
void loop()
{
  // Process USB keyboard tasks
  Usb.Task();


  // =================================================
  // RECEIVE DATA FROM ESP
  //
  // Checks whether ESP sent:
  //
  // LOCK
  // UNLOCK
  // QR_OK
  // QR_NOT_OK
  // =================================================
  while(espSerial.available())
  {
    String cmd=espSerial.readStringUntil('\n');

    cmd.trim();

    Serial.print("Received from ESP: ");
    Serial.println(cmd);

    if(cmd=="LOCK")
    {
      Serial.println("LOCK command received");

      applyRelay(1);
    }

    else if(cmd=="UNLOCK")
    {
      Serial.println("UNLOCK command received");

      applyRelay(0);
    }

    else if(cmd=="QR_OK")
    {
      Serial.println("QR_OK received");

      lastQR="";
      qrData="";

      Serial.println("QR memory cleared");
    }

    else if(cmd=="QR_NOT_OK")
    {
      Serial.println("QR_NOT_OK received");

      lastQR="";
      qrData="";

      Serial.println("QR memory cleared");
    }
  }


  // =================================================
  // QR RESEND LOGIC
  //
  // If QR is not acknowledged
  // resend every 1 second
  // =================================================
  if((millis()-prevSend)>1000)
  {
    prevSend=millis();

    if(lastQR.length()>0)
    {
      Serial.println("Resending QR");

      sendToESP("QR:"+lastQR);

      digitalWrite(LED_BLUE,HIGH);
      delay(20);
      digitalWrite(LED_BLUE,LOW);
    }
  }
}

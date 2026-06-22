#include <hidboot.h>
#include <usbhub.h>
#include <SoftwareSerial.h>

// =====================================================
// USB QR SCANNER + ESP01 MQTT SYSTEM
//
// FUNCTIONS:
// 1. Initialize USB Host Shield
// 2. Read QR scanner as keyboard
// 3. Send scanned characters to ESP-01
// 4. Configure ESP-01 using AT commands
// 5. Connect ESP to WiFi
// 6. Connect ESP to MQTT
// 7. Monitor ESP responses
// =====================================================


// =====================================================
// SOFTWARE SERIAL
//
// ESP_RX = Arduino RX pin connected to ESP TX
// ESP_TX = Arduino TX pin connected to ESP RX
// =====================================================
#define ESP_RX 10
#define ESP_TX 11

SoftwareSerial espSerial(ESP_RX, ESP_TX);


// =====================================================
// USB HOST OBJECTS
// =====================================================
USB Usb;
HIDBoot<USB_HID_PROTOCOL_KEYBOARD> HidKeyboard(&Usb);


// =====================================================
// GLOBAL VARIABLES
// =====================================================
String espResponse = "";


// =====================================================
// KEYBOARD PARSER
//
// Purpose:
// Reads QR scanner keyboard data
//
// Example:
//
// Scanner sends:
//
// A
// B
// C
//
// Output:
//
// ABC
//
// Characters are sent immediately to ESP
// =====================================================
class KbdRptParser : public KeyboardReportParser
{
  void OnKeyDown(uint8_t mod,uint8_t key) override
  {
    uint8_t c=OemToAscii(mod,key);

    if(c)
    {
      Serial.println("--------------------------------");
      Serial.println("Keyboard character received");

      Serial.print("ASCII Value: ");
      Serial.println(c);

      Serial.print("Character: ");
      Serial.println((char)c);

      Serial.println("Sending character to ESP");

      Serial.print("Data: ");
      Serial.println((char)c);

      // Show locally
      Serial.print((char)c);

      // Send to ESP
      espSerial.print((char)c);

      Serial.println();
      Serial.println("--------------------------------");
    }
  }
};

KbdRptParser KbdParser;


// =====================================================
// SEND AT COMMAND
//
// Purpose:
//
// Sends AT command to ESP
// Reads response
//
// Example:
//
// AT
// AT+RST
//
// =====================================================
void sendATCommand(String cmd,int waitTime=500)
{
  Serial.println();
  Serial.println("================================");

  Serial.println("Sending AT Command");

  Serial.print("Command: ");
  Serial.println(cmd);

  espResponse="";

  espSerial.println(cmd);

  delay(waitTime);

  Serial.println("Reading ESP response");

  while(espSerial.available())
  {
    char c=espSerial.read();

    espResponse+=c;
  }

  Serial.println("ESP Response:");
  Serial.println(espResponse);

  Serial.println("================================");
}


// =====================================================
// WIFI SETUP
//
// Purpose:
//
// Reset ESP
// Set Station mode
// Connect WiFi
// =====================================================
void setupWiFi()
{
  Serial.println();
  Serial.println("Starting WiFi Setup");

  Serial.println("Resetting ESP");
  sendATCommand("AT+RST",2000);

  Serial.println("Setting Station Mode");
  sendATCommand("AT+CWMODE=1");

  Serial.println("Connecting WiFi");

  sendATCommand(
  "AT+CWJAP=\"YourSSID\",\"YourPassword\"",
  5000);

  Serial.println("WiFi setup completed");
}


// =====================================================
// MQTT SETUP
//
// Purpose:
//
// Configure MQTT user
// Connect MQTT broker
// =====================================================
void setupMQTT()
{
  Serial.println();
  Serial.println("Starting MQTT Setup");

  Serial.println("Configuring MQTT user");

  sendATCommand(
  "AT+MQTTUSERCFG=0,1,\"UnoClient\",\"admin\",\"Password@1234\",0,0,\"\""
  );

  Serial.println("Connecting MQTT Broker");

  sendATCommand(
  "AT+MQTTCONN=0,\"broker.machinewiseapp.in\",1883,0"
  );

  Serial.println("MQTT setup completed");
}


// =====================================================
// MQTT PUBLISH
//
// Purpose:
//
// Publish MQTT message
//
// Example:
//
// Hello
// QR12345
// =====================================================
void publishMQTT(String msg)
{
  Serial.println();
  Serial.println("Publishing MQTT");

  Serial.print("Message: ");
  Serial.println(msg);

  String cmd=
  "AT+MQTTPUB=0,\"esp01/keyboard\",\""
  +msg+
  "\",0,0";

  sendATCommand(cmd);

  Serial.println("MQTT publish complete");
}


// =====================================================
// SETUP
//
// Runs once after power ON
//
// Tasks:
//
// 1.Start serial
// 2.Start ESP serial
// 3.Initialize USB Host
// 4.Attach keyboard parser
// 5.Connect WiFi
// 6.Connect MQTT
// =====================================================
void setup()
{
  Serial.begin(9600);

  Serial.println();
  Serial.println("================================");
  Serial.println("SYSTEM STARTING");
  Serial.println("================================");

  espSerial.begin(9600);

  Serial.println("ESP Serial Started");

  Serial.println("Initializing USB Host");

  if(Usb.Init()==-1)
  {
    Serial.println("USB Host Shield Init Failed");

    while(1)
    {
      Serial.println("USB ERROR");

      delay(1000);
    }
  }

  Serial.println("USB Host Initialized");

  delay(200);

  HidKeyboard.SetReportParser(
  0,
  &KbdParser);

  Serial.println("Keyboard Parser Attached");

  Serial.println();

  Serial.println(
  "Connecting ESP to WiFi");

  setupWiFi();

  Serial.println();

  Serial.println(
  "Connecting MQTT");

  setupMQTT();

  Serial.println();

  Serial.println(
  "System Ready");

  Serial.println(
  "Scan QR code...");
}


// =====================================================
// LOOP
//
// Runs forever
//
// Tasks:
//
// 1.Process USB events
// 2.Receive ESP data
// 3.Show ESP responses
// =====================================================
void loop()
{
  // ---------------------------------------
  // Process USB scanner events
  // ---------------------------------------
  Usb.Task();


  // ---------------------------------------
  // Read any incoming data from ESP
  // ---------------------------------------
  while(espSerial.available())
  {
    char c=espSerial.read();

    Serial.print(
    "ESP Incoming: ");

    Serial.write(c);
  }

  delay(50);
}

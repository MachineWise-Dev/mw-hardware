// =====================================================
// ETHERNET MQTT RELAY INTERLOCK SYSTEM
//
// Functions:
//
// 1. Initializes Ethernet with static IP
// 2. Connects to MQTT broker
// 3. Receives LOCK/UNLOCK MQTT commands
// 4. Controls relay and LEDs
// 5. Publishes device status
// 6. Publishes network information
// 7. Maintains MQTT connection
// =====================================================

#include <UIPEthernet.h>
#include <PubSubClient.h>
#include <SPI.h>


// =====================================================
// PIN DEFINITIONS
//
// RELAY_PIN  : Controls relay
// LED_POWER  : Power indication
// LED_MQTT   : MQTT activity LED
// LED_RELAY  : Relay state LED
// =====================================================
#define RELAY_PIN   7
#define LED_POWER   5
#define LED_MQTT    6
#define LED_RELAY   8


// =====================================================
// NETWORK CONFIGURATION
// =====================================================
byte mac[] =
{
  0xAC,0x0A,0xE7,
  0xFF,0xDC,0x01
};

IPAddress mqttServer(
192,168,4,150);

const int mqttPort=1883;


// =====================================================
// STATIC IP
// =====================================================
IPAddress staticIP(
192,168,4,101);


// =====================================================
// MQTT TOPICS
// =====================================================
const char pubTopic[]=
"pub1/AC0AE7FFDC01";

const char subTopic[]=
"sub1/AC0AE7FFDC01";


// =====================================================
// OBJECT CREATION
// =====================================================
EthernetClient ethClient;

PubSubClient client(
ethClient);


// =====================================================
// GLOBAL VARIABLES
// =====================================================
bool relayState=HIGH;

unsigned long lastPublish=0;


// =====================================================
// RELAY CONTROL
//
// Purpose:
//
// Updates relay state
// Controls relay LED
//
// state=LOW
// Relay ON
//
// state=HIGH
// Relay OFF
// =====================================================
void applyRelay(bool state)
{
  Serial.println();
  Serial.println("applyRelay() called");

  relayState=state;

  digitalWrite(
  RELAY_PIN,
  relayState);

  if(relayState==LOW)
  {
    digitalWrite(
    LED_RELAY,
    HIGH);

    Serial.println(
    "Relay ON");

    Serial.println(
    "LED_RELAY ON");
  }
  else
  {
    digitalWrite(
    LED_RELAY,
    LOW);

    Serial.println(
    "Relay OFF");

    Serial.println(
    "LED_RELAY OFF");
  }
}


// =====================================================
// PUBLISH DEVICE STATUS
//
// Purpose:
//
// Creates JSON payload
//
// Includes:
//
// MAC
// Relay State
// IP Address
//
// Publishes to MQTT
// =====================================================
void publishStatus()
{
  Serial.println();
  Serial.println(
  "publishStatus() called");

  IPAddress ip=
  Ethernet.localIP();

  char ipStr[16];

  sprintf(
  ipStr,
  "%u.%u.%u.%u",
  ip[0],
  ip[1],
  ip[2],
  ip[3]);

  char macStr[13];

  sprintf(
  macStr,
  "%02X%02X%02X%02X%02X%02X",
  mac[0],
  mac[1],
  mac[2],
  mac[3],
  mac[4],
  mac[5]);

  int lockVal=
  (relayState==LOW)
  ?1:0;

  char msg[220];

  sprintf(
  msg,
  "{\"data\":{\"mac\":\"%s\",\"status\":{\"lock1\":%d},\"network\":{\"eth0\":{\"ip\":\"%s\"}}}}",
  macStr,
  lockVal,
  ipStr
  );

  Serial.println(
  "MQTT Payload:");

  Serial.println(msg);

  if(client.publish(
  pubTopic,
  msg))
  {
      Serial.println(
      "MQTT Published");

      digitalWrite(
      LED_MQTT,
      HIGH);

      delay(40);

      digitalWrite(
      LED_MQTT,
      LOW);

      Serial.println(
      "MQTT LED Blink");
  }
  else
  {
      Serial.println(
      "MQTT Publish Failed");
  }
}


// =====================================================
// MQTT CALLBACK
//
// Purpose:
//
// Receives MQTT messages
//
// Commands:
//
// LOCK1
// UNLOCK1
// =====================================================
void callback(
char* topic,
byte* payload,
unsigned int length)
{
  Serial.println();
  Serial.println(
  "MQTT Message Received");

  Serial.print(
  "Topic: ");

  Serial.println(
  topic);

  char msg[20];

  int i;

  for(
  i=0;
  i<length && i<19;
  i++)
  {
      msg[i]=
      (char)
      payload[i];
  }

  msg[i]='\0';

  Serial.print(
  "Command: ");

  Serial.println(
  msg);


  // ---------------- LOCK ----------------
  if(strcmp(
     msg,
     "LOCK1")==0)
  {
      Serial.println(
      "Executing LOCK");

      applyRelay(
      LOW);
  }


  // ---------------- UNLOCK ----------------
  else if(strcmp(
          msg,
          "UNLOCK1")==0)
  {
      Serial.println(
      "Executing UNLOCK");

      applyRelay(
      HIGH);
  }

  publishStatus();
}


// =====================================================
// MQTT RECONNECT
//
// Purpose:
//
// Keeps MQTT connected forever
// =====================================================
void reconnect()
{
  Serial.println();
  Serial.println(
  "Entering reconnect()");

  while(
  !client.connected())
  {
      Serial.println(
      "Connecting MQTT...");

      if(client.connect(
      "RelayClient_01"))
      {
          Serial.println(
          "MQTT Connected");

          client.subscribe(
          subTopic);

          Serial.print(
          "Subscribed Topic: ");

          Serial.println(
          subTopic);

          publishStatus();
      }
      else
      {
          Serial.println(
          "MQTT Connection Failed");

          Serial.print(
          "State: ");

          Serial.println(
          client.state());

          delay(2000);
      }
  }
}


// =====================================================
// SETUP
//
// Runs once after power ON
//
// Tasks:
//
// 1.Initialize serial
// 2.Configure pins
// 3.Initialize relay
// 4.Start Ethernet
// 5.Configure MQTT
// =====================================================
void setup()
{
  Serial.begin(9600);

  Serial.println();
  Serial.println(
  "System Starting");

  pinMode(
  10,
  OUTPUT);

  digitalWrite(
  10,
  HIGH);

  Serial.println(
  "SPI SS configured");


  pinMode(
  RELAY_PIN,
  OUTPUT);

  pinMode(
  LED_POWER,
  OUTPUT);

  pinMode(
  LED_MQTT,
  OUTPUT);

  pinMode(
  LED_RELAY,
  OUTPUT);

  Serial.println(
  "Pins configured");

  digitalWrite(
  LED_POWER,
  HIGH);

  Serial.println(
  "Power LED ON");


  applyRelay(
  HIGH);

  Serial.println(
  "Relay initialized OFF");

  delay(200);

  Serial.println(
  "Starting Ethernet");

  Ethernet.begin(
  mac,
  staticIP);

  delay(1000);

  Serial.println(
  "Ethernet Started");

  Serial.print(
  "IP Address: ");

  Serial.println(
  Ethernet.localIP());

  client.setServer(
  mqttServer,
  mqttPort);

  Serial.println(
  "MQTT Server Configured");

  client.setCallback(
  callback);

  Serial.println(
  "MQTT Callback Attached");

  client.setBufferSize(
  256);

  Serial.println(
  "MQTT Buffer Set");

  Serial.println(
  "Setup Complete");
}


// =====================================================
// LOOP
//
// Runs forever
//
// Tasks:
//
// 1.Check MQTT connection
// 2.Process MQTT data
// 3.Publish status every 2 sec
// =====================================================
void loop()
{
  // --------------------------------
  // MQTT Connection Check
  // --------------------------------
  if(
  !client.connected())
  {
      Serial.println(
      "MQTT Disconnected");

      reconnect();
  }


  // --------------------------------
  // Process MQTT packets
  // --------------------------------
  client.loop();


  // --------------------------------
  // Publish status every 2 seconds
  // --------------------------------
  if(
  millis()
  -
  lastPublish
  >=2000)
  {
      lastPublish=
      millis();

      Serial.println(
      "Periodic MQTT Update");

      publishStatus();
  }
}

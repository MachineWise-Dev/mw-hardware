#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <ESP8266HTTPClient.h>

// =====================================================
// ESP8266 QR + MQTT + API INTERLOCK SYSTEM
//
// FUNCTIONS:
//
// 1.Connect WiFi (2 SSIDs)
// 2.Connect MQTT broker
// 3.Receive QR from Arduino
// 4.Call API using QR
// 5.Receive MQTT LOCK/UNLOCK
// 6.Send relay commands to Arduino
// 7.Publish MQTT status
// =====================================================


// =====================================================
// WIFI CREDENTIALS
// =====================================================
const char* ssid1="MachineWise-Electronics";
const char* pass1="Machine@123";

const char* ssid2="Mayuri";
const char* pass2="Mayuri@123";


// =====================================================
// MQTT SERVER
// =====================================================
const char* mqtt_server="178.236.185.7";


// =====================================================
// API URL
// =====================================================
String baseURL=
"http://178.236.185.7:2999/v1/";


WiFiClient espClient;
PubSubClient client(espClient);


// =====================================================
// GLOBAL VARIABLES
// =====================================================
String lastQR="";
String mqttCmd="";
String qrResult="";

String macID="";
String pubTopic="";
String subTopic="";

int relayState=1;
int httpCodeVal=0;

unsigned long lastMQTT=0;


// =====================================================
// CONNECT SINGLE WIFI
//
// Tries for 8 seconds
// =====================================================
void connectWiFi(
const char* ssid,
const char* password)
{
  Serial.println();
  Serial.println("Trying WiFi");

  Serial.print("SSID: ");
  Serial.println(ssid);

  WiFi.begin(
  ssid,
  password);

  unsigned long start=
  millis();

  while(
  WiFi.status()!=WL_CONNECTED &&
  millis()-start<8000)
  {
    Serial.print(".");
    delay(300);
  }

  Serial.println();

  if(WiFi.status()==WL_CONNECTED)
  {
    Serial.println(
    "WiFi Connected");

    Serial.print(
    "IP: ");

    Serial.println(
    WiFi.localIP());
  }
  else
  {
    Serial.println(
    "WiFi Failed");
  }
}


// =====================================================
// WIFI SETUP
//
// Try WiFi1
// If fail try WiFi2
// =====================================================
void setup_wifi()
{
  Serial.println(
  "Starting WiFi Setup");

  WiFi.mode(
  WIFI_STA);

  connectWiFi(
  ssid1,
  pass1);

  if(
  WiFi.status()==WL_CONNECTED)
  return;

  connectWiFi(
  ssid2,
  pass2);

  if(
  WiFi.status()==WL_CONNECTED)
  return;

  while(
  WiFi.status()!=WL_CONNECTED)
  {
    connectWiFi(
    ssid1,
    pass1);

    if(
    WiFi.status()==WL_CONNECTED)
      break;

    connectWiFi(
    ssid2,
    pass2);
  }
}


// =====================================================
// API CALL
//
// Sends QR to server
// Returns HTTP response
// =====================================================
int callAPI(String qr)
{
  Serial.println();
  Serial.println(
  "Calling API");

  HTTPClient http;

  String url=
  baseURL+qr;

  Serial.print(
  "URL: ");

  Serial.println(url);

  http.begin(
  espClient,
  url);

  int code=
  http.GET();

  Serial.print(
  "HTTP Response: ");

  Serial.println(code);

  http.end();

  return code;
}


// =====================================================
// SEND COMMAND TO ARDUINO
//
// Sends LOCK or UNLOCK
// =====================================================
void sendToArduino()
{
  Serial.println();

  Serial.println(
  "Sending command to Arduino");

  if(relayState==1)
  {
    Serial.println(
    "LOCK");

    Serial.println(
    "LOCK");

    delay(100);

    Serial.println(
    "LOCK");
  }
  else
  {
    Serial.println(
    "UNLOCK");

    Serial.println(
    "UNLOCK");

    delay(100);

    Serial.println(
    "UNLOCK");
  }
}


// =====================================================
// SEND MQTT STATUS
// =====================================================
void sendMQTT()
{
  if(!client.connected())
  {
    Serial.println(
    "MQTT Not Connected");

    return;
  }

  String mac=
  WiFi.macAddress();

  mac.replace(
  ":",
  "");

  String ip=
  WiFi.localIP().toString();

  String payload="{";

  payload+="\"data\":{";

  payload+="\"qr\":\""
  +lastQR+"\",";

  payload+="\"mac\":\""
  +mac+"\",";

  payload+="\"status\":{";

  payload+="\"lock1\":"
  +String(relayState);

  payload+="},";

  payload+="\"network\":{";

  payload+="\"wlan0\":{";

  payload+="\"ip\":\""
  +ip+"\"";

  payload+="}}}}";

  Serial.println();
  Serial.println(
  "Publishing MQTT");

  Serial.println(
  payload);

  if(
  client.publish(
  pubTopic.c_str(),
  payload.c_str()))
  {
    Serial.println(
    "MQTT_SENT");
  }
}


// =====================================================
// MQTT CALLBACK
//
// Receives MQTT command
// =====================================================
void callback(
char* topic,
byte* payload,
unsigned int length)
{
  Serial.println(
  "MQTT Message Received");

  mqttCmd="";

  for(
  unsigned int i=0;
  i<length;
  i++)
  {
    mqttCmd+=
    (char)payload[i];
  }

  mqttCmd.trim();

  Serial.print(
  "Command: ");

  Serial.println(
  mqttCmd);
}


// =====================================================
// MQTT RECONNECT
// =====================================================
void reconnect()
{
  while(
  !client.connected())
  {
    Serial.println(
    "Connecting MQTT");

    if(
    client.connect(
    "ESP01"))
    {
      Serial.println(
      "MQTT Connected");

      client.subscribe(
      subTopic.c_str());

      Serial.print(
      "Subscribed: ");

      Serial.println(
      subTopic);
    }
    else
    {
      Serial.println(
      "MQTT Failed");

      delay(1500);
    }
  }
}


// =====================================================
// SETUP
// =====================================================
void setup()
{
  Serial.begin(9600);

  Serial.println();
  Serial.println(
  "System Starting");

  delay(1000);

  setup_wifi();

  macID=
  WiFi.macAddress();

  macID.replace(
  ":",
  "");

  pubTopic=
  "pub1/"+macID;

  subTopic=
  "sub1/"+macID;

  Serial.print(
  "Publish Topic: ");

  Serial.println(
  pubTopic);

  Serial.print(
  "Subscribe Topic: ");

  Serial.println(
  subTopic);

  client.setServer(
  mqtt_server,
  1883);

  client.setCallback(
  callback);

  Serial.println(
  "Setup Complete");
}


// =====================================================
// LOOP
// =====================================================
void loop()
{
  if(
  !client.connected())
  {
    reconnect();
  }

  client.loop();

  // MQTT COMMAND
  if(
  mqttCmd.length()>0)
  {
    Serial.print(
    "Executing: ");

    Serial.println(
    mqttCmd);

    if(
    mqttCmd=="LOCK1" ||
    mqttCmd=="lock1")
    {
      relayState=1;

      sendToArduino();

      sendMQTT();
    }

    else if(
    mqttCmd=="UNLOCK1" ||
    mqttCmd=="unlock1")
    {
      relayState=0;

      sendToArduino();

      sendMQTT();
    }

    mqttCmd="";
  }


  // QR RECEIVE
  if(
  Serial.available())
  {
    String data=
    Serial.readStringUntil('\n');

    data.trim();

    Serial.print(
    "Received: ");

    Serial.println(
    data);

    if(
    data.startsWith(
    "QR:"))
    {
      lastQR=
      data.substring(3);

      Serial.print(
      "QR Value: ");

      Serial.println(
      lastQR);

      sendMQTT();

      httpCodeVal=
      callAPI(
      lastQR);

      if(
      httpCodeVal>=200 &&
      httpCodeVal<300)
      {
        Serial.println(
        "QR_OK");
      }
      else
      {
        Serial.println(
        "QR_NOT_OK");
      }

      lastQR="";

      sendMQTT();
    }
  }


  // Periodic MQTT
  if(
  millis()-lastMQTT>1000)
  {
    lastMQTT=
    millis();

    Serial.println(
    "Periodic MQTT Update");

    sendMQTT();
  }
}

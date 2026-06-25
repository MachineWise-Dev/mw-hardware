// =====================================================
// ESP8266 QR + MQTT + API + EEPROM INTERLOCK SYSTEM
//
// Functions:
//
// 1. Loads WiFi/MQTT/API configuration from EEPROM
// 2. Uses default values if EEPROM empty
// 3. Connects WiFi with timeout + retry
// 4. Automatically switches to default WiFi if needed
// 5. Connects MQTT broker
// 6. Receives QR from Arduino
// 7. Sends QR to API
// 8. Processes MQTT LOCK/UNLOCK commands
// 9. Publishes device status periodically
// 10. Automatically restarts after 1 hour
//
// Data Flow:
//
// QR Scanner
//      ↓
// Arduino → QR:xxxx
//      ↓
// ESP8266
//      ↓
// API Call
//      ↓
// MQTT Publish
//      ↓
// Arduino LOCK/UNLOCK
//
// =====================================================

#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <ESP8266HTTPClient.h>
#include <EEPROM.h>

// =====================================================
// EEPROM + SYSTEM SETTINGS
// =====================================================

#define EEPROM_SIZE 512
#define MAX_QR_LEN 200

// WiFi timeout before fallback
#define WIFI_TIMEOUT 15000

// Retry WiFi every 5 sec
#define WIFI_RETRY 5000

// Restart after 1 hour
#define AUTO_RESTART_TIME 3600000UL


// =====================================================
// EEPROM CONFIGURATION STRUCTURE
//
// Stored permanently inside EEPROM
// =====================================================

struct Config
{
  char ssid[32];
  char password[64];

  char mqttServer[64];

  uint16_t mqttPort;

  char apiURL[120];
};

Config config;


// =====================================================
// DEFAULT VALUES
//
// Used if EEPROM is empty
// =====================================================

const char* DEFAULT_SSID="MachineWise";
const char* DEFAULT_PASS="MachineWise";


// =====================================================
// OBJECTS
// =====================================================

WiFiClient espClient;

PubSubClient mqttClient(
espClient);


// =====================================================
// GLOBAL VARIABLES
// =====================================================

String mqtt_server="";
String baseURL="";

String lastQR="";
String mqttCmd="";

String macID="";
String pubTopic="";
String subTopic="";

char serialLine[MAX_QR_LEN+1];

int serialLen=0;

int relayState=0;

int httpCodeVal=0;

bool wifiConnecting=false;

bool triedDefaultWiFi=false;

unsigned long wifiStartTime=0;

unsigned long lastWifiAttempt=0;

unsigned long lastMQTT=0;

unsigned long lastMQTTConnect=0;

unsigned long restartTimer=0;


// =====================================================
// SAVE CONFIGURATION TO EEPROM
// =====================================================

void saveConfig()
{
  Serial.println();
  Serial.println("Saving Config");

  EEPROM.put(0,config);

  EEPROM.commit();

  Serial.println("EEPROM Saved");
}


// =====================================================
// LOAD CONFIGURATION
// =====================================================

void loadConfig()
{
  Serial.println();
  Serial.println("Loading EEPROM");

  EEPROM.get(
  0,
  config);

  if(
  config.ssid[0]==0xFF ||
  config.ssid[0]=='\0')
  {
      Serial.println(
      "No EEPROM Data");

      Serial.println(
      "Loading Defaults");

      strcpy(
      config.ssid,
      DEFAULT_SSID);

      strcpy(
      config.password,
      DEFAULT_PASS);

      strcpy(
      config.mqttServer,
      "178.236.185.7");

      config.mqttPort=
      1883;

      strcpy(
      config.apiURL,
      "http://178.236.185.7:2999/v1/");

      saveConfig();
  }

  mqtt_server=
  String(
  config.mqttServer);

  baseURL=
  String(
  config.apiURL);

  Serial.println(
  "Config Loaded");

  Serial.print(
  "SSID: ");

  Serial.println(
  config.ssid);

  Serial.print(
  "MQTT Server: ");

  Serial.println(
  config.mqttServer);

  Serial.print(
  "MQTT Port: ");

  Serial.println(
  config.mqttPort);

  Serial.print(
  "API URL: ");

  Serial.println(
  config.apiURL);
}


// =====================================================
// START WIFI
//
// Starts initial WiFi connection
// =====================================================

void startWiFi()
{
  Serial.println();

  Serial.println(
  "Starting WiFi");

  WiFi.mode(
  WIFI_STA);

  Serial.print(
  "Connecting: ");

  Serial.println(
  config.ssid);

  WiFi.begin(
  config.ssid,
  config.password);

  wifiConnecting=
  true;

  wifiStartTime=
  millis();
}


// =====================================================
// MAINTAIN WIFI CONNECTION
//
// Handles:
//
// Connection
// Timeout
// Retry
// Default fallback
// =====================================================

void connectWiFi()
{
  if(
  WiFi.status()==
  WL_CONNECTED)
  {
      if(
      wifiConnecting)
      {
          Serial.println();

          Serial.println(
          "WiFi Connected");

          Serial.print(
          "IP: ");

          Serial.println(
          WiFi.localIP());

          Serial.print(
          "RSSI: ");

          Serial.println(
          WiFi.RSSI());
      }

      wifiConnecting=
      false;

      return;
  }

  unsigned long now=
  millis();

  if(
  wifiConnecting)
  {
      if(
      now-
      wifiStartTime
      >
      WIFI_TIMEOUT)
      {
          Serial.println();

          Serial.println(
          "WiFi Timeout");

          WiFi.disconnect();

          wifiConnecting=
          false;

          if(
          !triedDefaultWiFi)
          {
              Serial.println(
              "Trying Default WiFi");

              WiFi.begin(
              DEFAULT_SSID,
              DEFAULT_PASS);

              triedDefaultWiFi=
              true;

              wifiConnecting=
              true;

              wifiStartTime=
              millis();
          }
      }

      return;
  }

  if(
  now-
  lastWifiAttempt
  <
  WIFI_RETRY)
  return;

  lastWifiAttempt=
  now;

  Serial.println(
  "Retrying WiFi");

  WiFi.begin(
  config.ssid,
  config.password);

  wifiConnecting=
  true;

  wifiStartTime=
  millis();
}


// =====================================================
// MQTT CALLBACK
//
// Receives MQTT commands
//
// Example:
//
// LOCK1
// UNLOCK1
// =====================================================

void mqttCallback(
char* topic,
byte* payload,
unsigned int length)
{
  Serial.println();

  Serial.println(
  "MQTT Message Received");

  mqttCmd="";

  for(
  int i=0;
  i<length;
  i++)
  {
      mqttCmd+=
      (char)
      payload[i];
  }

  mqttCmd.trim();

  mqttCmd.toUpperCase();

  Serial.print(
  "Command: ");

  Serial.println(
  mqttCmd);
}


// =====================================================
// MQTT RECONNECT
// =====================================================

void reconnectMQTT()
{
  if(
  mqttClient.connected())
  return;

  if(
  millis()
  -
  lastMQTTConnect
  <
  5000)
  return;

  lastMQTTConnect=
  millis();

  Serial.println();

  Serial.println(
  "Connecting MQTT");

  mqttClient.setServer(
  mqtt_server.c_str(),
  config.mqttPort);

  if(
  mqttClient.connect(
  macID.c_str()))
  {
      Serial.println(
      "MQTT Connected");

      mqttClient.subscribe(
      subTopic.c_str());

      Serial.print(
      "Subscribed: ");

      Serial.println(
      subTopic);

      sendMQTT();
  }
  else
  {
      Serial.println(
      "MQTT Failed");

      Serial.print(
      "State:");

      Serial.println(
      mqttClient.state());
  }
}


// =====================================================
// API CALL
// =====================================================

int callAPI(
String qr)
{
  Serial.println();

  Serial.println(
  "Calling API");

  HTTPClient http;

  http.setTimeout(
  5000);

  String url=
  baseURL+qr;

  Serial.print(
  "URL: ");

  Serial.println(
  url);

  http.begin(
  espClient,
  url);

  int code=
  http.GET();

  Serial.print(
  "HTTP Response: ");

  Serial.println(
  code);

  http.end();

  return code;
}


// =====================================================
// SEND COMMAND TO ARDUINO
// =====================================================

void sendToArduino(
String cmd)
{
  Serial.print(
  "Send To Arduino: ");

  Serial.println(
  cmd);

  Serial.println(
  cmd);

  delay(20);
}


// =====================================================
// SEND STATUS TO MQTT
// =====================================================

void sendMQTT()
{
  if(
  !mqttClient.connected())
  return;

  int rssiVal=0;

  if(
  WiFi.status()==
  WL_CONNECTED)
  {
      rssiVal=
      WiFi.RSSI();
  }

  String payload=
  "{\"data\":{\"qr\":\""+
  lastQR+
  "\",\"status\":{\"lock1\":"+
  String(relayState)+
  ",\"status-code\":"+
  String(httpCodeVal)+
  ",\"rssi\":"+
  String(rssiVal)+
  "}}}";

  Serial.println();

  Serial.println(
  "Publishing MQTT");

  Serial.println(
  payload);

  mqttClient.publish(
  pubTopic.c_str(),
  payload.c_str());
}


// =====================================================
// PROCESS QR DATA
// =====================================================

void processSerialLine(
char* line)
{
  Serial.println();

  Serial.print(
  "Received: ");

  Serial.println(
  line);

  if(
  strncmp(
  line,
  "QR:",
  3)!=0)
  return;

  lastQR=
  String(
  line+3);

  lastQR.trim();

  if(
  lastQR.length()==0)
  return;

  sendMQTT();

  httpCodeVal=
  callAPI(
  lastQR);

  if(
  httpCodeVal>=200 &&
  httpCodeVal<300)
  {
      Serial.println(
      "QR SUCCESS");

      relayState=0;

      sendToArduino(
      "UNLOCK");

      sendToArduino(
      "QR_OK");
  }
  else
  {
      Serial.println(
      "QR FAILED");

      sendToArduino(
      "QR_NOT_OK");
  }

  sendMQTT();

  lastQR="";
}


// =====================================================
// READ SERIAL DATA
// =====================================================

void processArduinoSerial()
{
  while(
  Serial.available())
  {
      char c=
      Serial.read();

      if(c=='\r')
      continue;

      if(c=='\n')
      {
          serialLine[
          serialLen]='\0';

          processSerialLine(
          serialLine);

          serialLen=0;

          return;
      }

      if(
      serialLen<
      MAX_QR_LEN)
      {
          serialLine[
          serialLen++]=c;
      }
  }
}


// =====================================================
// PROCESS MQTT COMMAND
// =====================================================

void processMQTTCommand()
{
  if(
  mqttCmd=="LOCK1")
  {
      relayState=1;

      sendToArduino(
      "LOCK");

      sendMQTT();
  }

  else if(
  mqttCmd=="UNLOCK1")
  {
      relayState=0;

      sendToArduino(
      "UNLOCK");

      sendMQTT();
  }

  mqttCmd="";
}


// =====================================================
// SETUP
// =====================================================

void setup()
{
  Serial.begin(
  9600);

  Serial.println();

  Serial.println(
  "System Starting");

  EEPROM.begin(
  EEPROM_SIZE);

  loadConfig();

  startWiFi();

  restartTimer=
  millis();

  macID=
  WiFi.macAddress();

  macID.replace(
  ":","");

  pubTopic=
  "pub1/"+macID+"/";

  subTopic=
  "sub1/"+macID+"/";

  mqttClient.setCallback(
  mqttCallback);

  mqttClient.setBufferSize(
  512);

  relayState=0;

  sendToArduino(
  "UNLOCK");

  Serial.println(
  "Setup Complete");
}


// =====================================================
// LOOP
// =====================================================

void loop()
{
  connectWiFi();

  if(
  WiFi.status()==
  WL_CONNECTED)
  {
      reconnectMQTT();

      mqttClient.loop();
  }

  processMQTTCommand();

  processArduinoSerial();

  if(
  millis()-lastMQTT>
  2000)
  {
      lastMQTT=
      millis();

      Serial.println(
      "Periodic MQTT");

      sendMQTT();
  }

  if(
  millis()-
  restartTimer
  >=
  AUTO_RESTART_TIME)
  {
      Serial.println();

      Serial.println(
      "Restarting Device");

      delay(1000);

      ESP.restart();
  }
}

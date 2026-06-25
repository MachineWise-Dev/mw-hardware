#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <ESP8266HTTPClient.h>
#include <EEPROM.h>

#define EEPROM_SIZE 512
#define MAX_QR_LEN 200
#define WIFI_TIMEOUT 15000
#define WIFI_RETRY 5000

struct Config
{
char ssid[32];
char password[64];
char mqttServer[64];
uint16_t mqttPort;
char apiURL[120];
};

Config config;

const char* DEFAULT_SSID="MachineWise";
const char* DEFAULT_PASS="MachineWise";

WiFiClient espClient;
PubSubClient mqttClient(espClient);

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

void saveConfig()
{
 EEPROM.put(0,config);
 EEPROM.commit();
}

void loadConfig()
{
 EEPROM.get(0,config);

 if(config.ssid[0]==0xFF ||
 config.ssid[0]=='\0')
 {
   strcpy(config.ssid,DEFAULT_SSID);

   strcpy(config.password,DEFAULT_PASS);

   strcpy(config.mqttServer,
   "178.236.185.7");

   config.mqttPort=1883;

   strcpy(
   config.apiURL,
   "http://178.236.185.7:2999/v1/");

   saveConfig();
 }

 mqtt_server=
 String(config.mqttServer);

 baseURL=
 String(config.apiURL);
}

void startWiFi()
{
 WiFi.mode(WIFI_STA);

 WiFi.begin(
 config.ssid,
 config.password);

 wifiConnecting=true;

 wifiStartTime=millis();
}

void connectWiFi()
{
 if(WiFi.status()==WL_CONNECTED)
 {
   wifiConnecting=false;
   return;
 }

 unsigned long now=millis();

 if(wifiConnecting)
 {
   if(now-wifiStartTime>WIFI_TIMEOUT)
   {
      WiFi.disconnect();

      wifiConnecting=false;

      if(!triedDefaultWiFi)
      {
          WiFi.begin(
          DEFAULT_SSID,
          DEFAULT_PASS);

          triedDefaultWiFi=true;

          wifiConnecting=true;

          wifiStartTime=millis();
      }
   }

   return;
 }

 if(now-lastWifiAttempt<WIFI_RETRY)
 return;

 lastWifiAttempt=now;

 WiFi.begin(
 config.ssid,
 config.password);

 wifiConnecting=true;

 wifiStartTime=millis();
}

void mqttCallback(
char* topic,
byte* payload,
unsigned int length)
{
 mqttCmd="";

 for(int i=0;i<length;i++)
 {
   mqttCmd+=(char)payload[i];
 }

 mqttCmd.trim();
 mqttCmd.toUpperCase();
}

void reconnectMQTT()
{
 if(mqttClient.connected())
 return;

 if(millis()-lastMQTTConnect<5000)
 return;

 lastMQTTConnect=millis();

 mqttClient.setServer(
 mqtt_server.c_str(),
 config.mqttPort);

 if(mqttClient.connect(
 macID.c_str()))
 {
   mqttClient.subscribe(
   subTopic.c_str());

   sendMQTT();
 }
}

int callAPI(String qr)
{
 HTTPClient http;

 http.setTimeout(5000);

 String url=
 baseURL+qr;

 http.begin(
 espClient,
 url);

 int code=
 http.GET();

 http.end();

 return code;
}

void sendToArduino(String cmd)
{
 Serial.println(cmd);
 delay(30);
}

void sendMQTT()
{
 if(!mqttClient.connected())
 return;

 int rssiVal=0;

 if(WiFi.status()==WL_CONNECTED)
 rssiVal=WiFi.RSSI();

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

 mqttClient.publish(
 pubTopic.c_str(),
 payload.c_str());
}

void processSerialLine(char* line)
{
 if(strncmp(line,"QR:",3)!=0)
 return;

 lastQR=
 String(line+3);

 lastQR.trim();

 if(lastQR.length()==0)
 return;

 sendMQTT();

 httpCodeVal=
 callAPI(lastQR);

 if(httpCodeVal>=200 &&
 httpCodeVal<300)
 {
    relayState=0;

    sendToArduino(
    "UNLOCK");

    sendToArduino(
    "QR_OK");
 }
 else
 {
    sendToArduino(
    "QR_NOT_OK");
 }

 sendMQTT();

 delay(300);

 lastQR="";
}

void processArduinoSerial()
{
 while(Serial.available())
 {
   char c=
   Serial.read();

   if(c=='\r')
   continue;

   if(c=='\n')
   {
      serialLine[serialLen]='\0';

      processSerialLine(
      serialLine);

      serialLen=0;

      return;
   }

   if(serialLen<MAX_QR_LEN)
   {
      serialLine[serialLen++]=c;
   }
 }
}

void processMQTTCommand()
{
 if(mqttCmd=="LOCK1")
 {
   relayState=1;

   sendToArduino(
   "LOCK");

   sendMQTT();
 }

 else if(mqttCmd=="UNLOCK1")
 {
   relayState=0;

   sendToArduino(
   "UNLOCK");

   sendMQTT();
 }

 mqttCmd="";
}

void setup()
{
 Serial.begin(9600);

 EEPROM.begin(
 EEPROM_SIZE);

 loadConfig();

 startWiFi();

 macID=
 WiFi.macAddress();

 macID.replace(":","");

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
}

void loop()
{
 connectWiFi();

 if(WiFi.status()==WL_CONNECTED)
 {
   reconnectMQTT();

   mqttClient.loop();
 }

 processMQTTCommand();

 processArduinoSerial();

 if(millis()-lastMQTT>2000)
 {
   lastMQTT=
   millis();

   sendMQTT();
 }
}

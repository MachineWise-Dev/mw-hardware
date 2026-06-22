```cpp
// =====================================================
// ESP8266 QR + MQTT + API INTERLOCK SYSTEM
//
// Functions:
// 1. Connects to WiFi (2 SSIDs supported)
// 2. Connects to MQTT broker
// 3. Receives QR from ATmega
// 4. Sends QR to API
// 5. Receives LOCK/UNLOCK MQTT commands
// 6. Sends commands to ATmega
// 7. Publishes status periodically
// =====================================================

#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <ESP8266HTTPClient.h>

// =====================================================
// WIFI CREDENTIALS
// First WiFi is attempted.
// If it fails, second WiFi is attempted.
// =====================================================
const char* ssid1="MachineWise";
const char* pass1="MachineWise";

const char* ssid2="MachineWise-Electronics";
const char* pass2="Machine@123";

// =====================================================
// MQTT SERVER DETAILS
// =====================================================
const char* mqtt_server="178.236.185.7";

// =====================================================
// API BASE URL
// QR value is appended at end
// =====================================================
String baseURL="http://178.236.185.7:2999/v1/";

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

int relayState=0;
int httpCodeVal=0;

unsigned long lastMQTT=0;


// =====================================================
// CONNECT TO SINGLE WIFI
//
// Tries connection for 8 seconds
// =====================================================
void connectWiFi(const char* ssid,const char* password)
{
    Serial.println("Trying WiFi");
    Serial.print("SSID: ");
    Serial.println(ssid);

    WiFi.begin(ssid,password);

    unsigned long start=millis();

    while(WiFi.status()!=WL_CONNECTED &&
          millis()-start<8000)
    {
        Serial.print(".");
        delay(300);
    }

    Serial.println();

    if(WiFi.status()==WL_CONNECTED)
    {
        Serial.println("WiFi Connected");

        Serial.print("IP: ");
        Serial.println(WiFi.localIP());
    }
    else
    {
        Serial.println("WiFi Failed");
    }
}


// =====================================================
// TRY WIFI1 THEN WIFI2
// =====================================================
void setup_wifi()
{
    Serial.println("Starting WiFi setup");

    WiFi.mode(WIFI_STA);

    connectWiFi(ssid1,pass1);

    if(WiFi.status()==WL_CONNECTED)
        return;

    connectWiFi(ssid2,pass2);

    if(WiFi.status()==WL_CONNECTED)
        return;

    while(WiFi.status()!=WL_CONNECTED)
    {
        connectWiFi(ssid1,pass1);

        if(WiFi.status()==WL_CONNECTED)
            break;

        connectWiFi(ssid2,pass2);
    }
}


// =====================================================
// API CALL
//
// Sends QR to server
// Returns HTTP response code
// =====================================================
int callAPI(String qr)
{
    Serial.println("Calling API");

    HTTPClient http;

    String url=baseURL+qr;

    Serial.print("URL: ");
    Serial.println(url);

    http.begin(espClient,url);

    int code=http.GET();

    Serial.print("HTTP Response: ");
    Serial.println(code);

    http.end();

    return code;
}


// =====================================================
// SEND COMMAND TO ARDUINO
// =====================================================
void sendToArduino(String cmd)
{
    Serial.print("Sending To Arduino: ");
    Serial.println(cmd);

    Serial.println(cmd);

    delay(100);

    Serial.println(cmd);
}


// =====================================================
// SEND MQTT STATUS
// =====================================================
void sendMQTT()
{
    if(!client.connected())
    {
        Serial.println("MQTT not connected");
        return;
    }

    String qrToSend="";

    if(lastQR.length()>0)
        qrToSend=lastQR;
    else
        qrToSend=qrResult;

    String payload="{";
    payload+="\"data\":{";
    payload+="\"qr\":\""+qrToSend+"\",";
    payload+="\"status\":{";
    payload+="\"lock1\":"+String(relayState)+",";
    payload+="\"status-code\":"+String(httpCodeVal);
    payload+="}}}";

    Serial.println("Publishing MQTT");
    Serial.println(payload);

    if(client.publish(
        pubTopic.c_str(),
        payload.c_str()))
    {
        Serial.println("MQTT_SENT");

        sendToArduino("MQTT_BLINK");
    }
}


// =====================================================
// MQTT MESSAGE RECEIVED
// =====================================================
void callback(
char* topic,
byte* payload,
unsigned int length)
{
    Serial.println("MQTT message received");

    mqttCmd="";

    for(int i=0;i<length;i++)
    {
        mqttCmd+=(char)payload[i];
    }

    mqttCmd.trim();

    Serial.print("MQTT Command: ");
    Serial.println(mqttCmd);
}

// =====================================================
// MQTT RECONNECT
//
// Continuously attempts MQTT connection
// Uses MAC address as client ID
// =====================================================
void reconnect()
{
    Serial.println("Entering reconnect()");

    while(!client.connected())
    {
        Serial.println("Connecting MQTT...");

        Serial.print("Client ID: ");
        Serial.println(macID);

        if(client.connect(macID.c_str()))
        {
            Serial.println("MQTT Connected");

            client.subscribe(
            subTopic.c_str());

            Serial.print("Subscribed Topic: ");
            Serial.println(subTopic);

            sendMQTT();
        }
        else
        {
            Serial.println("MQTT Connection Failed");

            Serial.print("MQTT State: ");
            Serial.println(client.state());

            delay(1500);
        }
    }
}


// =====================================================
// SETUP
//
// Runs once after power ON
//
// Tasks:
// 1.Start Serial
// 2.Connect WiFi
// 3.Generate MAC ID
// 4.Create MQTT topics
// 5.Configure MQTT
// 6.Send initial unlock
// =====================================================
void setup()
{
    Serial.begin(9600);

    Serial.println();
    Serial.println("System Starting");

    delay(1000);

    setup_wifi();

    macID=WiFi.macAddress();

    Serial.print("Original MAC: ");
    Serial.println(macID);

    macID.replace(":","");

    Serial.print("Modified MAC: ");
    Serial.println(macID);

    pubTopic=
    "pub1/"+macID;

    subTopic=
    "sub1/"+macID;

    Serial.print("Publish Topic: ");
    Serial.println(pubTopic);

    Serial.print("Subscribe Topic: ");
    Serial.println(subTopic);

    client.setServer(
    mqtt_server,
    1883);

    Serial.println("MQTT Server Configured");

    client.setCallback(
    callback);

    Serial.println("MQTT Callback Attached");

    relayState=0;

    Serial.println("Relay initialized OFF");

    sendToArduino(
    "UNLOCK");

    delay(500);

    sendMQTT();

    Serial.println("Setup Complete");
}


// =====================================================
// LOOP
//
// Runs continuously forever
//
// Tasks:
//
// 1.Check MQTT connection
// 2.Process MQTT messages
// 3.Execute MQTT commands
// 4.Receive QR data
// 5.Call API
// 6.Send QR status
// 7.Publish MQTT every second
// =====================================================
void loop()
{
    // -------------------------------------
    // Ensure MQTT remains connected
    // -------------------------------------
    if(!client.connected())
    {
        Serial.println("MQTT disconnected");

        reconnect();
    }

    client.loop();


    // -------------------------------------
    // MQTT COMMAND PROCESSING
    // -------------------------------------
    if(mqttCmd.length()>0)
    {
        Serial.print("Received Command: ");
        Serial.println(mqttCmd);

        if(
        mqttCmd=="LOCK1"||
        mqttCmd=="lock1")
        {
            Serial.println("Executing LOCK");

            relayState=1;

            sendToArduino(
            "LOCK");

            sendMQTT();
        }

        else if(
        mqttCmd=="UNLOCK1"||
        mqttCmd=="unlock1")
        {
            Serial.println("Executing UNLOCK");

            relayState=0;

            sendToArduino(
            "UNLOCK");

            sendMQTT();
        }

        mqttCmd="";

        Serial.println("MQTT command cleared");
    }


    // -------------------------------------
    // QR RECEIVE SECTION
    // -------------------------------------
    if(Serial.available())
    {
        String data=
        Serial.readStringUntil('\n');

        data.trim();

        Serial.print("Serial Received: ");
        Serial.println(data);

        if(data.startsWith("QR:"))
        {
            lastQR=
            data.substring(3);

            Serial.print("QR Value: ");
            Serial.println(lastQR);

            sendMQTT();

            httpCodeVal=
            callAPI(lastQR);


            // --------------------------------
            // SUCCESS RESPONSE
            // --------------------------------
            if(httpCodeVal>=200 &&
               httpCodeVal<300)
            {
                Serial.println("QR Success");

                qrResult=
                "QR_OK";

                if(relayState==1)
                {
                    relayState=0;

                    Serial.println(
                    "Unlocking Relay");

                    sendToArduino(
                    "UNLOCK");

                    delay(250);

                    sendMQTT();
                }

                Serial.println(
                "QR_OK");

                delay(50);

                Serial.println(
                "QR_OK");
            }


            // --------------------------------
            // FAILURE RESPONSE
            // --------------------------------
            else
            {
                Serial.println(
                "QR Failed");

                qrResult=
                "QR_NOT_OK";

                Serial.println(
                "QR_NOT_OK");

                delay(50);

                Serial.println(
                "QR_NOT_OK");
            }

            lastQR="";

            Serial.println(
            "QR buffer cleared");

            sendMQTT();
        }
    }


    // -------------------------------------
    // SEND MQTT EVERY SECOND
    // -------------------------------------
    if(millis()-lastMQTT>1000)
    {
        lastMQTT=
        millis();

        Serial.println(
        "Periodic MQTT Update");

        sendMQTT();
    }
}

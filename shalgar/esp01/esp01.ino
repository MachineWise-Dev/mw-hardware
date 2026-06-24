#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>

const char* ssid1 = "SHALGAR EQUIPMENTS PVT. LTD";
const char* pass1 = "DUAO1969";

const char* ssid2 = "MachineWise-Electronics";
const char* pass2 = "Machine@123";

String baseURL = "http://178.236.185.7:10130/v1";

unsigned long lastOperatorCheck = 0;
unsigned long lastWifiBroadcast = 0;

WiFiClient espClient;

bool wifiState = false;

// ---------- SEND ----------
void sendToArduino(String msg) {
  Serial.println(msg);
}

// ---------- WIFI ----------
void connectWiFi(const char* ssid, const char* password) {

  WiFi.disconnect(true);
  delay(1000);

  WiFi.mode(WIFI_STA);

  WiFi.setSleepMode(WIFI_NONE_SLEEP);

  WiFi.begin(ssid, password);

  unsigned long start = millis();

  while (WiFi.status() != WL_CONNECTED &&
         millis() - start < 15000) {

    delay(500);
  }
}

void setup_wifi() {

  WiFi.mode(WIFI_STA);

  connectWiFi(ssid1, pass1);

  if (WiFi.status() != WL_CONNECTED) {
    connectWiFi(ssid2, pass2);
  }

  if (WiFi.status() == WL_CONNECTED) {

    wifiState = true;

    sendToArduino("WIFI,1");
  } else {

    wifiState = false;

    sendToArduino("WIFI,0");
  }
}

// ---------- CHECK ACTIVE ----------
void checkActiveOperator() {

  HTTPClient http;

  String url = baseURL + "/operator-sessions/active";

  http.begin(espClient, url);

  int code = http.GET();

  if (code == 200) {

    String response = http.getString();

    response.toUpperCase();

    String mac = WiFi.macAddress();
    mac.toUpperCase();

    if (response.indexOf(mac) >= 0) {

      sendToArduino("LOGIN,1");
    } else {

      sendToArduino("LOGIN,0");
    }
  }

  http.end();
}

// ---------- OPERATOR ----------
void handleOperator(String operatorId) {

  HTTPClient http;

  String url = baseURL + "/operator-sessions/toggle";

  http.begin(espClient, url);

  http.addHeader("Content-Type", "application/json");

  String payload =
    "{\"operatorId\":\"" + operatorId + "\",\"deviceMAC\":\"" + WiFi.macAddress() + "\"}";

  int code = http.POST(payload);

  http.end();

  if (code == 200 || code == 201) {

    checkActiveOperator();

    sendToArduino("SCAN,SUCCESS");
  } else {
    sendToArduino("SCAN,FAIL");
  }
}

// ---------- ORDER ----------
void handleOrder(String orderId) {

  HTTPClient http;

  String url = baseURL + "/scans";

  http.begin(espClient, url);

  http.addHeader("Content-Type", "application/json");

  String payload =
    "{\"orderId\":\"" + orderId + "\",\"stationName\":\"Bending 1\"}";

  int code = http.POST(payload);

  http.end();

  if (code == 200) {
    sendToArduino("SCAN,SUCCESS");
  } else {
    sendToArduino("SCAN,FAIL");
  }
}

// ---------- SETUP ----------
void setup() {

  Serial.begin(9600);

  delay(100);

  setup_wifi();

  if (WiFi.status() == WL_CONNECTED) {
    checkActiveOperator();
  }
}

// ---------- LOOP ----------
void loop() {

  // WIFI MONITOR
  bool current = WiFi.status() == WL_CONNECTED;

  if (current != wifiState) {

    wifiState = current;

    sendToArduino(wifiState ? "WIFI,1" : "WIFI,0");

    if (wifiState) {
      checkActiveOperator();
    }
  }

  if (!wifiState) {
    setup_wifi();
  }

  // QR RECEIVE
  if (Serial.available()) {

    String data = Serial.readStringUntil('\n');

    data.trim();

    if (data.startsWith("operator-")) {
      handleOperator(data.substring(9));
    } else if (data.startsWith("order-")) {
      handleOrder(data.substring(6));
    } else {
      sendToArduino("SCAN,FAIL");
    }
  }

  if (millis() - lastOperatorCheck > 10000) {

    lastOperatorCheck = millis();

    if (wifiState) {
      checkActiveOperator();
    }
  }

  if (millis() - lastWifiBroadcast > 3000) {

    lastWifiBroadcast = millis();

    sendToArduino(wifiState ? "WIFI,1" : "WIFI,0");
  }
}
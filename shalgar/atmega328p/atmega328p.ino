#include <hidboot.h>
#include <usbhub.h>
#include <SPI.h>
#include <SoftwareSerial.h>

USB Usb;
HIDBoot<USB_HID_PROTOCOL_KEYBOARD> HidKeyboard(&Usb);

SoftwareSerial espSerial(4, 7);

// ---------- LEDS ----------
#define LED_RED 8
#define LED_BLUE 6
#define LED_GREEN 5

String qrData = "";

// ---------- STATES ----------
bool wifiConnected = false;
bool blueBlinkMode = false;

unsigned long redPrev = 0;
bool redState = false;

unsigned long blueStart = 0;
unsigned long bluePrev = 0;
bool blueState = false;

// ---------- SEND QR ----------
void sendToESP(String data) {
  espSerial.println(data);
}

// ---------- KEYBOARD ----------
class KbdRptParser : public KeyboardReportParser {

  void OnKeyDown(uint8_t mod, uint8_t key) {

    char c = OemToAscii(mod, key);

    if (c) {

      if (c == '\r' || c == '\n') {

        if (qrData.length() > 3) {
          sendToESP(qrData);
        }

        qrData = "";
      } else {
        qrData += c;
      }
    }
  }
};

KbdRptParser Parser;

// ---------- SETUP ----------
void setup() {

  espSerial.begin(9600);

  pinMode(LED_RED, OUTPUT);
  pinMode(LED_BLUE, OUTPUT);
  pinMode(LED_GREEN, OUTPUT);

  digitalWrite(LED_RED, LOW);
  digitalWrite(LED_BLUE, LOW);
  digitalWrite(LED_GREEN, LOW);

  if (Usb.Init() == -1) {

    while (1) {
      digitalWrite(LED_RED, HIGH);
      delay(200);
      digitalWrite(LED_RED, LOW);
      delay(200);
    }
  }

  HidKeyboard.SetReportParser(0, &Parser);
}

// ---------- LOOP ----------
void loop() {

  Usb.Task();

  unsigned long now = millis();

  // ---------- RED LED ----------
  if (!wifiConnected) {

    if (now - redPrev >= 1000) {

      redPrev = now;

      redState = !redState;

      digitalWrite(LED_RED, redState);
    }
  } else {

    if (!redState) {

      redState = true;

      digitalWrite(LED_RED, HIGH);
    }
  }

  // ---------- BLUE LED BLINK ----------
  if (blueBlinkMode) {

    if (now - blueStart < 10000) {

      if (now - bluePrev >= 300) {

        bluePrev = now;

        blueState = !blueState;

        digitalWrite(LED_BLUE, blueState);
      }
    } else {

      blueBlinkMode = false;

      digitalWrite(LED_BLUE, LOW);
    }
  }

  // ---------- BLUE SUCCESS ----------
  else if (blueStart > 0) {

    if (now - blueStart < 10000) {
      digitalWrite(LED_BLUE, HIGH);
    } else {
      digitalWrite(LED_BLUE, LOW);
      blueStart = 0;
    }
  }

  // ---------- ESP MESSAGES ----------
  static String msg = "";

  while (espSerial.available()) {

    char c = espSerial.read();

    if (c == '\n') {

      msg.trim();

      // WIFI
      if (msg == "WIFI,1") {
        wifiConnected = true;
      } else if (msg == "WIFI,0") {
        wifiConnected = false;
      }

      // LOGIN
      else if (msg == "LOGIN,1") {

        digitalWrite(LED_GREEN, HIGH);
      } else if (msg == "LOGIN,0") {

        digitalWrite(LED_GREEN, LOW);
      }

      // SCAN SUCCESS
      else if (msg == "SCAN,SUCCESS") {

        blueBlinkMode = false;

        blueStart = millis();

        digitalWrite(LED_BLUE, HIGH);
      }

      // SCAN FAIL
      else if (msg == "SCAN,FAIL") {

        blueStart = millis();

        blueBlinkMode = true;
      }

      msg = "";
    } else {

      msg += c;
    }
  }
}
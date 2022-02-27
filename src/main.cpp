/*
    Name: M5Atom Matrix Khey
    Author: µsini
    Licence: MIT
    Version: Alpha v0.2

    E131 - Clock - Text Scroller in one package !
    --> ⌨️ 🔴🟡🟢 🎶 Use LedFX to enjoy a tiny light show sync with your music or the sound of your keyboard!
    --> ⏲ Display the current time
    --> 📟 Send text message using simple web requests (http://atom.local/text?param=Hello World)
*/

#include <Arduino.h>

// Settings are saved on SPIFFS
#include "SPIFFS.h"

// Neopixels
#include <Adafruit_GFX.h> // Generate text
#include <Fonts/TomThumb.h> // A font small enough for the matrix
#include <FastLED.h> // Control the neopixel LED
#include <FastLED_NeoMatrix.h> // Manage a neopixel Matrix

// Button
#include <ezButton.h> // Easy way to manage buttons

// Connectivity
#include <WiFi.h>
#include <ESPmDNS.h>
#include <ESPAsyncE131.h>
#include <ezTime.h>
#include <aREST.h> // Manage Rest Commands (Serial/Web)

// You can change this if you want the M5Atom Khey to connect to a WiFi network by default
String ssid = "";
String passphrase = "";

/* LED Matrix
    ⬛⬛⬛⬛
    ⬛⬛⬛⬛
    ⬛⬛⬛⬛
    ⬛⬛⬛⬛
    ⬛⬛⬛⬛
*/
#define PIN 27
#define mw 5
#define mh 5
#define NUMMATRIX (mw*mh)
int x    = mh; // Use to scroll the matrix

CRGB matrixleds[NUMMATRIX];
FastLED_NeoMatrix *matrix = new FastLED_NeoMatrix(matrixleds, mw, mh,
    NEO_MATRIX_TOP     + NEO_MATRIX_RIGHT +
    NEO_MATRIX_COLUMNS + NEO_MATRIX_PROGRESSIVE );

/* Button */
ezButton button(39); // 🔘 Button behind the LED Matrix
bool mode = false;

/* WiFi Objects */
WiFiServer server(80); // 🌐 Create a server on port 80
aREST rest = aREST();  // 🌐 Create a aREST object
ESPAsyncE131 e131(10); // 🌐 Create a E131 (DMX) server

/* String Objects */
// TODO : Use char pointer instead of String
String timeString = ""; // ⏲ Time display on led matrix
String textString = "No text"; // 📄 Text display on led matrix
bool text_received = false;

Timezone myTZ;
String tzString = "Europe/Paris"; // 🗺️ Timezone
String nameString = "atom"; // 🏷️ Name of the device

// Use freeRTOS for the button (mostly to test if this make sense)
TaskHandle_t Core0;

// 🌐 Rest Command (Serial/Web)
int textControl(String command);
int ssidControl(String command);
int passControl(String command);
int rebootControl(String command);
int timeZoneControl(String command);
int nameControl(String command);
int ssidScan(String command);

// 💾 Convert a Spiffs file into a variable (check if exists or create with default var)
// Easier than manage a config file in JSON (probably not as correct)
String SpiffsToVar(String file, String default_var) {
    String var = "";
    File f;
    if(SPIFFS.exists(file)){
      f = SPIFFS.open(file, FILE_READ);
      var = f.readString();
    } else {
      f = SPIFFS.open(file, FILE_WRITE);
      f.print(default_var);
      var = default_var;
    }
    f.close();
    return var;
}

// 💾 Setup all variables (SSID/PASS/TIMEZONE/NAME)
void spiffsSetup() {
  if (!SPIFFS.begin(true)) {

    //⚪ White Led means formatting in progress
    matrixleds[0].r = 255;
    matrixleds[0].g = 255;
    matrixleds[0].b = 255;
    FastLED.show();

    Serial.println("SPIFFS: Formatting ... Please Wait...");
    SPIFFS.format();
    ESP.restart();
  }

  Serial.println("SPIFFS: Reading settings");
  ssid = SpiffsToVar("/ssid","");
  passphrase = SpiffsToVar("/pass","");
  tzString = SpiffsToVar("/tz","Europe/Paris");
  nameString = SpiffsToVar("/name","atom");
}

// Core0 task (just for the button)
void backTask( void * pvParameters) {
  for(;;) {
    button.loop();
    // 🔘 Button pressed and released
    if (button.isReleased()) {
      if (text_received) {
        text_received = false;
      } else {
        mode = !mode;
        Serial.print("Mode: ");
        if(mode){
          Serial.println(" E131");
        } else {
          Serial.println(" Clock");
        }
      }
    }
  }
}

void setup() {
  Serial.begin(115200);

  // Setup matrix
  // ⚠️ Brightness must be kept low or the M5Atom will be really hot! 🥵
  FastLED.addLeds<NEOPIXEL, PIN>(matrixleds, NUMMATRIX);
  FastLED.setBrightness(40); // Brightness mus
  matrix->begin();
  matrix->setTextWrap(false);
  matrix->setFont(&TomThumb);
  matrix->setBrightness(40);
  matrix->setTextColor(matrix->Color(255, 0, 0));

  /* Blue Led means startup in progress
    ⬛⬛⬛🟦
    ⬛⬛⬛⬛
    ⬛⬛⬛⬛
    ⬛⬛⬛⬛
    ⬛⬛⬛⬛
  */
  matrixleds[0].r = 0;
  matrixleds[0].g = 0;
  matrixleds[0].b = 255;
  FastLED.show();

  //💾 Setup Spiffs files (use for ⚙️ settings)
  spiffsSetup();

  Serial.print("NAME:");
  Serial.println(nameString);

  // 🌐 Generate REST functions and callback
  rest.function("text", textControl);
  rest.function("ssid",ssidControl);
  rest.function("pass",passControl);
  rest.function("timezone",timeZoneControl);
  rest.function("reboot",rebootControl);
  rest.function("name",nameControl);
  rest.function("scan",ssidScan);

  // Starting WiFi
  WiFi.mode(WIFI_STA);
  Serial.print("SSID:");
  Serial.println(ssid);

  WiFi.begin(ssid.c_str(), passphrase.c_str());
  if(ssid == "") {
    Serial.println("Waiting for SSID to be set...");
  }

    /* Red Led means WiFi connection started ()
    ⬛⬛⬛🟥
    ⬛⬛⬛⬛
    ⬛⬛⬛⬛
    ⬛⬛⬛⬛
    ⬛⬛⬛⬛
  */
  matrixleds[0].r = 255;
  matrixleds[0].g = 0;
  matrixleds[0].b = 0;
  FastLED.show();

  // 📡 Wait for connection to established
  // 📲 Rest Serial is active to change settings and reboot
  while (WiFi.status() != WL_CONNECTED) {
    rest.handle(Serial);
    delay(10);
  }

  // Display IP/Name (this is used by the Web Installer)
  Serial.print("IP: ");
  Serial.println(WiFi.localIP());
  if (!MDNS.begin(nameString.c_str())) {
    Serial.println("ERROR: can't start mDNS");
    return;
  }


  // Start NTP and wait for time
  waitForSync();
  myTZ.setLocation(F(tzString.c_str()));

  // Start Arest Web
  server.begin();

  // Start E131
  if (e131.begin(E131_UNICAST))
    Serial.println("E131: READY");
  else
    Serial.println("ERROR: E131 FAILED");

  // Start Core0 Task (BackTask)
  disableCore0WDT();
  xTaskCreatePinnedToCore(backTask, "BackTask", 10000, NULL, 1, &Core0, 0);
}

void loop() {

  /* Rest API Loop */
  WiFiClient client = server.available();
  rest.handle(client);
  rest.handle(Serial);

  /* E131 Loop */
  bool newpacket = false;
  e131_packet_t packet;
 if (!e131.isEmpty()) {
  e131.pull(&packet);     // Pull packet from ring buffer
  newpacket = true;
 }

 // If text send via Rest API (we kept it visible)
 // Until someone press a key
 if (text_received) {
      matrix->setTextColor(matrix->Color(0, 255, 0));
      matrix->fillScreen(0);
      matrix->setCursor(x, mh);
      matrix->print(textString);
      int t_length = textString.length() * 4;

      if (--x < -t_length) {
        x = matrix->width();
      }

      matrix->show();
      delay(100); // ToDo replace with millis()
    } else {
      // Mode = True is E131 controller
      if (mode) {
        if (newpacket) {
          int pixel = 0;
          for (int i = 1; i <= 75; i = i + 3) {
            matrixleds[pixel].r = packet.property_values[i];
            matrixleds[pixel].g = packet.property_values[i + 1];
            matrixleds[pixel].b = packet.property_values[i + 2];
            pixel++;
          }
          FastLED.show();
        }
      } else { // Mode = False is Clock
        matrix->setTextColor(matrix->Color(0, 0, 255));
        timeString = myTZ.dateTime("H:i"); //Format Time
        matrix->fillScreen(0);
        matrix->setCursor(x, mh);
        int t_length = timeString.length() * 4;
        matrix->print(timeString);
        if (--x < -t_length) {
          x = matrix->width();
        }
        matrix->show();
        delay(100); // ToDo replace with millis()
      }
    }
  }

// Show Text http://atom.local/text?param=Hello World
int textControl(String command) {
  textString = command;
  text_received = true;
  Serial.println(textString);
  return 1;
}

// Change SSID http://atom.local/ssid?param=MySSID
int ssidControl(String command) {
  ssid = command;
  Serial.println("SSID set");
  File f = SPIFFS.open("/ssid", FILE_WRITE);
  f.print(ssid);
  f.close();
  return 1;
}

// Change Pass http://atom.local/pass?param=My Complex PassPhrase
int passControl(String command) {
  passphrase = command;
  Serial.println("Pass set");
  File f = SPIFFS.open("/pass", FILE_WRITE);
  f.print(passphrase);
  f.close();
  return 1;
}

// Change TimeZone http://atom.local/timezone?param=Europe-Paris
int timeZoneControl(String command) {
  tzString = command;
  tzString.replace("-", "/");
  Serial.println("Timezone set");
  File f = SPIFFS.open("/tz", FILE_WRITE);
  f.print(tzString);
  f.close();
  return 1;
}

// Change Name http://atom.local/name?param=atom
int nameControl(String command) {
  nameString = command;
  Serial.println("Name set");
  File f = SPIFFS.open("/name", FILE_WRITE);
  f.print(nameString);
  f.close();
  return 1;
}

// Reboot http://atom.local/reboot
int rebootControl(String command) {
  Serial.println("reboot");
  ESP.restart();
  return 1;
}

int ssidScan(String command) {
  Serial.println("Scanning SSID");
  int n = WiFi.scanNetworks();
  for(int i=0;i<n;i++) {
    Serial.print("SSIDS:");
    Serial.println(WiFi.SSID(i));
  }
  Serial.println("Scan end");
  return 1;
}
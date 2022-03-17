/*
    Name: M5Atom Matrix Khey
    Author: µsini
    Licence: MIT
    Version: Alpha v0.2

    E131 - Clock - Text Scroller in one package !
    --> ⌨️ 🔴🟡🟢 🎶 Use LedFX to enjoy a tiny light show sync with
   your music or the sound of your keyboard!
    --> ⏲ Display the current time
    --> 📟 Send text message using simple web requests
   (http://atom.local/text?param=Hello World)
*/

#include <Arduino.h>

// Button
#include <ezButton.h> // Easy way to manage buttons

// Connectivity
#include <ESPmDNS.h>
#include <WiFi.h>
#include <WiFiClient.h>

#include <ESPAsyncWebServer.h>
#include <SerialCommands.h>

#include <ESPAsyncE131.h>
#include <ezTime.h>

#include "matrix.h"
#include "settings.h"
#include "web/web_index.h"
#include "web/web_matrix.h"
#include "web/web_qrcode.h"
#include "web/web_style.h"

// You can change this if you want the M5Atom Khey to connect to a WiFi network
// by default
String ssid = "";
String passphrase = "";

/* Button */
ezButton button(39); // 🔘 Button behind the LED Matrix
bool mode = false;

AsyncWebServer web(80);
ESPAsyncE131 e131(10); // 🌐 Create a E131 (DMX) server

/* String Objects */
// TODO : Use char pointer instead of String
String timeString = "";        // ⏲ Time display on led matrix
String textString = "No text"; // 📄 Text display on led matrix
int timeUpdate = 0;            // ⏱️ Time to update the time
bool text_received = false;
bool draw_received = false;

Timezone myTZ;
String tzString = "";   // 🗺️ Timezone
String nameString = ""; // 🏷️ Name of the device

settingsManager settings;
matrixManager matrix;

void draw(String command);

/* Serial Route */

char serial_command_buffer_[256];
SerialCommands serial_commands(&Serial, serial_command_buffer_,
                               sizeof(serial_command_buffer_), "\r\n", "§");

void cmd_text(SerialCommands *sender) {
  char *text = sender->Next();
  if (text == NULL) {
    sender->GetSerial()->println("No text try --> text§Hello World");

  } else {
    matrix.setTextColor(0, 255, 0);
    matrix.setText(text);
    Serial.println("OK");
    text_received = true;
  }
}

void cmd_reboot(SerialCommands *sender) {
  Serial.println("Rebooting...");
  ESP.restart();
}

void cmd_set(SerialCommands *sender) {
  char *arg = sender->Next();
  if (arg == NULL) {
    sender->GetSerial()->println(
        "No arg set (ssid/pass/name/tz) --> set§ssid§myssid");
  } else {
    char *value = sender->Next();
    settings.write("/" + String(arg), value);
  }
}

void cmd_scan(SerialCommands *sender) {
  int n = WiFi.scanNetworks();
  for (int i = 0; i < n; i++) {
    Serial.println("SSIDS:" + WiFi.SSID(i));
  }
  Serial.println("Scan end");
}

void cmd_draw(SerialCommands *sender) {
  char *command = sender->Next();
  if (command == NULL) {
    sender->GetSerial()->println(
        "No draw value --> draw§1111111111111111111111111");
  } else {
    draw(command);
    draw_received = true;
  }
}

void cmd_unrecognized(SerialCommands *sender, const char *cmd) {
  sender->GetSerial()->print("Unrecognized command [");
  sender->GetSerial()->print(cmd);
  sender->GetSerial()->println("]");
}

SerialCommand serialText("text", cmd_text);
SerialCommand serialReboot("reboot", cmd_reboot);
SerialCommand serialSet("set", cmd_set);
SerialCommand serialScan("scan", cmd_scan);
SerialCommand serialDraw("draw", cmd_draw);

void createSerialRoute() {
  serial_commands.SetDefaultHandler(cmd_unrecognized);
  serial_commands.AddCommand(&serialText);
  serial_commands.AddCommand(&serialReboot);
  serial_commands.AddCommand(&serialSet);
  serial_commands.AddCommand(&serialScan);
  serial_commands.AddCommand(&serialDraw);
}

/* Web Route */

void createRoute() {

  web.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
    AsyncWebServerResponse *response = request->beginResponse_P(
        200, "text/html", APP_INDEX, sizeof(APP_INDEX));
    response->addHeader("content-encoding", "gzip");
    request->send(response);
  });

  web.on("/qrcode.min.js", HTTP_GET, [](AsyncWebServerRequest *request) {
    AsyncWebServerResponse *response = request->beginResponse_P(
        200, "application/javascript", APP_QRCODE, sizeof(APP_QRCODE));
    response->addHeader("content-encoding", "gzip");
    request->send(response);
  });

  web.on("/matrix.js", HTTP_GET, [](AsyncWebServerRequest *request) {
    AsyncWebServerResponse *response = request->beginResponse_P(
        200, "application/javascript", APP_MATRIX, sizeof(APP_MATRIX));
    response->addHeader("content-encoding", "gzip");
    request->send(response);
  });

  web.on("/style.css", HTTP_GET, [](AsyncWebServerRequest *request) {
    AsyncWebServerResponse *response =
        request->beginResponse_P(200, "text/css", APP_STYLE, sizeof(APP_STYLE));
    response->addHeader("content-encoding", "gzip");
    request->send(response);
  });

  web.on("/reboot", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(200, "text/plain", "OK");
    ESP.restart();
  });

  web.on("/type", HTTP_GET, [](AsyncWebServerRequest *request) {
    #ifdef M5ATOM
    const char* type = "M5Atom";
    #else
    const char* type = "MATRIX256";
    #endif
    AsyncWebServerResponse *response =
        request->beginResponse_P(200, "text/plain", type);
    response->addHeader("Access-Control-Allow-Origin", "*");
    response->addHeader("Access-Control-Expose-Headers", "*");
    response->addHeader("Access-Control-Allow-Credentials", "true");
    request->send(response);
    request->send(response);
  });

  web.on("/mode", HTTP_GET, [](AsyncWebServerRequest *request) {
    if(request->hasParam("clock")) {
      text_received = false;
      draw_received = false;
      matrix.setTextColor(0, 0, 255);
      mode = false;
    } else if(request->hasParam("e131")){
      text_received = false;
      draw_received = false;
      mode = true;
    } else {
      text_received = false;
      draw_received = false;
    }
    request->send(200, "text/plain", "OK");
  });

  web.on("/show", HTTP_GET, [](AsyncWebServerRequest *request) {
    if (request->hasParam("text")) {
      matrix.setTextColor(0, 255, 0);
      matrix.setText(request->getParam("text")->value());
      Serial.println(request->getParam("text")->value());
      text_received = true;
      request->send(200, "text/plain", "OK");
    } else {
      request->send(200, "text/plain", "No arg -> /show?text=hello world");
    }
  });

  web.on("/draw", HTTP_GET, [](AsyncWebServerRequest *request) {
    const char *answer = "No args -> /draw?pixels=1";
    if (request->hasParam("pixels")) {
      draw(request->getParam("pixels")->value());
      answer = "DRAW OK";
    }
    AsyncWebServerResponse *response =
        request->beginResponse_P(200, "text/plain", answer);
    response->addHeader("Access-Control-Allow-Origin", "*");
    response->addHeader("Access-Control-Expose-Headers", "*");
    response->addHeader("Access-Control-Allow-Credentials", "true");
    request->send(response);
  });

  web.on("/set", HTTP_GET, [](AsyncWebServerRequest *request) {
    if (request->hasParam("ssid")) {
      settings.write("/ssid", request->getParam("ssid")->value());
      request->send(200, "text/plain", "SSID SET");
    }

    if (request->hasParam("pass")) {
      settings.write("/pass", request->getParam("pass")->value());
      request->send(200, "text/plain", "PASS SET");
    }

    if (request->hasParam("name")) {
      settings.write("/name", request->getParam("name")->value());
      request->send(200, "text/plain", "NAME SET");
    }

    if (request->hasParam("tz")) {
      String tzString = request->getParam("tz")->value();
      settings.write("/tz", tzString);
      request->send(200, "text/plain", "TZ SET");
    }
  });

  web.on("/scan", HTTP_GET, [](AsyncWebServerRequest *request) {
    int n = WiFi.scanNetworks();
    String ssids = "";
    for (int i = 0; i < n; i++) {
      String ssids = "SSIDS:" + WiFi.SSID(i) + "\n";
    }
    request->send(200, "text/plain", ssids);
  });

  web.begin();
}

void setup() {
  Serial.begin(115200);
  button.setDebounceTime(100);
  matrix.begin(40); // 🔘 Initialize the LED Matrix (r,g,b,brightness);
  matrix.setTextColor(0, 0, 255);

  /* Blue Led means startup in progress
    ⬛⬛⬛🟦
    ⬛⬛⬛⬛
    ⬛⬛⬛⬛
    ⬛⬛⬛⬛
    ⬛⬛⬛⬛
  */
  matrix.blue();

  //💾 Setup Spiffs files (use for ⚙️ settings)
  Serial.println("SPIFFS: Reading settings");
  settings.begin();
  ssid = settings.read("/ssid", "");
  passphrase = settings.read("/pass", "");
  tzString = settings.read("/tz", "");
  nameString = settings.read("/name", "atom");

  Serial.print("NAME:");
  Serial.println(nameString);

  createSerialRoute();

  // Starting WiFi
  WiFi.mode(WIFI_STA);
  Serial.print("SSID:");
  Serial.println(ssid);

  WiFi.begin(ssid.c_str(), passphrase.c_str());
  if (ssid == "") {
    Serial.println("Waiting for SSID to be set...");
  }

  /* Red Led means WiFi connection started ()
  ⬛⬛⬛🟥
  ⬛⬛⬛⬛
  ⬛⬛⬛⬛
  ⬛⬛⬛⬛
  ⬛⬛⬛⬛
*/
  matrix.red();

  // 📡 Wait for connection to established
  // 📲 Rest Serial is active to change settings and reboot
  while (WiFi.status() != WL_CONNECTED) {
    serial_commands.ReadSerial();
    delay(10);
  }

  createRoute();

  // Display IP/Name (this is used by the Web Installer)
  Serial.print("IP: ");
  Serial.println(WiFi.localIP());
  if (!MDNS.begin(nameString.c_str())) {
    Serial.println("ERROR: can't start mDNS");
    return;
  }

  // Start NTP and wait for time
  waitForSync();
  Serial.print("TZ:");
  Serial.println(tzString);
  myTZ.setLocation(F(tzString.c_str()));

  // Start E131
  if (e131.begin(E131_UNICAST))
    Serial.println("E131: READY");
  else
    Serial.println("ERROR: E131 FAILED");
}

void loop() {
  button.loop();
  serial_commands.ReadSerial();

  // 🔘 Button pressed and released
  if (button.isReleased()) {
    if (text_received) {
      Serial.println("Disabled text");
      matrix.setTextColor(0, 0, 255);
      timeString = myTZ.dateTime("H:i");
      matrix.setText(timeString);
      text_received = false;
    } else if (draw_received) {
      Serial.println("Disabled draw");
      matrix.setTextColor(0, 0, 255);
      timeString = myTZ.dateTime("H:i");
      matrix.setText(timeString);
      draw_received = false;
    } else {
      mode = !mode;
      Serial.print("Mode: ");
      if (mode) {
        Serial.println(" E131");

      } else {
        Serial.println(" Clock");
        matrix.setTextColor(0, 0, 255);
      }
    }
  }

  if (draw_received) {
  } else if (text_received) {
    matrix.scroll();
  } else {
    // Mode = True is E131 controller
    if (mode) {
      e131_packet_t packet;
      if (!e131.isEmpty()) {
        e131.pull(&packet);
        int pixel = 0;
        for (int i = 1; i <= NUMMATRIX * 3; i = i + 3) {
          matrix.setPixel(pixel, packet.property_values[i],
                          packet.property_values[i + 1],
                          packet.property_values[i + 2]);
          pixel++;
        }
        matrix.update();
      }
    } else { // Mode = False is Clock
      if (millis() > timeUpdate) {
        timeString = myTZ.dateTime("H:i");
        matrix.setText(timeString);
        timeUpdate = millis() + 1000;
      }
      matrix.scroll();
    }
  }
}

void draw(String command) {
  Serial.println("Drawing...");
  Serial.println(command);
  matrix.clear();
  matrix.draw(command);
  draw_received = true;
  matrix.update();
  Serial.println("DRAW OK");
}

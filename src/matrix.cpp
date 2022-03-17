#include "matrix.h"

// Neopixels
CRGB leds[NUMMATRIX];

#ifdef M5ATOM
FastLED_NeoMatrix *matrixFASTLED =
    new FastLED_NeoMatrix(leds, mw, mh,
                          NEO_MATRIX_TOP + NEO_MATRIX_RIGHT +
                              NEO_MATRIX_COLUMNS + NEO_MATRIX_PROGRESSIVE);
#else
FastLED_NeoMatrix *matrixFASTLED =
    new FastLED_NeoMatrix(leds, mw, mh,
                          NEO_MATRIX_BOTTOM + NEO_MATRIX_RIGHT +
                              NEO_MATRIX_COLUMNS + NEO_MATRIX_ZIGZAG);
#endif

void matrixManager::setSpeed(int speed) { matrixManager::speed = speed; }

void matrixManager::setTextColor(int r, int g, int b) {
  matrixFASTLED->setTextColor(matrixFASTLED->Color(r, g, b));
}

void matrixManager::setText(String text) { matrixManager::displayText = text; }

void matrixManager::begin(int brightness) {
  // Setup matrix
  // ⚠️ Brightness must be kept low or the M5Atom will be really hot! 🥵
  FastLED.addLeds<NEOPIXEL, PIN>(leds, NUMMATRIX);

  #ifdef M5ATOM
  FastLED.setBrightness(40); // Brightness
  #endif
  #ifdef MATRIX256
  FastLED.setMaxPowerInMilliWatts(1000);
  #endif

  matrixFASTLED->begin();
  matrixFASTLED->setTextWrap(false);
  matrixFASTLED->setFont(&TomThumb);
  matrixFASTLED->setBrightness(brightness);
}

void matrixManager::blue() {
  leds[0].r = 0;
  leds[0].g = 0;
  leds[0].b = 255;
  FastLED.show();
}

void matrixManager::update() { FastLED.show(); }

void matrixManager::setPixel(int pixel, int r, int g, int b) {
  leds[pixel].r = r;
  leds[pixel].g = g;
  leds[pixel].b = b;
}

void matrixManager::red() {
  leds[0].r = 255;
  leds[0].g = 0;
  leds[0].b = 0;
  FastLED.show();
}

void matrixManager::clear() { matrixFASTLED->fillScreen(0); }

void matrixManager::scroll() {
  if (millis() > matrixManager::nextUpdate) {
    int t_length = matrixManager::displayText.length() * 4;
    bool noscroll = false;
    #ifdef MATRIX256
        if (t_length <= 20) {
          noscroll = true;
          matrixFASTLED->fillScreen(0);
          matrixFASTLED->setCursor(0, mh);
          matrixFASTLED->print(matrixManager::displayText);
        }
    #endif


    if (!noscroll) {
      matrixFASTLED->fillScreen(0);
      matrixFASTLED->setCursor(matrixManager::x, mh);
      matrixFASTLED->print(matrixManager::displayText);
      if (--matrixManager::x < -t_length) {
        matrixManager::x = matrixFASTLED->width();
      }
    }
    matrixFASTLED->show();
    matrixManager::nextUpdate = millis() + matrixManager::speed;
  }
}

void matrixManager::draw(String command) {
  for (int i = 0; i < NUMMATRIX; i++) {
    char c = command.charAt(i);
    int value = c - '0';
    switch (value) {
    case 0:
      matrixManager::setPixel(i, 0, 0, 0);
      break;
    case 1:
      matrixManager::setPixel(i, 255, 0, 0);
      break;
    case 2:
      matrixManager::setPixel(i, 0, 255, 0);
      break;
    case 3:
      matrixManager::setPixel(i, 0, 0, 255);
      break;
    case 4:
      matrixManager::setPixel(i, 255, 255, 0);
      break;
    case 5:
      matrixManager::setPixel(i, 255, 165, 0);
      break;
    case 6:
      matrixManager::setPixel(i, 255, 0, 255);
      break;
    case 7:
      matrixManager::setPixel(i, 0, 255, 255);
      break;
    case 8:
      matrixManager::setPixel(i, 255, 255, 255);
      break;
    }
  }
}
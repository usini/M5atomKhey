#include <Adafruit_GFX.h>      // Generate text
#include <FastLED.h>           // Control the neopixel LED
#include <FastLED_NeoMatrix.h> // Manage a neopixel Matrix
#include <Fonts/TomThumb.h>    // A font small enough for the matrix

/* LED Matrix
    ⬛⬛⬛⬛
    ⬛⬛⬛⬛
    ⬛⬛⬛⬛
    ⬛⬛⬛⬛
    ⬛⬛⬛⬛
*/

#ifdef M5ATOM
#define PIN 27
#define mw 5
#define mh 5
#define NUMMATRIX (mw * mh)
#else
#define PIN 16
#define mw 16
#define mh 16
#define NUMMATRIX (mw * mh)
#endif

class matrixManager {
private:
  int x = mh; // Use to scroll the matrix
  String displayText = "";
  int speed = 100;
  long nextUpdate = 0;

public:
  void begin(int brightness);
  void setSpeed(int speed);
  void setPixel(int pixel, int r, int g, int b);
  void setText(String text);
  void setTextColor(int r, int g, int b);
  void blue();
  void red();
  void scroll();
  void update();
  void clear();
  void draw(String command);
};

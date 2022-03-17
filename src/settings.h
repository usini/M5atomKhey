#include "SPIFFS.h"

class settingsManager {
private:
public:
  void begin();
  String read(String file, String value);
  void write(String file, String value);
};

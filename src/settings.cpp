#include "settings.h"

// 💾 Convert a Spiffs file into a variable (check if exists or create with default var)
// Easier than manage a config file in JSON (probably not as correct)
String settingsManager::read(String file, String default_value)
{
    String value = "";
    File f;
    if (SPIFFS.exists(file))
    {
        f = SPIFFS.open(file, FILE_READ);
        value = f.readString();
    }
    else
    {
        f = SPIFFS.open(file, FILE_WRITE);
        f.print(default_value);
        value = default_value;
    }
    f.close();
    return value;
}

void settingsManager::write(String file, String value)
{
    File f = SPIFFS.open(file, FILE_WRITE);
    f.print(value);
    f.close();
    Serial.print(file);
    Serial.print(" --> ");
    Serial.println(value);
    Serial.print(file);
    Serial.println(" SET");
}

// 💾 Setup all variables (SSID/PASS/TIMEZONE/NAME)
void settingsManager::begin()
{
    if (!SPIFFS.begin(true))
    {
        Serial.println("SPIFFS: Formatting ... Please Wait...");
        SPIFFS.format();
        ESP.restart();
    }
}
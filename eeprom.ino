#include "config.h"
#include "eeprom.h"
#ifdef USE_EEPROM
#include <FS.h>
#include "SPIFFS.h"
#include <CRC32.h>
#include <ArduinoJson.h>
#endif

E_config e_config = {
  .mail_time = 0,
  .crc = 0
};

#ifdef USE_EEPROM
void writeConfig()
{
  e_config.crc = CRC32::calculate(&e_config, sizeof(E_config) - sizeof(uint16_t));
  File file = SPIFFS.open("/config.json", FILE_WRITE);
  if (file && !file.isDirectory()) {
    SpiRamJsonBuffer jsonBuffer;

    // Parse the root object
    JsonObject &root = jsonBuffer.createObject();
    root["mail_time"] = e_config.mail_time;
    root.printTo(file);
    file.close();
    jsonBuffer.clear();
  }
}

boolean readConfig()
{
  boolean res = false;
  if (SPIFFS.begin(FORMAT_SPIFFS_IF_FAILED))
  {
    File file = SPIFFS.open("/config.json");
    if (file && !file.isDirectory()) {
      SpiRamJsonBuffer jsonBuffer;

      // Parse the root object
      JsonObject &root = jsonBuffer.parseObject(file);
      if (root.success()) {
        Serial.println(F("Failed to read file, using default configuration"));

        e_config.mail_time = root["mail_time"];
        res = true;
      }

      file.close();
      jsonBuffer.clear();
    }
  }
  return res;
}

void writeWorkTime()
{
  time_t now = time(nullptr);

  File file = SPIFFS.open("/work.json", FILE_WRITE);
  
  if (file && !file.isDirectory()) {
    SpiRamJsonBuffer jsonBuffer;

    // Parse the root object
    JsonObject &root = jsonBuffer.createObject();
    root["time"] = now;
    root["duration"] = elapsedWork();
    root.printTo(file);
    file.close();
    jsonBuffer.clear();
  }
}
#endif

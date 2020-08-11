#include "config.h"
#include <time.h>
#ifdef USE_EEPROM
#include <FS.h>
#endif

void setupTime(void *pvParameters)  // This is a task.
{
  (void) pvParameters;

  configTime(3 * 3600, 0, "ru.pool.ntp.org");

  serialTake();
  Serial.print("Waiting for NTP time sync: ");
  serialGive();
  time_t now = time(nullptr);
  int cnt = 120;
  while ((now < 8 * 3600 * 2) && (cnt-- > 0)) {
    delay(250);
    Serial.print(".");
    now = time(nullptr);
    digitalWrite(ledGreen, !digitalRead(ledGreen));
  }
  Serial.println("");
  struct tm timeinfo;
  gmtime_r(&now, &timeinfo);
  serialTake();
  Serial.print("Current time: ");
  Serial.println(asctime(&timeinfo));

#ifdef DEBUG_STACK_SIZE
  Serial.printf("Stack TIME: %u\n", uxTaskGetStackHighWaterMark( NULL ));
#endif  
  serialGive(); 
#ifdef USE_EEPROM
  File file = SPIFFS.open("/work.json", FILE_READ);
  
  if (file && !file.isDirectory()) {
    SpiRamJsonBuffer jsonBuffer;

    // Parse the root object
    JsonObject &root = jsonBuffer.parseObject(file);
    if (root.success()) {
      time_t time_ = root["time"];
      uint32_t duration = root["duration"];
      if ((time_!=0) && (duration>0)) {
        time_t now = time(nullptr);
        
        Serial.printf("Saved %u sec.\n", time_);
        Serial.printf("Current %u sec.\n", now);
        
        if (time_+(duration)>now) {
          uint32_t onTime = ((time_+(duration))-now);
          startWork(onTime);
          
          serialTake();
          Serial.printf("Work recovered %u sec.\n", onTime);
          serialGive();
        } else
          clearWorkTime();
      }
    }
    file.close();
    jsonBuffer.clear();
  }
#endif
  vTaskDelete(NULL);

  while (1);
}

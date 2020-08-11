//To use send Email for Gmail to port 465 (SSL), less secure app option should be enabled. https://myaccount.google.com/lesssecureapps?pli=1

//To receive Email for Gmail, IMAP option should be enabled. https://support.google.com/mail/answer/7126229?hl=en

#if CONFIG_FREERTOS_UNICORE
#define ARDUINO_RUNNING_CORE 0
#else
#define ARDUINO_RUNNING_CORE 1
#endif

#include "config.h"
#include <Arduino.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <time.h>
#include <CTBot.h>
#include "ESP32_MailClient.h"
#include "esp_task_wdt.h"
#include "eeprom.h"

const uint8_t ledGreen = 21;
const uint8_t ledRed = 22;

static TaskHandle_t botTask = NULL;
static TaskHandle_t mailTask = NULL;
static TaskHandle_t workTask = NULL;
static TimerHandle_t timerMailCheck = NULL;
static TimerHandle_t timerWork = NULL;
static SemaphoreHandle_t hMutex = NULL;
static SemaphoreHandle_t hSerialMutex = NULL;

void serialTake();
void serialGive();

void clearWorkTime()
{
    SPIFFS.remove("/work.json");
}

void vWorkTimerCB( TimerHandle_t xTimer )
{
  xTaskNotify(workTask, 1, eSetBits);
}

void startWorkTimer(void)
{
  if (timerWork == NULL) {
    timerWork = xTimerCreate("timerWork", pdMS_TO_TICKS(1000), false, NULL, vWorkTimerCB);
  }
  xTimerStart( timerWork, 0 );
}

void vCallbackFunction( TimerHandle_t xTimer )
{
  xTaskNotify(mailTask, 1, eSetBits);
}

void processWDT(void *pvParameters)
{
  (void) pvParameters;
  while (1) {
    esp_task_wdt_reset();
    vTaskDelay(1000);
  }
}

void setup()
{
  Serial.begin(115200);
  Serial.println();

  pinMode(ledGreen, OUTPUT);
  pinMode(ledRed, OUTPUT);
  digitalWrite(ledRed, LOW);

  Serial.printf("Internal Total heap %d, internal Free Heap %d\n", ESP.getHeapSize(), ESP.getFreeHeap());
  Serial.printf("SPIRam Total heap %d, SPIRam Free Heap %d\n", ESP.getPsramSize(), ESP.getFreePsram());
  Serial.printf("ChipRevision %d, Cpu Freq %d, SDK Version %s\n", ESP.getChipRevision(), ESP.getCpuFreqMHz(), ESP.getSdkVersion());
  Serial.printf("Flash Size %d, Flash Speed %d\n", ESP.getFlashChipSize(), ESP.getFlashChipSpeed());
#if defined(BOARD_HAS_PSRAM)
  Serial.printf("Board has PSRAM\n");
#else
  Serial.printf("Board doesn't have PSRAM\n");
#endif

#ifdef USE_EEPROM
  readConfig();
#endif  

  if (e_config.mail_time <= 0) e_config.mail_time = MAIL_REQUEST_INTERVAL;

connect:
  Serial.printf("Connecting to AP %s", WIFI_SSID);

  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  int cnt = 0;
  while (WiFi.status() != WL_CONNECTED)
  {
    Serial.print(".");
    delay(200);
    digitalWrite(ledGreen, !digitalRead(ledGreen));
    if (cnt++ > 10) {
      Serial.println("Wrong Password? Retry.");
      Serial.setDebugOutput(true);
      goto connect;
    }
  }
  Serial.println("");
  Serial.println("WiFi connected.");
  Serial.print("IP address: ");  Serial.println(WiFi.localIP());
  Serial.println();

  hSerialMutex = xSemaphoreCreateMutex();

  if (!hSerialMutex) {
    while (1);
  }

  xTaskCreatePinnedToCore(setupTime,  "setupTime",  10*1024,  NULL,  1  ,  NULL ,  ARDUINO_RUNNING_CORE);
  xTaskCreatePinnedToCore(processBot,  "botTask",  12 * 1024,  NULL,  2,  &botTask ,  ARDUINO_RUNNING_CORE);
  xTaskCreatePinnedToCore(processMail,  "mailTask",  24 * 1024,  NULL,  2,  &mailTask ,  ARDUINO_RUNNING_CORE);
  xTaskCreatePinnedToCore(processWork,  "workTask",  10 * 1024,  NULL,  2,  &workTask , ARDUINO_RUNNING_CORE);
  xTaskCreatePinnedToCore(processWDT,  "WDTTask",  1024,  NULL,  1,  NULL , ARDUINO_RUNNING_CORE);  
  
  timerMailCheck = xTimerCreate("checkMail", pdMS_TO_TICKS(e_config.mail_time * 1000), false, NULL, vCallbackFunction);
  xTimerStart( timerMailCheck, 0 );
}

void loop()
{  
  vTaskDelete(NULL);
  while (1);
}

void serialTake()
{
  xSemaphoreTake(hSerialMutex, portMAX_DELAY);
}

void serialGive()
{
  xSemaphoreGive(hSerialMutex);
}

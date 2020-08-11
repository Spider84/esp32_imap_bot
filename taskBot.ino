#include "config.h"

static String botMessage = "";

void botSendMessage(String msg);

void processBot(void *pvParameters)
{
  (void) pvParameters;

  //Костыль чтобы класс бота оказался в PSRAM
  void *BotMem = my_malloc(sizeof(CTBot));
  CTBot *myBot = new (BotMem) CTBot();

  serialTake();
  Serial.print("Starting Telegram Bot... ");
  serialGive();
  myBot->setTelegramToken(TLG_BOT_TOKEN);
  myBot->useDNS(true);

  if (myBot->testConnection()) {
    serialTake();
    Serial.println("Bot OK");
    serialGive();
    digitalWrite(ledGreen, HIGH);

    uint64_t chipid = ESP.getEfuseMac();
    myBot->sendMessage(TLG_CHAT_ID, String(F("QR Started ")) + String((uint32_t)chipid, DEC));
  }
  else {
    serialTake();
    Serial.println("Bot connection ERROR!");
    serialGive();
    digitalWrite(ledGreen, LOW);
    for (uint8_t i = 0; i < 10; i++) {
      if (i & 1)
        digitalWrite(ledGreen, LOW);
      else
        digitalWrite(ledGreen, HIGH);
      delay(400);
    }
  }

  TBMessage *msg = (TBMessage *)my_malloc(sizeof(TBMessage));

  while (1) {
#ifdef DEBUG_STACK_SIZE
    serialTake();
    Serial.printf("Stack BOT: %u\n", uxTaskGetStackHighWaterMark( NULL ));
    serialGive();
#endif    
    if (myBot->getNewMessage(*msg)) {
      String text = msg->text;
      text.toLowerCase();
      if (text.compareTo("state") == 0) {
        if (isWork()) {
          myBot->sendMessage(TLG_CHAT_ID, String(F("On time: ")) + elapsedWork());
        } else
        {
          myBot->sendMessage(TLG_CHAT_ID, F("Idle"));
        }
      } else if (text.startsWith("turnon ")) {        
        if (isWork()) {
          myBot->sendMessage(TLG_CHAT_ID, F("Already switched on"));
        } else {
          String number = msg->text.substring(7);
          int work_time = number.toInt();
          
          startWork(work_time);
#ifdef USE_EEPROM          
          writeWorkTime();
#endif  
          Serial.println(F("Manual switch On"));          
          myBot->sendMessage(TLG_CHAT_ID, F("Switched on"));
        }
      } else if (text.compareTo("turnoff") == 0) {
        if (isWork()) {
          stopWork();
          Serial.println(F("Manual switch Off"));
          myBot->sendMessage(TLG_CHAT_ID, F("Switched off"));
        } else {
          myBot->sendMessage(TLG_CHAT_ID, F("Already switched off"));
        }
      }
      text = "";
    }
    uint32_t value = 0;
    if (xTaskNotifyWait(0, 1, &value, 1000) == pdPASS) {
      if (value & 1) {
        for (int i = 0; (i < TLG_SEND_RET) && (!myBot->sendMessage(TLG_CHAT_ID, botMessage)); i++) vTaskDelay(1000);
        botMessage = "";
        xSemaphoreGive (hMutex);
      }
    }
  }
}

void botSendMessage(String msg)
{
  if (botTask) {
    if (hMutex == NULL) {
      hMutex = xSemaphoreCreateMutex ();
    }
    if (xSemaphoreTake (hMutex, 1000) == pdPASS) {
      botMessage = msg;
      xTaskNotify(botTask, 1, eSetBits);
    }
  }
}

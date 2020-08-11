#include "config.h"

uint32_t onTime = 0;
static const char work_end[] = "Work end.";

void startWork(uint32_t work_time)
{
  onTime = work_time;
  digitalWrite(ledRed, HIGH);
  startWorkTimer();
}

void stopWork()
{
   onTime = 0;
   vWorkTimerCB(NULL);
}

boolean isWork()
{
  return (onTime!=0);
}

unsigned int elapsedWork()
{
  return onTime;
}

void processWork(void *pvParameters)
{
  (void) pvParameters;
  uint32_t lastSend = 0;
  while (1) {
    uint32_t value = 0;
    if ((xTaskNotifyWait(0, 1, &value, portMAX_DELAY) == pdPASS) && (onTime)) {
      if (--onTime == 0) {
        lastSend = 0;
        serialTake();
        Serial.println(work_end);
        serialGive();
        digitalWrite(ledRed, LOW);
        botSendMessage(work_end);

        xTimerDelete( timerWork, portMAX_DELAY);
        timerWork = NULL;
        clearWorkTime();
        continue;
      }
      startWorkTimer();
      if ((lastSend==0) || (lastSend-onTime>=60)) {
        String msg = "Work elapsed " + String(onTime, DEC);
        Serial.println(msg);
        botSendMessage(msg);
        msg = "";
        lastSend = onTime;
      }
    }
#ifdef DEBUG_STACK_SIZE    
    serialTake();
    Serial.printf("Stack WORK: %u\n", uxTaskGetStackHighWaterMark( NULL ));
    serialGive();
#endif    
  }
}

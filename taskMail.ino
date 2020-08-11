#include "config.h"

static String messageUID = "";
const static String qr_filter = QR_NUMBER;

//The Email Reading data object contains config and data that received
IMAPData *imapData = NULL;

void readCallback(ReadStatus info);
void readEmail();

static const char pay_detect[] = "Payment detected!";

void processMail(void *pvParameters)
{
  (void) pvParameters;

  void *imapMem = my_malloc(sizeof(IMAPData));
  imapData = new (imapMem) IMAPData();

  imapData->setLogin(IMAP_SERVER, 993, MAIL_USER, MAIL_PASS);
  imapData->setFolder("INBOX");

  //Save attachament
  imapData->setDownloadAttachment(false);

  //Set fetch/search result to return html message
  imapData->setHTMLMessage(true);

  //Set fetch/search result to return text message
  imapData->setTextMessage(true);

  //Set the maximum result when search criteria was set.
  imapData->setSearchLimit(1);

  //Set the return tex/html message size in byte.
  imapData->setMessageBufferSize(512);

  //Set to get attachment downloading progress status.
  imapData->setDownloadReport(true);

  imapData->setFileStorageType(MailClientStorageType::SPIFFS);

  imapData->setDebug(false);

  while (1) {
    uint32_t value = 0;
    if (xTaskNotifyWait(0, 1, &value, portMAX_DELAY) == pdPASS) {
      serialTake();
#ifdef DEBUG_STACK_SIZE
      Serial.printf("Stack MAIL: %u\n", uxTaskGetStackHighWaterMark( NULL ));
      Serial.printf("Heap size: %d\r\n", ESP.getFreeHeap());
#endif
      Serial.println("\r\nRead Email...");
      serialGive();

      while (1) {
        messageUID = "";
        imapData->setFetchUID(messageUID);
        imapData->setSearchCriteria("UID SEARCH NOT SEEN"); //AND NOT DELETED
        imapData->setReadCallback(searchCallback);
        MailClient.readMail(*imapData);
        if (messageUID.length() > 0) {
          if (!isWork()) {
            imapData->setReadCallback(readCallback);
            imapData->setFetchUID(messageUID);
            MailClient.readMail(*imapData);
          }
          imapData->setReadCallback(NULL);
          MailClient.addFlag(*imapData, messageUID.toInt(), "\\Seen \\Deleted");
        } else
          break;
      }

      xTimerChangePeriod(timerMailCheck, pdMS_TO_TICKS(10000), portMAX_DELAY);
    }
  }
}

//Callback function to get the Email reading status
void readCallback(ReadStatus msg)
{
  //Show the result when reading finished
  if (msg.success() && imapData->availableMessages())
  {
    serialTake();
    Serial.println("=================");

    //Search result number which varied upon search crieria
    Serial.println("Messsage Number: " + imapData->getNumber(0));
    Serial.println("Messsage UID: " + imapData->getUID(0));
    serialGive();

    //If setHeaderOnly to false;
    if (!imapData->isHeaderOnly())
    {
      String msg = imapData->getTextMessage(0);
      serialTake();
      Serial.println("Text Message: " + msg);
      Serial.println("Text Message Charset: " + imapData->getTextMessgaeCharset(0));
      if (imapData->isFetchMessageFailed(0))
        Serial.println("Fetch Error: " + imapData->getFetchMessageFailedReason(0));
      serialGive();

      static char pBuf[128];
      unsigned int nBytes = 0;
      boolean nextline = false;
      unsigned int summ = 0;
      boolean found = false;
      String qr;

      for (unsigned int n = 0; (n < msg.length()) && (!found); n++)
      {
        char c = msg[n];
        //#ifdef DEBUG_MAIL
        //        if (c < 0x20) {
        //          Serial.print('<');
        //          Serial.print((byte)c);
        //          Serial.print('>');
        //        }
        //        Serial.print(c);
        //#endif
        if (c == '\n' || c == '\r') {
          pBuf[nBytes++] = '\0';
          String line = pBuf;
          if (line.startsWith("TEXT ")) {
            nextline = true;

            serialTake();
            Serial.print(F("TEXT Line: "));
            serialGive();
            int rubOffset = line.indexOf(" RUB.");
            if (rubOffset > 0) {
              int summOffset = 0;
              for (int i = rubOffset - 1; i > 1; i--) {
                char c = line[i];
                //Serial.print(c);
                if (c == ' ' || c == ':') {
                  summOffset = i;
                  serialTake();
                  Serial.print(line.substring(i, rubOffset));
                  serialGive();
                  break;
                }
              }
              serialTake();
              Serial.print(F( " P. "));
              serialGive();

              if (summOffset > 0) {
                for (int i = 1; i < 20; i++) {
                  char c = line[summOffset + i];
                  if (c >= '0' && c <= '9') {
                    summ *= 10;
                    summ += c - '0';
                  } else {
                    break;
                  }
                }
              }
              int qrOffset = line.indexOf("QR:");
              if (qrOffset > 0) {
                int qrEnd = line.indexOf('.', qrOffset + 1);
                if ((qrEnd > 0) && (qrEnd - (qrOffset + 4) > 9)) {
                  qr = line.substring(qrOffset + 4, qrEnd);
                  serialTake();
                  Serial.print(F("QR: "));
                  Serial.println(qr);
                  serialGive();
                  nextline = false;
                  found = true;
                }
              }
            }
          } else if (nextline) {
            int qrEnd = line.indexOf('.');
            if (qrEnd > 9) {
              qr = line.substring(0, qrEnd);
              serialTake();
              Serial.print(F("QR: "));
              Serial.println(qr);
              serialGive();
              nextline = false;
              found = true;
            }
          }

          if (found) {
            nextline = false;
            if (summ > 0) {
              serialTake();
              Serial.print(F("Summ: "));
              Serial.print(summ);
              Serial.print(F(" QR: "));
              Serial.println(qr);
              serialGive();
              if ((summ > 0) && (!qr_filter.compareTo(qr))) {
                startWork((((60 * summ * 8)/10)));
#ifdef USE_EEPROM                
                writeWorkTime();
#endif                                                
                serialTake();
                Serial.println(pay_detect);
                serialGive();
                digitalWrite(ledRed, HIGH);                
                botSendMessage(pay_detect);
              } else {
                serialTake();
                Serial.println(F("Wrong Summ or QR"));
                serialGive();
              }
            }
            break;
          }
          nBytes = 0;
        } else
          pBuf[nBytes++] = c;
      }
    }
  }
}

//Callback function to get the Email reading status
void searchCallback(ReadStatus msg)
{
  if (msg.success() && (imapData->availableMessages()))
  {
    messageUID = imapData->getUID(0);
    serialTake();
    Serial.println("=================");
    //Search result number which varied upon search crieria
    Serial.println("Messsage Number: " + imapData->getNumber(0));    
    //UID only available when assigned UID keyword in setSearchCriteria
    //e.g. imapData->setSearchCriteria("UID SEARCH ALL");
    Serial.println("Messsage UID: " + messageUID);
    Serial.println("From: " + imapData->getFrom(0));
    Serial.println("To: " + imapData->getTo(0));
    Serial.println("Date: " + imapData->getDate(0));
    Serial.println("Subject: " + imapData->getSubject(0));
    serialGive();
  }
}

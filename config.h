#define CTBOT_DEBUG_MODE               0
//#define CTBOT_USE_FINGERPRINT          0

#define WIFI_SSID "SSID"
#define WIFI_PASSWORD "PASSWORD"

#define TLG_BOT_TOKEN "token"  //токен телеграм бота
#define TLG_CHAT_ID 191835312
//Кол-во попыток отправить телеграм сообщение если не удалось
#define TLG_SEND_RET 10

//Сервер с письмами
#define IMAP_SERVER "imap.gmail.com"
#define IMAP_PORT 993
#define POP_SERVER "pop.gmail.com"
#define POP_PORT 995
#define MAIL_USER "test@gmail.com"
#define MAIL_PASS "TestPassword"

#define QR_NUMBER "1000112466"
//Интервал проверки почты в сек.
#define MAIL_REQUEST_INTERVAL 10
//Время включения реле в мин
#define TURN_ON_TIME 10
//Минимальная сумма платежа
#define PAY_MIN_AMOUNT 1

#define USE_EEPROM
//#define DEBUG_STACK_SIZE

#include <Arduino.h>
void * my_malloc(size_t size);

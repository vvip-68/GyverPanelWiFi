// Гайд по постройке матрицы: https://alexgyver.ru/matrix_guide/
// Страница проекта на GitHub: https://github.com/vvip-68/GyverPanelWiFi
// Автор идеи, начальный проект - GyverMatrixBT: AlexGyver Technologies, 2019 (https://alexgyver.ru/gyvermatrixbt/)
// Дальнейшее развитие: vvip-68, 2019-2022
// https://AlexGyver.ru/
//
// Дополнительные ссылки для Менеджера плат ESP8266 и ESP32 в Файл -> Настройки
// https://raw.githubusercontent.com/esp8266/esp8266.github.io/master/stable/package_esp8266com_index.json
// https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json

#define FIRMWARE_VER F("WiFiPanel v.1.13.2022.1212")

// -------------------------------------------------------------------------------------------------------
// Версии (что нового): 
// -------------------------------------------------------------------------------------------------------

// v1.13 - добавлен вариант использования сборных матриц, состоящих из элементарных матриц разного (смешанного) типа,
// с различаюимися размерами, углом подключения и направления из угла - параметр MATRIX_TYPE == 2
// Карта индексов составляется в утилите LedMapper? включенной в состав проекта

// -------------------------------------------------------------------------------------------------------

// v1.12 изменилась карта распределения хранения настроек в постоянной памяти EEPROM
// После сборки и загрузки скомпилированной прошивки 1.12 вам придется заново перенастраивать все эффекты
// и прочие настройки программы, в том числе набор текстов бегущей строки. Ввиду несовместимости расположения 
// сохраненных настроек, их восстановление из файла резервной копии также недоступно или приведет к
// сбоям в работе прошивки. Первая загрузка прошивки 1.12 поверх более ранней опции должна быть выполнена
// с настройкой в меню "Инструменты" - "Erase Flash: Erase All"

// -------------------------------------------------------------------------------------------------------

// v1.11 изменилась максимально допустимая длина пароля и имя сети WiFi, а так же место 
// их хранения в в контроллере. Теперь максимальная длина ssid - 32 символа, максимальная длина пароля - 
// 64 символа. Для того, чтобы пароль сохранялся - при сборке прошивки нужно выбирать вариант распределения 
// памяти, отличный от 'FS:none' в меню "Инструменты" -> "Flash size", например "FS:2MB OTA:~1019KB"

// -------------------------------------------------------------------------------------------------------

// Внимание !!!
//
// Тип микроконтроллера в меню "Инструменты -> Плата" для ESP8266 выбирать "NodeMCU 1.0 (ESP12E Module)" даже
// с случае использования микроконтроллера семейства Wemos d1 mini. При выборе другого типа микроконтроллера
// скорее всего пин вывода на ленту переназначится компилятором на другой пин, отличный от D2, вероятнее всего - на D4.
// Пин D4 используется данной прошивкой (в стандартном варианте) для подключения кнопки. Такое совпадение пинов
// приведет к невозможности регулировки яркости кнопкой, а сама яркость будет автоматически плавно за несколько секунд
// уменьшаться до нуля, и вместо изображения эффектов вы увидите либо черную матрицу, либо тусклые "ночные" часы.
//
// Внимание!!!
// Рабочие комбинации версий ядра и библиотек:
//
// Версия ядра ESP8266 - 2.7.4    
// Версия ядра ESP32   - 1.0.6
// Версия FastLED      - 3.5.0 (или 3.4.0)
//
// Вне зависимости от реальной платы выбирать NodeMCU v1.0 (ESP12-E)
// Пин вывода тя ленту LED_PIN обозначен как 2 (GPIO2) или D4
// Тем не менее при указанной выше комбинации Ядро/библиотека назначает вывод на пин D2 микроконтроллера
// Если используется плеер MP3 DFPlayer - будет ли он работать - чистая лотерея, зависит от чипа и фаз луны
// В некоторых случаях может быть циклическая перезагрузка после подключения к сети,
// в некоторых случаях - перезагрузка микроконтроллера при порытке проиграть мелодию будильника из приложения
// Но, может и работать нормально.
//
// Вероятнее всего нестабильность работы плеера зависит от версии библиотеки SoftwareSerial, входящей в состав ядра.
// Другая вероятная причина возможных сбоев плеера - (не)стабильность или недостаточное (или завышенное) напряжение питание платы плеера.
//
// С остальными комбинациями версии ядра ESP8266 и выбранных плат скорее всего стабильно работать не будет:
//  -> Постоянно горит самый первый светодиод в цепочке синим или зелёным
//  -> некоторые эффекты сильно мерцают или замирают.
//  -> пин ленты с D2 переназначается ядром на D4
//
// На версии ядра ESP32 1.0.5 или 2.x.x и FastLED 3.4/3.5 работать не будет! Используйте версию ядра 1.0.6.
//  -> 1.0.5 - При USE_E131 == 1 - при переключении с MASTER на STANDALONE или SLAVE - crash  и перезагрузка. 
//             Для исправления - в файле C:\Users\<user>\AppData\Local\Arduino15\packages\esp32\hardware\esp32\1.0.5\libraries\AsyncUDP\src\AsyncUDP.cpp
//             найти деструктор AsyncUDP::~AsyncUDP() и перенести вызов close(); из начала процедуры в конец  
//  -> 2.0.0 - Нет вывода изображения на ленту ни на одном из пинов

// -------------------------------------------------------------------------------------------------------

// Библиотеку TM1637 следует обязательно устанавливать из папки проекта. В ней сделаны исправления, позволяющие
// компилироваться проекту для микроконтроллера ESP32. Со стандартной версией библиотеки из менеджера библиотек
// проект не будет компилироваться

// Библиотеку ESPAsyncE131 следует обязательно устанавливать из папки проекта. В ней исправлены ошибки стандартной
// библиотеки (добавлен деструктор и освобождение выделяемых ресурсов), а также добавлен ряд функций, позволяющих
// осуществлять передачу сформированных пакетов в сеть. Со стандартной версией библиотеки из менеджера библиотек
// проект не будет компилироваться

// -------------------------------------------------------------------------------------------------------

// ************************ WIFI ПАНЕЛЬ *************************

#include "a_def_hard.h"     // Определение параметров матрицы, пинов подключения и т.п
#include "a_def_soft.h"     // Определение параметров эффектов, переменных программы и т.п.

#if (USE_MQTT == 1)

// ------------------ MQTT CALLBACK -------------------

void callback(char* topic, uint8_t* payload, uint32_t length) {
  if (stopMQTT) return;
  // проверяем из нужного ли нам топика пришли данные
  DEBUG("MQTT << topic='" + String(topic) + "'");
  if (strcmp(topic, mqtt_topic(TOPIC_CMD).c_str()) == 0) {
    memset(incomeMqttBuffer, 0, BUF_MQTT_SIZE);
    memcpy(incomeMqttBuffer, payload, length);
    
    DEBUG(F("; cmd='"));
    DEBUG(incomeMqttBuffer);
    DEBUG("'");
    
    // В одном сообщении может быть несколько команд. Каждая команда начинается с '$' и заканчивается ';'/ Пробелы между ';' и '$' НЕ допускаются.
    String command = String(incomeMqttBuffer);    
    command.replace("\n", "~");
    command.replace(";$", "\n");
    uint32_t count = CountTokens(command, '\n');
    
    for (uint8_t i=1; i<=count; i++) {
      String cmd = GetToken(command, i, '\n');
      cmd.replace('~', '\n');
      cmd.trim();
      // После разделения команд во 2 и далее строке '$' (начало команды) удален - восстановить
      if (!cmd.startsWith("$")) {
        cmd = "$" + cmd;
      }
      // После разделения команд во 2 и далее строке ';' (конец команды) удален - восстановить
      // Команда '$6 ' не может быть в пакете и признак ';' (конец команды) не используется - не восстанавливать
      if (!cmd.endsWith(";") && !cmd.startsWith(F("$6 "))) {
        cmd += ";";
      }        
      if (cmd.length() > 0 && queueLength < QSIZE_IN) {
        queueLength++;
        cmdQueue[queueWriteIdx++] = cmd;
        if (queueWriteIdx >= QSIZE_IN) queueWriteIdx = 0;
      }
    }    
  }
  DEBUGLN();
}

#endif

// ---------------------------------------------------------------

void setup() {
  #if defined(ESP8266)
    ESP.wdtEnable(WDTO_8S);
  #endif

  // Инициализация EEPROM и загрузка начальных значений переменных и параметров
  #if (EEPROM_MAX <= EEPROM_TEXT)
    #pragma message "Не выделено памяти для хранения строк эффекта 'Бегущая строка'"
    EEPROM.begin(EEPROM_TEXT);
  #else  
    EEPROM.begin(EEPROM_MAX);
  #endif

  #if (DEBUG_SERIAL == 1)
    Serial.begin(115200);
    delay(300);
  #endif

  // пинаем генератор случайных чисел
  #if defined(ESP8266) && defined(TRUE_RANDOM)
  uint32_t seed = (int)RANDOM_REG32;
  #else
  uint32_t seed = (int)(analogRead(0) ^ micros());
  #endif
  randomSeed(seed);
  random16_set_seed(seed);

  #ifdef DEVICE_ID
    host_name = String(HOST_NAME) + "-" + String(DEVICE_ID);
  #else
    host_name = String(HOST_NAME);
  #endif

  uint8_t eeprom_id = EEPROMread(0);
  
  DEBUGLN();
  DEBUGLN(FIRMWARE_VER);
  DEBUG(F("Версия EEPROM: "));
  DEBUGLN("0x" + IntToHex(eeprom_id, 2));  
  if (eeprom_id != EEPROM_OK) {
    DEBUG(F("Обновлено до: "));
    DEBUGLN("0x" + IntToHex(EEPROM_OK, 2));  
  }
  DEBUGLN("Host: '" + host_name + "'");
  DEBUGLN();
  
  DEBUGLN(F("Инициализация файловой системы... "));
  
  spiffs_ok = LittleFS.begin();
  if (!spiffs_ok) {
    DEBUGLN(F("Выполняется разметка файловой системы... "));
    LittleFS.format();
    spiffs_ok = LittleFS.begin();    
  }

  if (spiffs_ok) {
    #if defined(ESP32)
      spiffs_total_bytes = LittleFS.totalBytes();
      spiffs_used_bytes  = LittleFS.usedBytes();
      DEBUGLN(String(F("Использовано ")) + String(spiffs_used_bytes) + " из " + String(spiffs_total_bytes) + " байт");
      DEBUGLN();
    #else
      FSInfo fs_info;
      if (LittleFS.info(fs_info)) {
        spiffs_total_bytes = fs_info.totalBytes;
        spiffs_used_bytes  = fs_info.usedBytes;
        DEBUGLN(String(F("Использовано ")) + String(spiffs_used_bytes) + " из " + String(spiffs_total_bytes) + " байт");
        DEBUGLN();
      } else {
        DEBUGLN(F("Ошибка получения сведений о файловой системе."));
        DEBUGLN();
    }
    #endif
  } else {
    DEBUGLN(F("Файловая система недоступна."));
    DEBUGLN();
  }

  loadSettings();

  // -----------------------------------------  
  // Вывод основных возможностей: поддержка в прошивке - 
  // включена или выключена + некоторые параметры
  // -----------------------------------------  

  DEBUGLN();
  DEBUG(F("Матрица: "));
  if (DEVICE_TYPE == 0) DEBUG(F("труба "));
  if (DEVICE_TYPE == 1) DEBUG(F("плоская "));
  DEBUGLN(String(pWIDTH) + "x" + String(pHEIGHT));

  if (sMATRIX_TYPE == 2) {
    DEBUGLN(F("Aдресация: карта индексов"));
  } else {
    DEBUGLN(F("Адресация: по подключению"));
    DEBUG(F("  Угол: ")); 
    if (sCONNECTION_ANGLE == 0) DEBUGLN(F("левый нижний")); else
    if (sCONNECTION_ANGLE == 1) DEBUGLN(F("левый верхний")); else
    if (sCONNECTION_ANGLE == 2) DEBUGLN(F("правый верхний")); else
    if (sCONNECTION_ANGLE == 3) DEBUGLN(F("правый нижний"));
    DEBUG(F("  Направление: "));
    if (sSTRIP_DIRECTION == 0) DEBUGLN(F("вправо")); else
    if (sSTRIP_DIRECTION == 1) DEBUGLN(F("вверх")); else
    if (sSTRIP_DIRECTION == 2) DEBUGLN(F("влево")); else
    if (sSTRIP_DIRECTION == 3) DEBUGLN(F("вниз"));
    DEBUG(F("  Тип: "));
    if (sMATRIX_TYPE == 0) DEBUGLN(F("зигзаг")); else
    if (sMATRIX_TYPE == 1) DEBUGLN(F("параллельная"));
    
    if (mWIDTH > 1 || mHEIGHT > 1) {
      DEBUG(F("Cегменты: "));
      DEBUGLN(String(mWIDTH) + "x" + String(mHEIGHT));
      DEBUG(F("  Угол: ")); 
      if (mANGLE == 0) DEBUGLN(F("левый нижний")); else
      if (mANGLE == 1) DEBUGLN(F("левый верхний")); else
      if (mANGLE == 2) DEBUGLN(F("правый верхний")); else
      if (mANGLE == 3) DEBUGLN(F("правый нижний"));
      DEBUG(F("  Направление: "));
      if (mDIRECTION == 0) DEBUGLN(F("вправо")); else
      if (mDIRECTION == 1) DEBUGLN(F("вверх")); else
      if (mDIRECTION == 2) DEBUGLN(F("влево")); else
      if (mDIRECTION == 3) DEBUGLN(F("вниз"));
      DEBUG(F("  Тип: "));
      if (mTYPE == 0) DEBUGLN(F("зигзаг")); else
      if (mTYPE == 1) DEBUGLN(F("параллельная"));
    }
  }

  DEBUGLN();
  DEBUGLN(F("Доступные возможности:"));
  
  DEBUG(F("+ Бегущая строка: шрифт "));
  if (BIG_FONT == 0)
    DEBUGLN(F("5x8"));
  if (BIG_FONT == 2)
    DEBUGLN(F("8x13"));
  if (BIG_FONT == 1)
    DEBUGLN(F("10x16"));

  DEBUG(F("+ Кнопка управления: "));
  if (BUTTON_TYPE == 0)
    DEBUGLN(F("сенсорная"));
  if (BUTTON_TYPE == 1)
    DEBUGLN(F("тактовая"));

  DEBUGLN(F("+ Синхронизация времени с сервером NTP"));

  DEBUG((USE_POWER == 1 ? "+" : "-"));
  DEBUGLN(F(" Управление питанием"));

  DEBUG((USE_WEATHER == 1 ? "+" : "-"));
  DEBUGLN(F(" Получение информации о погоде"));

  DEBUG((USE_MQTT == 1 ? "+" : "-"));
  DEBUGLN(F(" Управление по каналу MQTT"));

  DEBUG((USE_E131 == 1 ? "+" : "-"));
  DEBUGLN(F(" Групповая синхронизация по протоколу E1.31"));

  DEBUG((USE_TM1637 == 1 ? "+" : "-"));
  DEBUGLN(F(" Дополнительный индикатор TM1637"));

  DEBUG((USE_SD == 1 ? "+" : "-"));
  DEBUGLN(F(" Эффекты Jinx! с SD-карты"));

  DEBUG((USE_MP3 == 1 ? "+" : "-"));
  DEBUGLN(F(" Поддержка MP3 Player"));

  DEBUGLN();

  if (sMATRIX_TYPE == 2) {
    pWIDTH = mapWIDTH;
    pHEIGHT = mapHEIGHT;
  } else {
    pWIDTH = sWIDTH * mWIDTH;
    pHEIGHT = sHEIGHT * mHEIGHT;
  }

  NUM_LEDS = pWIDTH * pHEIGHT;
  maxDim   = max(pWIDTH, pHEIGHT);
  minDim   = min(pWIDTH, pHEIGHT);

  // -----------------------------------------
  // В этом блоке можно принудительно устанавливать параметры, которые должны быть установлены при старте микроконтроллера
  // -----------------------------------------
  
  // -----------------------------------------  
    
  // Настройки ленты
  leds =  new CRGB[NUM_LEDS];          
  overlayLEDs = new CRGB[OVERLAY_SIZE];

  // Создать массив для карты индексов адресации светодиодов в ленте
  bool ok = loadIndexMap();
  if (!ok || mapListLen == 0) {
    sMATRIX_TYPE = 0;
    putMatrixSegmentType(sMATRIX_TYPE);
  }

  FastLED.addLeds<WS2812, LED_PIN, COLOR_ORDER>(leds, NUM_LEDS).setCorrection( TypicalLEDStrip );

  /*
  // Это пример, как выводить на две матрицы 16х16 (сборная матрица 32х16) через два пина D2 и D3
  // Чтобы вывод был именно на D2 и D3 - в меню Инструменты - плату выбирать "Wemos D1 mini pro" - при выбранной плате NodeMCU назначение пинов куда-то "съезжает" на другие - нужно искать куда. 
  // Убедитесь в правильном назначении адресации диодов матриц в сборной матрице используя индексные файлы или сьорную матрицу из матриц одного размера и подключения сегментов.
  FastLED.addLeds<WS2812, D2, COLOR_ORDER>(leds, 256).setCorrection( TypicalLEDStrip );
  FastLED.addLeds<WS2812, D3, COLOR_ORDER>(leds, 256, 256).setCorrection( TypicalLEDStrip );
  */
  
  FastLED.setBrightness(globalBrightness);
  if (CURRENT_LIMIT > 0) {
    FastLED.setMaxPowerInVoltsAndMilliamps(5, CURRENT_LIMIT);
  }
  FastLED.clear();
  FastLED.show();

  // Инициализация SD-карты
  #if (USE_SD == 1)
    InitializeSD1();
  #endif

  // Проверить наличие резервной копии настроек EEPROM в файловой системе MK и/или на SD-карте
  eeprom_backup = checkEepromBackup();
  if ((eeprom_backup & 0x01) > 0) {
    DEBUGLN(F("Найдены сохраненные настройки: FS://eeprom.bin"));
  }
  if ((eeprom_backup & 0x02) > 0) {
    DEBUGLN(F("Найдены сохраненные настройки: SD://eeprom.bin"));
  }
    
  // Инициализация SD-карты
  #if (USE_SD == 1)
    InitializeSD2();
  #endif

  // Поиск доступных анимаций
  initAnimations();

  // Поиск картинок, пригодных для эффекта "Слайды"
  initialisePictures();
  
  #if (USE_POWER == 1)
    pinMode(POWER_PIN, OUTPUT);
  #endif
     
  // Первый этап инициализации плеера - подключение и основные настройки
  #if (USE_MP3 == 1)
    InitializeDfPlayer1();
  #endif

  #if defined(ESP8266)
    WiFi.setSleepMode(WIFI_NONE_SLEEP);
  #endif

  // Настройка кнопки
  butt.setStepTimeout(100);
  butt.setClickTimeout(500);

  // Второй этап инициализации плеера - проверка наличия файлов звуков на SD карте
  #if (USE_MP3 == 1)
    if (isDfPlayerOk) {
      InitializeDfPlayer2();
      if (!isDfPlayerOk) {
        DEBUGLN(F("MP3 плеер недоступен."));
      }
    } else
        DEBUGLN(F("MP3 плеер недоступен."));
  #endif

  // Подключение к сети
  connectToNetwork();

  #if (USE_E131 == 1)
    InitializeE131();
  #endif
  
  #if (USE_MQTT == 1)
  // Настройка соединения с MQTT сервером
  stopMQTT = !useMQTT;
  changed_keys = "";
  mqtt_client_name = host_name + "-" + String(random16(), HEX);
  last_mqtt_server = mqtt_server;
  last_mqtt_port = mqtt_port;
  mqtt.setServer(mqtt_server, mqtt_port);
  mqtt.setCallback(callback);
  mqtt.setSocketTimeout(1);
  uint32_t t = millis();
  checkMqttConnection();
  if (millis() - t > MQTT_CONNECT_TIMEOUT) {
    nextMqttConnectTime = millis() + MQTT_RECONNECT_PERIOD;
  }
  String msg = F("START");
  SendMQTT(msg, TOPIC_STA);
  #endif

  // Port defaults to 8266
  // ArduinoOTA.setPort(8266);
 
  // Hostname defaults to esp8266-[ChipID]
  ArduinoOTA.setHostname(host_name.c_str());
 
  // No authentication by default
  // ArduinoOTA.setPassword((const char *)"123");
 
  ArduinoOTA.onStart([]() {
    String type;
    if (ArduinoOTA.getCommand() == U_FLASH)
      type = F("скетча...");
    else // U_SPIFFS
      type = F("файловой системы SPIFFS...");
    // NOTE: if updating SPIFFS this would be the place to unmount SPIFFS using SPIFFS.end()
    DEBUG(F("Начато обновление "));    
    DEBUGLN(type);    
  });

  ArduinoOTA.onEnd([]() {
    DEBUGLN(F("\nОбновление завершено"));
  });

  ArduinoOTA.onProgress([](uint32_t progress, uint32_t total) {
    #if (DEBUG_SERIAL == 1)
    Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
    #endif
  });

  ArduinoOTA.onError([](ota_error_t error) {
    DEBUG(F("Ошибка: "));
    DEBUGLN(error);
    if      (error == OTA_AUTH_ERROR)    DEBUGLN(F("Неверное имя/пароль сети"));
    else if (error == OTA_BEGIN_ERROR)   DEBUGLN(F("Не удалось запустить обновление"));
    else if (error == OTA_CONNECT_ERROR) DEBUGLN(F("Не удалось установить соединение"));
    else if (error == OTA_RECEIVE_ERROR) DEBUGLN(F("Не удалось получить данные"));
    else if (error == OTA_END_ERROR)     DEBUGLN(F("Ошибка завершения сессии"));
  });

  ArduinoOTA.begin();

  // UDP-клиент на указанном порту
  udp.begin(localPort);

  // Настройка внешнего дисплея TM1637
  #if (USE_TM1637 == 1)
  display.setBrightness(7);
  display.displayByte(_empty, _empty, _empty, _empty);
  #endif

  // Таймер синхронизации часов
  ntpSyncTimer.setInterval(1000 * 60 * SYNC_TIME_PERIOD);

  #if (USE_WEATHER == 1)     
  // Таймер получения погоды
  weatherTimer.setInterval(1000 * 60 * SYNC_WEATHER_PERIOD);
  #endif

  // Таймер рассвета
  dawnTimer.stopTimer();
  
  // Проверить соответствие позиции вывода часов размерам матрицы
  // При необходимости параметры отображения часов корректируются в соответствии с текущими аппаратными возможностями
  checkClockOrigin();
  
  // Если был задан спец.режим во время предыдущего сеанса работы матрицы - включить его
  // Номер спец-режима запоминается при его включении и сбрасывается при включении обычного режима или игры
  // Это позволяет в случае внезапной перезагрузки матрицы (например по wdt), когда был включен спец-режим (например ночные часы или выкл. лампы)
  // снова включить его, а не отображать случайный обычный после включения матрицы
  int8_t spc_mode = getCurrentSpecMode();

  if (spc_mode >= 0 && spc_mode < MAX_SPEC_EFFECT) {
    setSpecialMode(spc_mode);
    set_isTurnedOff(spc_mode == 0);
    set_isNightClock(spc_mode == 8);
  } else {
    set_thisMode(getCurrentManualMode());
    if (thisMode < 0 || thisMode == MC_TEXT || thisMode >= SPECIAL_EFFECTS_START) {
      setRandomMode();
    } else {
      setEffect(thisMode);        
    }
  }
  autoplayTimer = millis();

  #if (USE_MQTT == 1)
  if (!stopMQTT) mqttSendStartState();
  #endif

  setIdleTimer();
}

void loop() {
  if (wifi_connected) {
    ArduinoOTA.handle();
    #if (USE_MQTT == 1)
      if (!stopMQTT) {
        checkMqttConnection();
        mqtt.loop();
      }
    #endif
  }
  
  process();
}

// -----------------------------------------

void startWiFi(uint32_t waitTime) { 
  
  WiFi.disconnect(true);
  set_wifi_connected(false);
  
  delay(10);               // Иначе получаем Core 1 panic'ed (Cache disabled but cached memory region accessed)
  WiFi.mode(WIFI_STA);
 
  // Пытаемся соединиться с роутером в сети
  if (ssid.length() > 0) {
    DEBUG(F("\nПодключение к "));
    DEBUG(ssid);

    if (IP_STA[0] + IP_STA[1] + IP_STA[2] + IP_STA[3] > 0) {
      WiFi.config(IPAddress(IP_STA[0], IP_STA[1], IP_STA[2], IP_STA[3]),  // 192.168.0.106
                  IPAddress(IP_STA[0], IP_STA[1], IP_STA[2], GTW),        // 192.168.0.1
                  IPAddress(255, 255, 255, 0),                            // Mask
                  IPAddress(IP_STA[0], IP_STA[1], IP_STA[2], GTW),        // DNS1 192.168.0.1
                  IPAddress(8, 8, 8, 8));                                 // DNS2 8.8.8.8                  
      DEBUG(F(" -> "));
      DEBUG(IP_STA[0]);
      DEBUG(".");
      DEBUG(IP_STA[1]);
      DEBUG(".");
      DEBUG(IP_STA[2]);
      DEBUG(".");
      DEBUG(IP_STA[3]);                  
    }              
    WiFi.begin(ssid.c_str(), pass.c_str());
  
    // Проверка соединения (таймаут 180 секунд, прерывается при необходимости нажатием кнопки)
    // Такой таймаут нужен в случае, когда отключают электричество, при последующем включении устройство стартует быстрее
    // чем роутер успеет загрузиться и создать сеть. При коротком таймауте устройство не найдет сеть и создаст точку доступа,
    // не сможет получить время, погоду и т.д.
    bool     stop_waiting = false;
    uint32_t start_wifi_check = millis();
    uint32_t last_wifi_check = 0;
    int16_t  cnt = 0;
    while (!(stop_waiting || wifi_connected)) {
      delay(0);
      if (millis() - last_wifi_check > 250) {
        last_wifi_check = millis();
        set_wifi_connected(WiFi.status() == WL_CONNECTED); 
        if (wifi_connected) {
          // Подключение установлено
          DEBUGLN();
          DEBUG(F("WiFi подключен. IP адрес: "));
          DEBUGLN(WiFi.localIP());
          break;
        }
        if (cnt % 50 == 0) {
          DEBUGLN();
        }
        DEBUG(".");
        cnt++;
      }
      if (millis() - start_wifi_check > waitTime) {
        // Время ожидания подключения к сети вышло
        break;
      }
      delay(0);
      // Опрос состояния кнопки
      butt.tick();
      if (butt.hasClicks()) {
        butt.getClicks();
        DEBUGLN();
        DEBUGLN(F("Нажата кнопка.\nОжидание подключения к сети WiFi прервано."));  
        stop_waiting = true;
        break;
      }
      delay(0);
    }
    DEBUGLN();

    if (!wifi_connected && !stop_waiting)
      DEBUGLN(F("Не удалось подключиться к сети WiFi."));
  }  
}

void startSoftAP() {
  WiFi.softAPdisconnect(true);
  ap_connected = false;

  DEBUG(F("Создание точки доступа "));
  DEBUG(apName);
  
  ap_connected = WiFi.softAP(apName, apPass);

  for (uint8_t j = 0; j < 10; j++ ) {    
    delay(0);
    if (ap_connected) {
      DEBUGLN();
      DEBUG(F("Точка доступа создана. Сеть: '"));
      DEBUG(apName);
      // Если пароль совпадает с паролем по умолчанию - печатать для информации,
      // если был изменен пользователем - не печатать
      if (strcmp(apPass, "12341234") == 0) {
        DEBUG(F("'. Пароль: '"));
        DEBUG(apPass);
      }
      DEBUGLN(F("'."));
      DEBUG(F("IP адрес: "));
      DEBUGLN(WiFi.softAPIP());
      break;
    }    
    
    WiFi.enableAP(false);
    WiFi.softAPdisconnect(true);
    delay(500);
    
    DEBUG(".");
    ap_connected = WiFi.softAP(apName, apPass);
  }  
  DEBUGLN();  

  if (!ap_connected) 
    DEBUGLN(F("Не удалось создать WiFi точку доступа."));
}

void connectToNetwork() {
  // Подключиться к WiFi сети, ожидать подключения 180 сек пока, например, после отключения электричества роутер загрузится и поднимет сеть
  startWiFi(180000);

  // Если режим точки доступа не используется и к WiFi сети подключиться не удалось - создать точку доступа
  if (!wifi_connected){
    WiFi.mode(WIFI_AP);
    startSoftAP();
  }

  if (useSoftAP && !ap_connected) startSoftAP();    

  // Сообщить UDP порт, на который ожидаются подключения
  if (wifi_connected || ap_connected) {
    DEBUG(F("UDP-сервер на порту "));
    DEBUGLN(localPort);
  }
}

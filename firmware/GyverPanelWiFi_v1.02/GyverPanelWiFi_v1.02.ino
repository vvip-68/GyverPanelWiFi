// Скетч к проекту "Широкоформатная WiFi панель / гирлянда"
// Гайд по постройке матрицы: https://alexgyver.ru/matrix_guide/
// Страница проекта на GitHub: https://github.com/vvip-68/GyverPanelWiFi
// Автор: AlexGyver Technologies, 2019
// Дальнейшее развитие: vvip, 2019,2020
// https://AlexGyver.ru/
//
// Дополнительные ссылки для Менеджера плат ESP8266 и ESP32 в Файл -> Настройки
// http://arduino.esp8266.com/stable/pspackage_esp8266com_index.json
// https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json


// ************************ WIFI ПАНЕЛЬ *************************

#define FIRMWARE_VER F("LED-Panel-WiFi v.1.01.2020.0901")

#include "a_def_hard.h"     // Определение параметров матрицы, пинов подключения и т.п
#include "a_def_soft.h"     // Определение параметров эффектов, переменных программы и т.п.

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

  Serial.begin(115200);
  delay(300);

  Serial.println();
  Serial.println();
  Serial.println(FIRMWARE_VER);
  Serial.println();

  loadSettings();

  // Настройки ленты
  FastLED.addLeds<WS2812, LED_PIN, COLOR_ORDER>(leds, NUM_LEDS).setCorrection( TypicalLEDStrip );
  FastLED.setBrightness(globalBrightness);
  if (CURRENT_LIMIT > 0) {
    FastLED.setMaxPowerInVoltsAndMilliamps(5, CURRENT_LIMIT);
  }
  FastLED.clear();
  FastLED.show();

#if defined(ESP8266) && defined(TRUE_RANDOM)
  unsigned long seed = (int)RANDOM_REG32;
#else
  unsigned long seed = (int)(analogRead(0) ^ micros());
#endif
  randomSeed(seed);

 // randomSeed(analogRead(0) ^ millis());    // пинаем генератор случайных чисел

  // Первый этап инициализации плеера - подключение и основные настройки
  #if (USE_MP3 == 1)
    InitializeDfPlayer1();
  #endif
     
  #if defined(ESP8266)
    WiFi.setSleepMode(WIFI_NONE_SLEEP);
  #endif

  // Подключение к сети
  connectToNetwork();

  // Port defaults to 8266
  // ArduinoOTA.setPort(8266);
 
  // Hostname defaults to esp8266-[ChipID]
  ArduinoOTA.setHostname("Panel-WiFi");
 
  // No authentication by default
  // ArduinoOTA.setPassword((const char *)"123");
 
  ArduinoOTA.onStart([]() {
    String type;
    if (ArduinoOTA.getCommand() == U_FLASH)
      type = F("скетча...");
    else // U_SPIFFS
      type = F("файловой системы SPIFFS...");
    // NOTE: if updating SPIFFS this would be the place to unmount SPIFFS using SPIFFS.end()
    Serial.print(F("Начато обносление "));    
    Serial.println(type);    
  });
  ArduinoOTA.onEnd([]() {
    Serial.println(F("\nОбновление завершено"));
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
  });
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.print(F("Ошибка: "));
    Serial.println(error);
    if      (error == OTA_AUTH_ERROR)    Serial.println(F("Неверное имя/пароль сети"));
    else if (error == OTA_BEGIN_ERROR)   Serial.println(F("Не удалось запустить обновление"));
    else if (error == OTA_CONNECT_ERROR) Serial.println(F("Не удалось установить соединение"));
    else if (error == OTA_RECEIVE_ERROR) Serial.println(F("Не удалось получить данные"));
    else if (error == OTA_END_ERROR)     Serial.println(F("Ошибка завершения сессии"));
  });
  ArduinoOTA.begin();

  // UDP-клиент на указанном порту
  udp.begin(localPort);

  // Настройка кнопки
  butt.setStepTimeout(100);
  butt.setClickTimeout(500);

  // Настройка внешнего дисплея TM1637
  #if (USE_TM1637 == 1)
  display.setBrightness(7);
  display.displayByte(_empty, _empty, _empty, _empty);
  #endif
    
  // Таймер бездействия
  if (idleTime == 0) // Таймер Idle  отключен
    idleTimer.setInterval(4294967295);
  else  
    idleTimer.setInterval(idleTime);

  // Таймер синхронизации часов
  ntpSyncTimer.setInterval(1000 * 60 * SYNC_TIME_PERIOD);

  #if (USE_WEATHER == 1)     
  // Таймер получения погоды
  weatherTimer.setInterval(1000 * 60 * SYNC_WEATHER_PERIOD);
  #endif

  // Таймер рассвета
  dawnTimer.setInterval(4294967295);
  
  // Второй этап инициализации плеера - проверка наличия файлов звуков на SD карте
  #if (USE_MP3 == 1)
    InitializeDfPlayer2();
    if (!isDfPlayerOk) {
      Serial.println(F("MP3 плеер недоступен."));
    }
  #endif

  // Проверить соответствие позиции вывода часов размерам матрицы
  // При необходимости параметры отображения часов корректируются в соответствии с текущими аппаратными возможностями
  checkClockOrigin();
  
  // Если был задан спец.режим во время предыдущего сеанса работы матрицы - включить его
  // Номер спец-режима запоминается при его включении и сбрасывается при включении обычного режима или игры
  // Это позволяет в случае внезапной перезагрузки матрицы (например по wdt), когда был включен спец-режим (например ночные часы или выкл. лампы)
  // снова включить его, а не отображать случайный обычный после включения матрицы
  int8_t spc_mode = getCurrentSpecMode();
  int8_t mnl_mode = getCurrentManualMode();

  if (spc_mode >= 0 && spc_mode < MAX_SPEC_EFFECT) {
    setSpecialMode(spc_mode);
    isTurnedOff = spc_mode == 0;
    isNightClock = spc_mode == 8;
  } else {
    thisMode = getCurrentManualMode();
    if (thisMode < 0 || thisMode == MC_TEXT) {
      setRandomMode2();
    } else {
      setEffect(thisMode);        
    }
  }
  autoplayTimer = millis();

  if (manualMode || specialMode) {
    idleTimer.setInterval(4294967295);
  } else {
    idleTimer.setInterval(idleTime);    
  }
  idleTimer.reset();
}

void loop() {
  ArduinoOTA.handle();
  process();
}

// -----------------------------------------

void startWiFi() { 
  
  WiFi.disconnect(true);
  wifi_connected = false;
  
  delay(10);               // Иначе получаем Core 1 panic'ed (Cache disabled but cached memory region accessed)
  WiFi.mode(WIFI_STA);
 
  // Пытаемся соединиться с роутером в сети
  if (strlen(ssid) > 0) {
    Serial.print(F("\nПодключение к "));
    Serial.print(ssid);

    if (IP_STA[0] + IP_STA[1] + IP_STA[2] + IP_STA[3] > 0) {
      WiFi.config(IPAddress(IP_STA[0], IP_STA[1], IP_STA[2], IP_STA[3]),  // 192.168.0.106
                  IPAddress(IP_STA[0], IP_STA[1], IP_STA[2], 1),          // 192.168.0.1
                  IPAddress(255, 255, 255, 0),                            // Mask
                  IPAddress(IP_STA[0], IP_STA[1], IP_STA[2], 1),          // DNS1 192.168.0.1
                  IPAddress(8, 8, 8, 8));                                 // DNS2 8.8.8.8                  
    }              
    WiFi.begin(ssid, pass);
  
    // Проверка соединения (таймаут 5 секунд)
    for (int j = 0; j < 10; j++ ) {
      wifi_connected = WiFi.status() == WL_CONNECTED; 
      if (wifi_connected) {
        // Подключение установлено
        Serial.println();
        Serial.print(F("WiFi подключен. IP адрес: "));
        Serial.println(WiFi.localIP());
        break;
      }
      delay(500);
      Serial.print(".");
    }
    Serial.println();

    if (!wifi_connected)
      Serial.println(F("Не удалось подключиться к сети WiFi."));
  }  
}

void startSoftAP() {
  WiFi.softAPdisconnect(true);
  ap_connected = false;

  Serial.print(F("Создание точки доступа "));
  Serial.print(apName);
  
  ap_connected = WiFi.softAP(apName, apPass);

  for (int j = 0; j < 10; j++ ) {    
    if (ap_connected) {
      Serial.println();
      Serial.print(F("Точка доступа создана. Сеть: '"));
      Serial.print(apName);
      // Если пароль совпадает с паролем по умолчанию - печатать для информации,
      // если был изменен пользователем - не печатать
      if (strcmp(apPass, "12341234") == 0) {
        Serial.print(F("'. Пароль: '"));
        Serial.print(apPass);
      }
      Serial.println(F("'."));
      Serial.print(F("IP адрес: "));
      Serial.println(WiFi.softAPIP());
      break;
    }    
    
    WiFi.enableAP(false);
    WiFi.softAPdisconnect(true);
    delay(500);
    
    Serial.print(".");
    ap_connected = WiFi.softAP(apName, apPass);
  }  
  Serial.println();  

  if (!ap_connected) 
    Serial.println(F("Не удалось создать WiFi точку доступа."));
}

void printNtpServerName() {
  Serial.print(F("NTP-сервер "));
  Serial.print(ntpServerName);
  Serial.print(F(" -> "));
  Serial.println(timeServerIP);
}

void connectToNetwork() {
  // Подключиться к WiFi сети
  startWiFi();

  // Если режим точки тоступане используется и к WiFi сети подключиться не удалось - создать точку доступа
  if (!wifi_connected){
    WiFi.mode(WIFI_AP);
    startSoftAP();
  }

  if (useSoftAP && !ap_connected) startSoftAP();    

  // Сообщить UDP порт, на который ожидаются подключения
  if (wifi_connected || ap_connected) {
    Serial.print(F("UDP-сервер на порту "));
    Serial.println(localPort);
  }
}

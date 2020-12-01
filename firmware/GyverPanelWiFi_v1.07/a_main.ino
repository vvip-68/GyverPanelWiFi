
// ----------------------------------------------------

/*
// Временно для вывода информации о времени цикла
uint32_t last_ms = millis();  
char data[100];
*/

void process() {  

  /*
  // Время прохода одного цикла
  uint16_t duration = millis() - last_ms;
  if (duration > 0) {
    sprintf(data, "duration=%d", duration);
    Serial.println(data);
  }
  last_ms = millis();
  */

  // принимаем данные
  parsing();                           

  // Если включен эффект с кодом большим кол-ва эффектов (но меньшим кода специальных эффектов) - 
  // выбрать случайный эффект, иначе будет отображаться черный экран или застывший предыдущий эффект
  if (thisMode >= MAX_EFFECT && thisMode < SPECIAL_EFFECTS_START) {
    setRandomMode2(); 
  }

  if (tmpSaveMode != thisMode) {

    String out, effect_name;

    #if (USE_MQTT == 1)
    DynamicJsonDocument doc(256);
    doc["act"] = F("MODE");
    doc["id"] = thisMode;
    #endif

    switch (thisMode) {
      case MC_CLOCK:
        tmpSaveMode = thisMode;
        if (isNightClock)
          effect_name = F("'Ночные часы'");
        else
          effect_name = F("'Часы'");
        #if (USE_MQTT == 1)
        doc["name"] = effect_name;
        serializeJson(doc, out);    
        NotifyInfo(out);
        #else
        Serial.println(String(F("Режим: ")) + effect_name);
        #endif
        break;
      case MC_TEXT:
        tmpSaveMode = thisMode;
        effect_name = F("'Бегущая строка'");
        #if (USE_MQTT == 1)
        doc["name"] = effect_name;
        serializeJson(doc, out);    
        NotifyInfo(out);
        #else
        Serial.println(String(F("Режим: ")) + effect_name);
        #endif
        break;
      case MC_LOADIMAGE:
        tmpSaveMode = thisMode;
        effect_name = F("'Загрузка изображения'");
        #if (USE_MQTT == 1)
        doc["name"] = effect_name;
        serializeJson(doc, out);    
        NotifyInfo(out);
        #else
        Serial.println(String(F("Режим: ")) + effect_name);
        #endif
        break;
      case MC_DRAW:
        tmpSaveMode = thisMode;
        effect_name = F("'Рисование'");
        #if (USE_MQTT == 1)
        doc["name"] = effect_name;
        serializeJson(doc, out);    
        NotifyInfo(out);
        #else
        Serial.println(String(F("Режим: ")) + effect_name);
        #endif
        break;
      default:
        // Определить какой эффект включился
        String s_tmp = String(EFFECT_LIST);    
        uint16_t len1 = s_tmp.length();
        s_tmp = GetToken(s_tmp, thisMode+1, ',');
        uint16_t len2 = s_tmp.length();
        if (len1 > len2) { 
          tmpSaveMode = thisMode;
          effect_name = "'" + s_tmp + "'";
          #if (USE_MQTT == 1)
          doc["name"] = effect_name;
          serializeJson(doc, out);    
          NotifyInfo(out);
          #else
          Serial.println(String(F("Режим: ")) + effect_name);
          #endif
        } else {
          // Если режим отсутствует в списке эффектов - включить случайный, если включенный режим не один из спец-режимов
          setRandomMode2(); 
        }    
        break;
    }
  }

  // на время принятия данных матрицу не обновляем!
  if (!parseStarted) {

    // Раз в час выполнять пересканирование текстов бегущих строк на наличие события непрерывного отслеживания.
    // При сканировании события с нечеткими датами (со звездочками) просматриваются не далее чем на сутки вперед
    // События с более поздним сроком не попадут в отслеживание. Поэтому требуется периодическое перестроение списка.
    // Сканирование требуется без учета наличия соединения с интернетом и значения флага useNTP - время может быть установлено вручную с телефона
    if (init_time && (millis() - textCheckTime > SECS_PER_HOUR * 1000L)) {
      rescanTextEvents();
    }

    if (wifi_connected) {

      // Если настройки программы предполагают синхронизацию с NTP сервером и время пришло - выполнить синхронизацию
      if (useNtp) {
        if ((ntp_t > 0) && getNtpInProgress && (millis() - ntp_t > 5000)) {
          Serial.println(F("Таймаут NTP запроса!"));
          ntp_cnt++;
          getNtpInProgress = false;
          if (init_time && ntp_cnt >= 10) {
            Serial.println(F("Не удалось установить соединение с NTP сервером."));  
            refresh_time = false;
          }
          
          #if (USE_MQTT == 1)
          DynamicJsonDocument doc(256);
          String out;
          doc["act"] = F("TIME");
          doc["server_name"] = ntpServerName;
          doc["server_ip"] = timeServerIP.toString();
          doc["result"] = F("TIMEOUT");
          serializeJson(doc, out);      
          NotifyInfo(out);
          #endif
        }
        
        bool timeToSync = ntpSyncTimer.isReady();
        if (timeToSync) { ntp_cnt = 0; refresh_time = true; }
        if (timeToSync || (refresh_time && (ntp_t == 0 || (millis() - ntp_t > 60000)) && (ntp_cnt < 10 || !init_time))) {
          ntp_t = millis();
          getNTP();
          /*
          if (ntp_cnt >= 10) {
            if (init_time) {
              udp.flush();
            } else {
              //ESP.restart();
              ntp_cnt = 0;
              connectToNetwork();
            }
          } 
          */       
        }
      }

      #if (USE_WEATHER == 1)  
        if (useWeather > 0) {   
          // Если настройки программы предполагают получение сведений о текущей погоде - выполнить обновление данных с погодного сервера
          if ((weather_t > 0) && getWeatherInProgress && (millis() - weather_t > 5000)) {
            Serial.println(F("Таймаут запроса погоды!"));
            getWeatherInProgress = false;
            weather_cnt++;
            if (init_weather && weather_cnt >= 10) {
              Serial.println(F("Не удалось установить соединение с сервером погоды."));  
              refresh_weather = false;
              
              #if (USE_MQTT == 1)
              DynamicJsonDocument doc(256);
              String out;
              doc["act"] = F("WEATHER");
              doc["region"] = useWeather == 1 ? regionID : regionID2;
              doc["result"] = F("TIMEOUT");
              serializeJson(doc, out);      
              NotifyInfo(out);
              #endif
            }
          }
          
          bool timeToGetWeather = weatherTimer.isReady(); 
          if (timeToGetWeather) { weather_cnt = 0; weather_t = 0; refresh_weather = true; }
          // weather_t - время последней отправки запроса. Запрашивать погоду если weather_t обнулено или если последний (неудачный) запрос произошел не менее чем минуту назад
          // иначе слишком частые запросы нарушают коммуникацию с приложением - сервер все время блокирующе запрашивает данные с сервера погоды
          if (timeToGetWeather || (refresh_weather && (weather_t == 0 || (millis() - weather_t > 60000)) && (weather_cnt < 10 || !init_weather))) {            
            weather_t = millis();
            getWeatherInProgress = true;
            getWeather();
            if (weather_cnt >= 10) {
              if (init_weather) {
                udp.flush();
              } else {
                weather_cnt = 0;
              }
            }        
          }
          if (init_weather && (millis() - weather_time > weatherActualityDuration * 3600L * 1000L)) {
            init_weather = false;
            refresh_weather = true;
          }
        }
      #endif
    }

    // Сформировать и вывести на матрицу текущий демо-режим
    // При яркости = 1 остаются гореть только красные светодиоды и все эффекты теряют вид.
    // поэтому отображать эффект "ночные часы"
    byte br = specialMode ? specialBrightness : globalBrightness;
    if (br == 1 && !(loadingFlag || isAlarming || thisMode == MC_TEXT)) {
      customRoutine(MC_CLOCK);    
    } else {    
      customRoutine(thisMode);
    }
    
    clockTicker();
    
    checkAlarmTime();
    checkAutoMode1Time();
    checkAutoMode2Time();
    checkAutoMode3Time();
    checkAutoMode4Time();

    butt.tick();  // обязательная функция отработки. Должна постоянно опрашиваться
    byte clicks = 0;

    // Один клик
    if (butt.isSingle()) clicks = 1;    
    // Двойной клик
    if (butt.isDouble()) clicks = 2;
    // Тройной клик
    if (butt.isTriple()) clicks = 3;
    // Четверной и более клик
    if (butt.hasClicks()) clicks = butt.getClicks();
    
    if (butt.isPress()) {
      // Состояние - кнопку нажали  
    }
    
    if (butt.isRelease()) {
      // Состояние - кнопку отпустили
      isButtonHold = false;
    }
    
    if (butt.isHolded()) {
      isButtonHold = true;
      if (!isTurnedOff && thisMode != MC_DAWN_ALARM) {
        if (globalBrightness == 255)
          brightDirection = false;
        else if (globalBrightness == 0)  
          brightDirection = true;
        else  
          brightDirection = !brightDirection;
      }
    }

    if (clicks > 0) {
      Serial.print(F("Кнопка нажата "));  
      Serial.print(String(clicks));
      Serial.println(F(" раз"));  
    }

    // Любое нажатие кнопки останавливает будильник
    if ((isAlarming || isPlayAlarmSound) && (isButtonHold || clicks > 0)) {
      stopAlarm();
      clicks = 0;
    }

    // Одинарный клик - включить . выключить панель
    if (clicks == 1) {
      if (isTurnedOff) {
        // Если выключен - включить панель, восстановив эффект на котором панель была выключена
        if (saveSpecialMode && saveSpecialModeId != 0) 
          setSpecialMode(saveSpecialModeId);
        else {
          saveMode = getCurrentManualMode();
          if (saveMode == 0 && globalColor == 0) globalColor = 0xFFFFFF;
          Serial.println(String(F("Вкл: ")) + String(saveMode));
          manualMode = getAutoplay();
          setManualModeTo(manualMode);
          setEffect(saveMode);
        }
      } else {
        // Выключить панель, запомнив текущий режим
        saveSpecialMode = specialMode;
        saveSpecialModeId = specialModeId;
        saveMode = thisMode;
        bool mm = manualMode;
        // Выключить панель - черный экран
        setSpecialMode(0);
        setCurrentManualMode(saveMode);
        saveAutoplay(mm);
        Serial.println(String(F("Выкл: ")) + String(saveMode));
      }
    }
    
    // Прочие клики работают только если не выключено
    if (isTurnedOff) {
      // Выключить питание матрицы
      #if (USE_POWER == 1)
        digitalWrite(POWER_PIN, POWER_OFF);
      #endif      
      //  - длительное нажатие кнопки включает яркий белый свет
#if (DEVICE_TYPE == 0)
      if (isButtonHold) {
        // Включить панель - белый цвет
        specialBrightness = 255;
        globalBrightness = 255;
        globalColor = 0xFFFFFF;
        isButtonHold = false;
        setSpecialMode(1);
        FastLED.setBrightness(globalBrightness);
      }
#endif      
    } else {
      // Включить питание матрицы
      #if (USE_POWER == 1)
        digitalWrite(POWER_PIN, POWER_ON);
      #endif
      
      // Был двойной клик - следующий эффект, сброс автоматического переключения
      if (clicks == 2) {
        bool tmpSaveSpecial = specialMode;
        resetModes();  
        setManualModeTo(true);
        if (tmpSaveSpecial) setRandomMode();
        else                nextMode();
      }

      // Тройное нажатие - включить случайный режим с автосменой
      else if (clicks == 3) {
        // Включить демо-режим
        resetModes();          
        setManualModeTo(false);        
        setRandomMode();
      }

// Если устройство - лампа
#if (DEVICE_TYPE == 0)
      // Четверное нажатие - включить белую лампу независимо от того была она выключена или включен любой другой режим
      if (clicks == 4) {
        // Включить лампу - белый цвет
        specialBrightness = 255;
        globalBrightness = 255;
        globalColor = 0xFFFFFF;
        setSpecialMode(1);
        FastLED.setBrightness(globalBrightness);
      }      
      // Пятикратное нажатие - показать текущий IP WiFi-соединения            
      else if (clicks == 5) {
        showCurrentIP(false);
      }      
#else      
      // Четырехкратное нажатие - показать текущий IP WiFi-соединения            
      else if (clicks == 4) {
        showCurrentIP(false);
      }      
#endif
      // ... и т.д.
      
      // Обработка нажатой и удерживаемой кнопки
      else {

        // Удержание кнопки повышает / понижает яркость панели (лампы)
        if (butt.isStep() && thisMode != MC_DAWN_ALARM) {
          if (brightDirection) {
            if (globalBrightness < 10) globalBrightness += 1;
            else if (globalBrightness < 250) globalBrightness += 5;
            else {
              globalBrightness = 255;
            }
          } else {
            if (globalBrightness > 15) globalBrightness -= 5;
            else if (globalBrightness > 1) globalBrightness -= 1;
            else {
              globalBrightness = 1;
            }
          }

          specialBrightness = globalBrightness;
        
          FastLED.setBrightness(globalBrightness);
          
          saveMaxBrightness(globalBrightness);
        }
      }            
    }

    #if (USE_MP3 == 1)
    // Есть ли изменение статуса MP3-плеера?
    if (dfPlayer.available()) {

      // Вывести детали об изменении статуса в лог
      byte msg_type = dfPlayer.readType();      
      printDetail(msg_type, dfPlayer.read());

      // Действия, которые нужно выполнить при изменении некоторых статусов:
      if (msg_type == DFPlayerCardRemoved) {
        // Карточка "отвалилась" - делаем недоступным все что связано с MP3 плеером
        isDfPlayerOk = false;
        alarmSoundsCount = 0;
        dawnSoundsCount = 0;
        Serial.println(F("MP3 плеер недоступен."));
      } else if (msg_type == DFPlayerCardOnline || msg_type == DFPlayerCardInserted) {
        // Плеер распознал карту - переинициализировать стадию 2
        InitializeDfPlayer2();
        if (!isDfPlayerOk) Serial.println(F("MP3 плеер недоступен."));
      }
    }
    #endif
    
    // Проверить - если долгое время не было ручного управления - переключиться в автоматический режим
    if (!(isAlarming || isPlayAlarmSound)) checkIdleState();

    // Если есть несохраненные в EEPROM данные - сохранить их
    if (saveSettingsTimer.isReady()) {
      saveSettings();
    }
  }
}

// ********************* ПРИНИМАЕМ ДАННЫЕ **********************

void parsing() {
  
  // ****************** ОБРАБОТКА *****************
  String str, str1, str2;
  byte b_tmp;
  int8_t tmp_eff;
  char c;
  bool err = false;
  int pntX, pntY, pntColor, pntIdx, idx;
  String pictureLine;
  byte alarmHourVal;
  byte alarmMinuteVal;

// Программа передает изображение в матрицу по стороне с меньшей длиной, чтобы избежать переполнение буфера приема 
// Если ширина матрицы больше чем ее высота - передача будет происходить по колонкам слева направо
// Если ширина матрицы меньше чем ее высота - передача будет происходить по строкам сверху вниз
#if (HEIGHT > WIDTH)
  String pntPart[WIDTH];      // массив разобранной входной строки на строки точек
#else
  String pntPart[HEIGHT];     // массив разобранной входной строки на строки точек
#endif  
  /*
    Протокол связи, посылка начинается с режима. Режимы:
    3 - управление играми из приложение WiFi Panel Player
      - $3 0;   включить на устройстве демо-режим
      - $3 1;   включить игру "Лабиринт" в режиме ожидания начала игры
      - $3 2;   включить игру "Змейка" в режиме ожидания начала игры
      - $3 3;   включить игру "Тетрис" в режиме ожидания начала игры
      - $3 4;   включить игру "Арканоид" в режиме ожидания начала игры
      - $3 10 - кнопка вверх
      - $3 11 - кнопка вправо
      - $3 12 - кнопка вниз
      - $3 13 - кнопка влево
      - $3 14 - центральная кнопка джойстика (ОК)
    4 - яркость - 
      $4 0 value   установить текущий уровень общей яркости
    5 - рисование
      - $5 0 RRGGBB; - установить активный цвет рисования в формате RRGGBB
      - $5 1;        - очистить матрицу (заливка черным)
      - $5 2;        - заливка матрицы активным активным цветом рисования 
      - $5 3 X Y;    - рисовать точку активным цветом рисования в позицию X Y
    6 - текст $6 N|some text, где N - назначение текста;
        0 - текст бегущей строки N|X|text - X - 0..9,A..Z - индекс строки
        1 - имя сервера NTP
        2 - SSID сети подключения
        3 - пароль для подключения к сети 
        4 - имя точки доступа
        5 - пароль к точке доступа
        6 - настройки будильников
        7 - строка запрашиваемых параметров для процедуры getStateString(), например - "CE|CC|CO|CK|NC|SC|C1|DC|DD|DI|NP|NT|NZ|NS|DW|OF"
        8 - имя сервера MQTT
        9 - имя пользователя MQTT
       10 - пароль к MQTT-серверу
       11 - картинка построчно $6 11|Y colorHEX X|colorHEX X|...|colorHEX X;
       12 - картинка по колонкам $6 12|X colorHEX Y|colorHEX Y|...|colorHEX Y;
    8 - эффект
      - $8 0 N; включить эффект N
      - $8 1 N D; D -> параметр #1 для эффекта N;
      - $8 2 N X; вкл/выкл использовать в демо-режиме; N - номер эффекта, X=0 - не использовать X=1 - использовать 
      - $8 3 N D; D -> параметр #2 для эффекта N;
      - $8 4 N X; вкл/выкл оверлей текста поверх эффекта; N - номер эффекта, X=0 - выкл X=1 - вкл 
      - $8 5 N X; вкл/выкл оверлей часов поверх эффекта; N - номер эффекта, X=0 - выкл X=1 - вкл 
      - $8 6 N D; D -> контрастность эффекта N;
    11 - Настройки MQTT-канала (см. также $6 для N=8,9,10)
      - $11 1 X;   - использовать управление через MQTT сервер X; 0 - не использовать; 1 - использовать
      - $11 2 D;   - порт MQTT
      - $11 3 X;   - Флаг - использует ли MQTT сервер префикс - имя пользователя при формировании топика
      - $11 4 D;   - Задержка между последовательными обращениями к MQTT серверу
      - $11 5;     - Разорвать подключение к MQTT серверу, чтобы он иог переподключиться с новыми параметрами
    12 - Настройки погоды
      - $12 3 X;   - использовать цвет для отображения температуры X=0 - выкл X=1 - вкл в дневных часах
      - $12 4 X;   - использовать получение погоды с погодного сервера
      - $12 5 I С C2; - интервал получения погоды с сервера в минутах (I) и код региона C - Yandex и код региона C2 - OpenWeatherMap
      - $12 6 X;   - использовать цвет для отображения температуры X=0 - выкл X=1 - вкл в ночных часах
    13 - Настройки бегущей cтроки  
      - $13 0 N; - активация для редактирования строки с номером N - запрос текста строки
      - $13 1 N; - активация прокручивания строки с номером N
      - $13 2 I; - запросить текст бегущей строки с индексом I как есть, без обработки макросов
      - $13 3 I; - запросить текст бегущей строки с индексом I с обработкой макросов
      - $13 9 I; - сохранить настройку I - интервал в секундах отображения бегущей строки
      - $13 11 X; - Режим цвета бегущей строки X: 0,1,2,           
      - $13 13 X; - скорость прокрутки бегущей строки
      - $13 15 00FFAA; - цвет текстовой строки для режима "монохромный", сохраняемый в globalTextColor
      - $13 18 X; - сохранить настройку X "Бегущая строка в эффектах" (общий, для всех эффектов)
    14 - быстрая установка ручных режимов с пред-настройками
       - $14 0;  Черный экран (выкл);  
       - $14 1;  Белый экран (освещение);  
       - $14 2;  Цветной экран;  
       - $14 3;  Огонь;  
       - $14 4;  Конфетти;  
       - $14 5;  Радуга;  
       - $14 6;  Матрица;  
       - $14 7;  Светлячки;  
       - $14 8;  Часы ночные;
       - $14 9;  Часы бегущей строкой;
       - $14 10; Часы простые;  
    15 - скорость $15 скорость таймер; 0 - таймер эффектов
    16 - Режим смены эффектов: $16 value; N: 0 - ручной режим;  1 - авторежим; 2 - PrevMode; 3 - NextMode; 5 - вкл/выкл случайный выбор следующего режима
    17 - Время автосмены эффектов и бездействия: $17 сек сек;
    18 - Запрос текущих параметров программой: $18 page; page - страница настройки в программе на смартфоне (1..7) или специальный параметр (91..99)
         page 1:  // Настройки
         page 2:  // Эффекты
         page 3:  // Настройки бегущей строки
         page 4:  // Настройки часов
         page 5:  // Настройки будильника
         page 6:  // Настройки подключения
         page 7:  // Настройки режимов автовключения по времени
         page 10: // Загрузка картинок
         page 11: // Рисование
         page 12: // Игры
         page 91: // Запрос текста бегущей строки как есть без обработки макросов
         page 92: // Запрос текста бегущей строки с обработкой макросов для заполнения списка в программе
         page 93: // Запрос списка звуков будильника
         page 94: // Запрос списка звуков рассвета
         page 95: // Ответ состояния будильника - сообщение по инициативе сервера
         page 96: // Ответ демо-режима звука - сообщение по инициативе сервера
         page 99: // Запрос списка эффектов
    19 - работа с настройками часов
      - $19 1 X; - сохранить настройку X "Часы в эффектах"
      - $19 2 X; - Использовать синхронизацию часов NTP  X: 0 - нет, 1 - да
      - $19 3 N Z; - Период синхронизации часов NTP и Часовой пояс
      - $19 4 X; - Выключать индикатор TM1637 при выключении экрана X: 0 - нет, 1 - да
      - $19 5 X; - Режим цвета часов оверлея X: 0,1,2,3
      - $19 6 X; - Ориентация часов  X: 0 - горизонтально, 1 - вертикально
      - $19 7 X; - Размер часов X: 0 - авто, 1 - малые 3х5, 2 - большие 5x7
      - $19 8 YYYY MM DD HH MM; - Установить текущее время YYYY.MM.DD HH:MM
      - $19 9 X; - Показывать температуру вместе с малыми часами 1 - да; 0 - нет
      - $19 10 X; - Цвет ночных часов:  0 - R; 1 - G; 2 - B; 3 - C; 3 - M; 5 - Y; 6 - W;
      - $19 11 X; - Яркость ночных часов:  0..255
      - $19 12 X; - скорость прокрутки часов оверлея или 0, если часы остановлены по центру
      - $19 14 00FFAA; - цвет часов оверлея, сохраняемый в globalClockColor
      - $19 16 X; - Показывать дату в режиме часов  X: 0 - нет, 1 - да
      - $19 17 D I; - Продолжительность отображения даты / часов (в секундах)
    20 - настройки и управление будильников
      - $20 0;       - отключение будильника (сброс состояния isAlarming)
      - $20 2 X VV MA MB;
           X    - исп звук будильника X=0 - нет, X=1 - да 
          VV    - максимальная громкость
          MA    - номер файла звука будильника
          MB    - номер файла звука рассвета
      - $20 3 X NN VV; - пример звука будильника
           X - 1 играть 0 - остановить
          NN - номер файла звука будильника из папки SD:/01
          VV - уровень громкости
      - $20 4 X NN VV; - пример звука рассвета
           X - 1 играть 0 - остановить
          NN - номер файла звука рассвета из папки SD:/02
          VV - уровень громкости
      - $20 5 VV; - установить уровень громкости проигрывания примеров (когда уже играет)
          VV - уровень громкости
    21 - настройки подключения к сети / точке доступа
      - $21 0 X - использовать точку доступа: X=0 - не использовать X=1 - использовать
      - $21 1 IP1 IP2 IP3 IP4 - установить статический IP адрес подключения к локальной WiFi сети, пример: $21 1 192 168 0 106
      - $21 2; Выполнить переподключение к сети WiFi
    22 - настройки включения режимов матрицы в указанное время
       - $22 HH1 MM1 NN1 HH2 MM2 NN2 HH3 MM3 NN3 HH4 MM4 NN4
             HHn - час срабатывания
             MMn - минуты срабатывания
             NNn - эффект: -3 - выключено; -2 - выключить матрицу; -1 - ночные часы; 0 - случайный режим и далее по кругу; 1 и далее - список режимов EFFECT_LIST 
    23 - прочие настройки
       - $23 0 VAL  - лимит по потребляемому току
  */  

  // Если прием данных завершен и управляющая команда в intData[0] распознана
  if (recievedFlag && intData[0] > 0 && intData[0] <= 23) {
    recievedFlag = false;

    // Режим 18  не сбрасывают idleTimer
    if (intData[0] != 18) {
      idleTimer.reset();
      idleState = false;      
    }

    // Режимы кроме 18 останавливают будильник, если он работает (идет рассвет)
    if (intData[0] != 18) {
      wifi_print_ip = false;
      stopAlarm();
    }
    
    switch (intData[0]) {

      // ----------------------------------------------------
      // 3 - управление играми из приложение WiFi Panel Player
      //   - $3 0;   включить на устройстве демо-режим
      //   - $3 1;   включить игру "Лабиринт" в режиме ожидания начала игры
      //   - $3 2;   включить игру "Змейка" в режиме ожидания начала игры
      //   - $3 3;   включить игру "Тетрис" в режиме ожидания начала игры
      //   - $3 4;   включить игру "Арканоид" в режиме ожидания начала игры
      //   - $3 10 - кнопка вверх
      //   - $3 11 - кнопка вправо
      //   - $3 12 - кнопка вниз
      //   - $3 13 - кнопка влево
      //   - $3 14 - центральная кнопка джойстика (ОК)
      // ----------------------------------------------------

      case 3:
        if (intData[1] == 0) {
          // Включить на устройстве демо-режим
          FastLED.clear();
          FastLED.show();
          delay(50);
          setManualModeTo(false);
          nextMode();
          sendAcknowledge(cmdSource);
        } else
        if (intData[1] == 1) {
          // Игра "Лабиринт"
          setManualModeTo(true);
          setEffect(MC_MAZE);
          gameDemo = false;
          gamePaused = true;
          sendPageParams(12, cmdSource);
        } else
        if (intData[1] == 2) {
          // Игра "Змейка"
          setManualModeTo(true);
          setEffect(MC_SNAKE);
          gameDemo = false;
          gamePaused = true;
          sendPageParams(12, cmdSource);
        } else
        if (intData[1] == 3) {
          // Игра "Тетрис"
          setManualModeTo(true);
          setEffect(MC_TETRIS);
          gameDemo = false;
          gamePaused = true;
          sendPageParams(12, cmdSource);
        } else
        if (intData[1] == 4) {
          // Игра "Арканоид"
          setManualModeTo(true);
          setEffect(MC_ARKANOID);
          gameDemo = false;
          gamePaused = true;
          sendPageParams(12, cmdSource);
        } else
        if (intData[1] == 10) {
          // кнопка вверх
          buttons = 0;
          gamePaused = false;
          sendAcknowledge(cmdSource);
        } else
        if (intData[1] == 11) {
          // кнопка вправо
          buttons = 1;
          gamePaused = false;
          sendAcknowledge(cmdSource);
        } else
        if (intData[1] == 12) {
          // кнопка вниз
          buttons = 2;
          gamePaused = false;
          sendAcknowledge(cmdSource);
        } else
        if (intData[1] == 13) {
          // кнопка влево
          buttons = 3;
          gamePaused = false;
          sendAcknowledge(cmdSource);
        } else
        if (intData[1] == 14) {
          // кнопка влево
          buttons = 5;
          gamePaused = false;
          sendAcknowledge(cmdSource);
        }
        else {
          #if (USE_MQTT == 1)
          notifyUnknownCommand(incomeBuffer);
          #endif
        }        
        break;

      // ----------------------------------------------------
      // 4 - яркость - 
      //  $4 0 value   установить текущий уровень общей яркости / яркости ночных часов
      // ----------------------------------------------------
      
      case 4:
        if (intData[1] == 0) {
          // При включенном режиме ночных часов ползунок яркости регулирует только яркость ночных часов
          // Для прочих режимов - общую яркость системы
          if (isNightClock) {
            nightClockBrightness = intData[2];
            if (nightClockBrightness < 2) nightClockBrightness = 2;
            setNightClockBrightness(nightClockBrightness);
            specialBrightness = nightClockBrightness < MIN_BRIGHT_FOR_NIGHT ? MIN_BRIGHT_FOR_NIGHT : nightClockBrightness;
            FastLED.setBrightness(specialBrightness);
          } else {
            globalBrightness = intData[2];
            saveMaxBrightness(globalBrightness);
            if (specialMode) specialBrightness = globalBrightness;
            FastLED.setBrightness(globalBrightness);
          }
          // Для команд, пришедших от MQTT отправлять только ACK;
          // Для команд, пришедших от UDP отправлять при необходимости другие данные, например - состояние элементов управления на странице от которой пришла команда 
          sendAcknowledge(cmdSource);
        } else {
          #if (USE_MQTT == 1)
          notifyUnknownCommand(incomeBuffer);
          #endif
        }
        break;

      // ----------------------------------------------------
      // 5 - рисование
      //   $5 0 RRGGBB; - установить активный цвет рисования в формате RRGGBB
      //   $5 1;        - очистить матрицу (заливка черным)
      //   $5 2;        - заливка матрицы активным активным цветом рисования 
      //   $5 3 X Y;    - рисовать точку активным цветом рисования в позицию X Y
      // ----------------------------------------------------

      case 5:
        if (thisMode != MC_DRAW) {
          if (thisMode != MC_LOADIMAGE) {
            FastLED.clear();
            FastLED.show();
            delay(50);
          }
          thisMode = MC_DRAW;
          setManualModeTo(true);
        }      
        if (intData[1] == 0) {
          // Установить активный цвет рисования - incomeBuffer = '$5 0 00FF00;'
          str = String(incomeBuffer).substring(5,11);
          drawColor = HEXtoInt(str);
          sendAcknowledge(cmdSource);
        } else
        if (intData[1] == 1) {
          // Очистка матрицы (заливка черным)
          FastLED.clear();
          sendAcknowledge(cmdSource);
        } else
        if (intData[1] == 2) {
          // Залить матрицу активным цвет ом рисования
          fillAll(drawColor);
          sendAcknowledge(cmdSource);
        } else
        if (intData[1] == 3) {
          // Нарисовать точку активным цветом рисования в позиции X Y
          drawPixelXY(intData[2], intData[3], gammaCorrection(drawColor));
          sendAcknowledge(cmdSource);
        }         
        else {
          #if (USE_MQTT == 1)
          notifyUnknownCommand(incomeBuffer);
          #endif
        }
        break;

      // ----------------------------------------------------
      // 6 - прием строки: строка принимается в формате N|text, где N:
      //   0 - принятый текст бегущей строки X|text - X - 0..9,A..Z - индекс строки
      //   1 - имя сервера NTP
      //   2 - имя сети (SSID)
      //   3 - пароль к сети
      //   4 - имя точки доступа
      //   5 - пароль точки доступа
      //   6 - настройки будильника в формате $6 6|DD EF WD HH1 MM1 HH2 MM2 HH3 MM3 HH4 MM4 HH5 MM5 HH6 MM6 HH7 MM7        
      //   7 - строка запрашиваемых параметров для процедуры getStateString(), например - "$6 7|CE CC CO CK NC SC C1 DC DD DI NP NT NZ NS DW OF"
      //   8 - имя сервера MQTT
      //   9 - имя пользователя MQTT
      //  10 - пароль к MQTT-серверу
      //  11 - картинка построчно $6 11|Y colorHEX X|colorHEX X|...|colorHEX X;
      //  12 - картинка по колонкам $6 12|X colorHEX Y|colorHEX Y|...|colorHEX Y;   - пока не реализовано (зарезервировано)
      // ----------------------------------------------------

      case 6:
        b_tmp = 0;
        tmp_eff = receiveText.indexOf("|");
        if (tmp_eff > 0) {
           b_tmp = receiveText.substring(0, tmp_eff).toInt();
           str = receiveText.substring(tmp_eff+1, receiveText.length()+1);
           switch(b_tmp) {

            case 0:
              c = str[0];
              str = str.substring(2);
              str.replace('\r', ' ');
              str.replace('\n', ' ');
              str.trim();
              tmp_eff = getTextIndex(c); // '0'..'9','A'..'Z' -> 0..35
              if (tmp_eff >= 0) {
                textLines[tmp_eff] = str;                
              }
              if (tmp_eff == 0 && textLines[0] == "#") {
                 textLines[0] = "##";
              }
              saveTexts();
              rescanTextEvents();
              break;

            case 1:
              setNtpServer(str);
              getNtpServer().toCharArray(ntpServerName, 31);
              if (wifi_connected) {
                refresh_time = true; ntp_t = 0; ntp_cnt = 0;
              }
              break;

            case 2:
              str.toCharArray(ssid, 24);
              setSsid(str);
              break;

            case 3:
              str.toCharArray(pass, 16);
              setPass(str);
              break;

            case 4:
              str.toCharArray(apName, 10);
              setSoftAPName(str);
              break;

            case 5:
              str.toCharArray(apPass, 16);
              setSoftAPPass(str);
              // Передается в одном пакете - использовать SoftAP, имя точки и пароль
              // После получения пароля - перезапустить создание точки доступа
              if (useSoftAP) startSoftAP();
              break;
              
            case 6:
              // Настройки будильника в формате $6 6|DD EF WD AD HH1 MM1 HH2 MM2 HH3 MM3 HH4 MM4 HH5 MM5 HH6 MM6 HH7 MM7
              // DD    - установка продолжительности рассвета (рассвет начинается за DD минут до установленного времени будильника)
              // EF    - установка эффекта, который будет использован в качестве рассвета
              // WD    - установка дней пн-вс как битовая маска
              // AD    - продолжительность "звонка" сработавшего будильника
              // HHx   - часы дня недели x (1-пн..7-вс)
              // MMx   - минуты дня недели x (1-пн..7-вс)
              //
              // Остановить будильник, если он сработал
              #if (USE_MP3 == 1)
              if (isDfPlayerOk) {
                dfPlayer.stop();
              }
              soundFolder = 0;
              soundFile = 0;
              #endif
              
              isAlarming = false;
              isAlarmStopped = false;

              // Настройки содержат 18 элементов (см. формат выше)
              tmp_eff = CountTokens(str, ' ');
              if (tmp_eff == 18) {
              
                dawnDuration = constrain(GetToken(str, 1, ' ').toInt(),1,59);
                alarmEffect = GetToken(str, 2, ' ').toInt();
                alarmWeekDay = GetToken(str, 3, ' ').toInt();
                alarmDuration = constrain(GetToken(str, 4, ' ').toInt(),1,10);
                saveAlarmParams(alarmWeekDay,dawnDuration,alarmEffect,alarmDuration);
                
                for(byte i=0; i<7; i++) {
                  alarmHourVal = constrain(GetToken(str, i*2+5, ' ').toInt(), 0, 23);
                  alarmMinuteVal = constrain(GetToken(str, i*2+6, ' ').toInt(), 0, 59);
                  alarmHour[i] = alarmHourVal;
                  alarmMinute[i] = alarmMinuteVal;
                  setAlarmTime(i+1, alarmHourVal, alarmMinuteVal);
                }

                // Рассчитать время начала рассвета будильника
                calculateDawnTime();            
              }
              break;
              
            case 7:
              // Запрос значений параметров, требуемых приложением вида "CE|CC|CO|CK|NC|SC|C1|DC|DD|DI|NP|NT|NZ|NS|DW|OF"
              // Каждый запрашиваемый приложением параметр - для заполнения соответствующего поля в приложении 
              // Передать строку для формирования, затем отправить параметры в приложение
              str = "$18 " + getStateString(str) + ";";
              break;
              
            #if (USE_MQTT == 1)
            case 8:
              str.toCharArray(mqtt_server, 24);
              setMqttServer(str);
              break;
            case 9:
              str.toCharArray(mqtt_user, 14);
              setMqttUser(str);
              break;
            case 10:
              str.toCharArray(mqtt_pass, 14);
              setMqttPass(str);
              break;
            #endif

            // Прием строки передаваемого отображения по строкам  $6 11|Y colorHEX X|colorHEX X|...|colorHEX X;
            case 11:
              pictureLine = str;
              
              if (thisMode != MC_LOADIMAGE) {
                setManualModeTo(true);
                thisMode = MC_LOADIMAGE;
                FastLED.clear();
                FastLED.show();
                delay(50);
              }
      
              // Разбираем СТРОКУ из принятого буфера формата 'Y colorHEX X|colorHEX X|...|colorHEX X'
              // Получить номер строки (Y) для которой получили строку с данными (номер строки - сверху вниз, в то время как матрица - индекс строки снизу вверх)
              b_tmp = pictureLine.indexOf(" ");
              str = pictureLine.substring(0, b_tmp);
              pntY = str.toInt();
              pictureLine = pictureLine.substring(b_tmp+1);
      
              pntIdx = 0;
              idx = pictureLine.indexOf("|");
              while (idx>0)
              {
                str = pictureLine.substring(0, idx);
                pictureLine = pictureLine.substring(idx+1);
                
                pntPart[pntIdx++] = str;
                idx = pictureLine.indexOf("|");
      
                if (idx<0 && pictureLine.length()>0) {
                  pntPart[pntIdx++] = pictureLine;  
                }          
                delay(0);
              }
      
              for (int i=0; i<pntIdx; i++) {
                str = pntPart[i];
                idx = str.indexOf(" ");
                str1 = str.substring(0, idx);
                str2 = str.substring(idx+1);
      
                pntColor = HEXtoInt(str1);
                pntX=str2.toInt();
                
                // начало картинки - очистить матрицу
                if (pntX == 0 && pntY == 0) {
                  FastLED.clear(); 
                  FastLED.show();
                }
                
                drawPixelXY(pntX, HEIGHT - pntY - 1, gammaCorrection(pntColor));
              }
      
              // Выводить построчно для ускорения вывода на экран
              if (pntX == WIDTH - 1) {
                FastLED.show();
              }
              
              // Строка подтверждения приема строки изображения + восстановить переменную выбора типа строки из команды "$6 11|xxx"
              b_tmp = 11;
              str = "#L " + String(pntY)+ "-" + String(pntX) + " ack" + String(ackCounter++) + ";";
              
              break;

            // Прием строки передаваемого отображения по колонкам $6 12|X colorHEX Y|colorHEX Y|...|colorHEX Y;   - пока не реализовано (зарезервировано)
            case 12:
              pictureLine = str;

              if (thisMode != MC_LOADIMAGE) {
                setManualModeTo(true);
                thisMode = MC_LOADIMAGE;
                FastLED.clear();
                FastLED.show();
                delay(50);
              }
      
              // Разбираем СТРОКУ из принятого буфера формата 'X colorHEX Y|colorHEX Y|...|colorHEX Y'
              // Получить номер колонки (X) для которой получили строку с данными (номер строки - сверху вниз, в то время как матрица - индекс строки снизу вверх)
              b_tmp = pictureLine.indexOf(" ");
              str = pictureLine.substring(0, b_tmp);
              pntX = str.toInt();
              pictureLine = pictureLine.substring(b_tmp+1);
      
              pntIdx = 0;
              idx = pictureLine.indexOf("|");
              while (idx>0)
              {
                str = pictureLine.substring(0, idx);
                pictureLine = pictureLine.substring(idx+1);
                                
                pntPart[pntIdx++] = str;
                idx = pictureLine.indexOf("|");
      
                if (idx<0 && pictureLine.length()>0) {
                  pntPart[pntIdx++] = pictureLine;  
                }          
                delay(0);
              }
      
              for (int i=0; i<pntIdx; i++) {
                str = pntPart[i];
                idx = str.indexOf(" ");
                str1 = str.substring(0, idx);
                str2 = str.substring(idx+1);
      
                pntColor = HEXtoInt(str1);
                pntY=str2.toInt();
                
                // начало картинки - очистить матрицу
                if (pntX == 0 && pntY == 0) {
                  FastLED.clear(); 
                  FastLED.show();
                }
                
                drawPixelXY(pntX, HEIGHT - pntY - 1, gammaCorrection(pntColor));
              }
      
              // Выводить построчно для ускорения вывода на экран
              if (pntY == HEIGHT - 1) {
                FastLED.show();
              }
              
              // Строка подтверждения приема строки изображения + восстановить переменную выбора типа строки из команды "$6 11|xxx"
              b_tmp = 12;
              str = "#C " + String(pntX)+ "-" + String(pntY) + " ack" + String(ackCounter++) + ";";
              
              break;
           }
        }

        // При сохранении текста бегущей строки (b_tmp == 0) не нужно сразу автоматически сохранять ее в EEPROM - Сохранение будет выполнено по таймеру. 
        // При получении запроса параметров (b_tmp == 7) ничего сохранять не нужно - просто отправить требуемые параметры
        // При получении очередной строки изображения (b_tmp == 11 или b_tmp == 12) ничего сохранять не нужно
        // Остальные полученные строки - сохранять сразу, ибо это настройки сети, будильники и другая критически важная информация
        if (b_tmp != 0 && b_tmp != 7 && b_tmp != 11 && b_tmp != 12) {
          saveSettings();
        }
        
        if (b_tmp >= 0 && b_tmp <= 12) {
          // Для команд, пришедших от MQTT отправлять только ACK;
          // Для команд, пришедших от UDP отправлять при необходимости другие данные, например - состояние элементов управления на странице от которой пришла команда 
          if (cmdSource == UDP) {
            if (b_tmp == 0) 
              sendPageParams(3, cmdSource);
            else if (b_tmp == 6) 
              sendPageParams(5, cmdSource);
            else if (b_tmp == 7) 
              sendStringData(str, cmdSource);
            else if (b_tmp == 11) 
              sendStringData(str, cmdSource);
            else if (b_tmp == 12) 
              sendStringData(str, cmdSource);
            else
              sendAcknowledge(cmdSource);
          } else {
            if (b_tmp == 7) 
              // 7 - запрос значений параметров - отправить их на MQTT-сервер
              sendStringData(str, cmdSource);
            else if (b_tmp == 11) 
              sendStringData(str, cmdSource);
            else if (b_tmp == 12) 
              sendStringData(str, cmdSource);
            else
              // Другие команды - отправить подтверждение о выполнении
              sendAcknowledge(cmdSource);
          }
        } else {
          #if (USE_MQTT == 1)
          notifyUnknownCommand(incomeBuffer);
          #endif
        }        
        break;

      // ----------------------------------------------------
      // 8 - эффект
      //  - $8 0 N; включить эффект N
      //  - $8 1 N D; D -> параметр #1 для эффекта N;
      //  - $8 2 N X; вкл/выкл использовать в демо-режиме; N - номер эффекта, X=0 - не использовать X=1 - использовать 
      //  - $8 3 N D; D -> параметр #2 для эффекта N;
      //  - $8 4 N X; вкл/выкл оверлей текста поверх эффекта; N - номер эффекта, X=0 - выкл X=1 - вкл 
      //  - $8 5 N X; вкл/выкл оверлей часов поверх эффекта; N - номер эффекта, X=0 - выкл X=1 - вкл 
      //  - $8 6 N D; D -> контрастность эффекта N;
      // ----------------------------------------------------

      case 8:      
        tmp_eff = intData[2];
        // intData[1] : дейстие -> 0 - выбор эффекта;  1 - параметры #1 и #2; 2 - вкл/выкл "использовать в демо-режиме"
        // intData[2] : номер эффекта
        // intData[3] : действие = 1: значение параметра #1 или #2
        //              действие = 2: 0 - выкл; 1 - вкл;
        if (intData[1] == 0) {    
          // Включить эффект      
          /*
          // Если в приложении выбраны часы, но они недоступны из-за размеров матрицы - брать другой случайный эффект
          if (tmp_eff == MC_CLOCK){
             if (!(allowHorizontal || allowVertical)) {
               setRandomMode2();
             }
          } 
          */         
          // Если в приложении выбраны эффект "Ночные часы" / "Дневные часы", но они недоступны из-за размеров матрицы - выключить матрицу
          // Если в приложении выбраны часы, но они недоступны из-за размеров матрицы - брать другой случайный эффект
          if (tmp_eff == MC_CLOCK){
            if (!(allowHorizontal || allowVertical)) {
              // setSpecialMode(0);  // Выключить
              setRandomMode2();
            } else {
              setSpecialMode(10);    // Дневные часы. Для ночных - 8
            }
          } else {
            setManualModeTo(true);
            loadingFlag = intData[1] == 0;
            setEffect(tmp_eff);
            if (tmp_eff == MC_FILL_COLOR && globalColor == 0x000000) globalColor = 0xffffff;
          }
        } else 
        
        if (intData[1] == 1) {          
          // Параметр #1 эффекта 
          if (tmp_eff == MC_CLOCK){
             // Параметр "Вариант" меняет цвет часов. 
             if (isNightClock) {
               // Для ночных часов - полученное значение -> в Map 0..6
               setNightClockColor(map(intData[3], 0,255, 0,6));
               nightClockColor = getNightClockColor();
               specialBrightness = nightClockBrightness < MIN_BRIGHT_FOR_NIGHT ? MIN_BRIGHT_FOR_NIGHT : nightClockBrightness;
               FastLED.setBrightness(specialBrightness);
             } else {
               // Для дневных часов - меняется цвет часов (параметр HUE цвета, hue < 2 - белый)
               setScaleForEffect (tmp_eff,   intData[3]);
               effectScaleParam  [tmp_eff] = intData[3];
             }
          } else {
            setScaleForEffect (tmp_eff,   intData[3]);
            effectScaleParam  [tmp_eff] = intData[3];
            if (tmp_eff == MC_FILL_COLOR) {  
              globalColor = getColorInt(CHSV(getEffectSpeed(MC_FILL_COLOR), effectScaleParam[MC_FILL_COLOR], 255));
              setGlobalColor(globalColor);
            } else 
            if (thisMode == tmp_eff && tmp_eff == MC_BALLS) {
              // При получении параметра эффекта "Шарики" (кол-во шариков) - надо переинициализировать эффект
              loadingFlag = true;
            }
          }
        } else 
        
        if (intData[1] == 2) {
          // Вкл/выкл использование эффекта в демо-режиме
          saveEffectUsage(tmp_eff, intData[3] == 1);           
        } else 
        
        if (intData[1] == 3) {
          // Параметр #2 эффекта
          setScaleForEffect2(tmp_eff,   intData[3]);
          effectScaleParam2 [tmp_eff] = intData[3];
          if (thisMode == tmp_eff && tmp_eff == MC_RAINBOW) {
            // При получении параметра 2 эффекта "Радуга" (тип радуги) - надо переинициализировать эффект
            // Если установлен тип радуги - "случайный" - продолжаем показывать тот что был
            loadingFlag = effectScaleParam2[tmp_eff] != 0;
          } else
          if (thisMode == tmp_eff && tmp_eff == MC_ARROWS) {
            // При получении параметра 2 эффекта "Стрелки" -  вид - надо переинициализировать эффект
            // Если установлен тип - "случайный" - продолжаем показывать тот что был
            loadingFlag = effectScaleParam2[tmp_eff] != 0;
          } else
          if (thisMode == tmp_eff && tmp_eff == MC_PAINTBALL) {
            // При получении параметра 2 эффекта "Пэйнтбол" (сегменты) - надо переинициализировать эффект
            loadingFlag = true;
          } else
          if (thisMode == tmp_eff && tmp_eff == MC_SWIRL) {
            // При получении параметра 2 эффекта "Водоворот" (сегменты) - надо переинициализировать эффект
            loadingFlag = true;
          } else
          if (thisMode == tmp_eff && tmp_eff == MC_CYCLON) {
            // При получении параметра 2 эффекта "Циклон" (сегменты) - надо переинициализировать эффект
            loadingFlag = true;
          } else
          if (thisMode == tmp_eff && tmp_eff == MC_PATTERNS) {
            // При получении параметра 2 эффекта "Узоры" -  вид - надо переинициализировать эффект
            // Если установлен узор - "случайный" - продолжаем показывать тот что был
            loadingFlag = effectScaleParam2[tmp_eff] != 0;
          } else
          if (thisMode == tmp_eff && tmp_eff == MC_SDCARD) {
            // При получении параметра 2 эффекта "SD-карта" -  вид - надо переинициализировать эффект
            // Если установлен эффект - "случайный" - продолжаем показывать тот что был
            loadingFlag = effectScaleParam2[tmp_eff] != 0;
          }          
        } else

        if (intData[1] == 4) {
          // Вкл/выкл оверлей бегущей строки поверх эффекта
          saveEffectTextOverlayUsage(tmp_eff, intData[3] == 1);           
        } else 

        if (intData[1] == 5) {
          // Вкл/выкл оверлей часов поверх эффекта
          saveEffectClockOverlayUsage(tmp_eff, intData[3] == 1);           
        } else

        if (intData[1] == 6) {
          // Контрастность эффекта
          setEffectContrast(tmp_eff, intData[3]);
        } 

        // Для "0","2","4","5","6" - отправляются параметры, подтверждение отправлять не нужно. Для остальных - нужно
        if (intData[1] >= 0 && intData[1] <= 6) {
          // Для команд, пришедших от MQTT отправлять только ACK;
          // Для команд, пришедших от UDP отправлять при необходимости другие данные, например - состояние элементов управления на странице от которой пришла команда 
          if (cmdSource == UDP) {
            if (intData[1] != 1) {
              sendPageParams(2, cmdSource);
            } else { 
              sendAcknowledge(cmdSource);
            }
          } else {
            sendAcknowledge(cmdSource);
          }
        } else {
          #if (USE_MQTT == 1)
          notifyUnknownCommand(incomeBuffer);
          #endif
        }
        break;

      // ----------------------------------------------------
      // 11 - Настройки MQTT-канала
      // - $11 1 X;   - использовать управление через MQTT сервер X; 0 - не использовать; 1 - использовать
      // - $11 2 D;   - Порт MQTT
      // - $11 3 X;   - Флаг - использует ли MQTT сервер префикс - имя пользователя при формировании топика
      // - $11 4 D;   - Задержка между последовательными обращениями к MQTT серверу
      // - $11 5;     - Разорвать подключение к MQTT серверу, чтобы он иог переподключиться с новыми параметрами
      // ----------------------------------------------------

      #if (USE_MQTT == 1)
      case 11:
         switch (intData[1]) {
           case 1:               // $11 1 X; - Использовать отображение температуры цветом 0 - нет; 1 - да
             useMQTT = intData[2] == 1;
             setUseMqtt(useMQTT);
             break;
           case 2:               // $11 2 D; - Порт MQTT
             mqtt_port = intData[2];
             setMqttPort(mqtt_port);
             break;
           case 3:               // $11 3 X; - Флаг - использует ли MQTT сервер префикс - имя пользователя при формировании топика
             mqtt_use_prefix = intData[2] == 1;
             setMqttUsePrefix(mqtt_use_prefix);
             break;
           case 4:               // $11 4 D; - Задержка между последовательными обращениями к MQTT серверу
             mqtt_send_delay = intData[2];
             setMqttSendDelay(mqtt_send_delay);
             break;
           case 5:               // $11 5;   - Сохранить изменения ипереподключиться к MQTT серверу
             saveSettings();
             mqtt.disconnect();
             break;
          default:
            err = true;
            notifyUnknownCommand(incomeBuffer);
            break;
        }
        if (!err) {
          // Для команд, пришедших от MQTT отправлять только ACK;
          // Для команд, пришедших от UDP отправлять при необходимости другие данные, например - состояние элементов управления на странице от которой пришла команда 
          sendAcknowledge(cmdSource);
        }
        break;
      #endif
      
      // ----------------------------------------------------
      // 12 - Настройки погоды
      // - $12 3 X;      - использовать цвет для отображения температуры X=0 - выкл X=1 - вкл в дневных часах
      // - $12 4 X;      - использовать получение погоды с погодного сервера
      // - $12 5 I С C2; - интервал получения погоды с сервера в минутах (I) и код региона C - Yandex и код региона C2 - OpenWeatherMap
      // - $12 6 X;      - использовать цвет для отображения температуры X=0 - выкл X=1 - вкл в ночных часах
      // ----------------------------------------------------

      #if (USE_WEATHER == 1)                  
      case 12:
         switch (intData[1]) {
           case 3:               // $12 3 X; - Использовать отображение температуры цветом 0 - нет; 1 - да
             useTemperatureColor = intData[2] == 1;
             setUseTemperatureColor(useTemperatureColor);
             break;
           case 4:               // $12 4 X; - Использовать получение погоды с сервера 0 - нет; 1 - Yandex 2 - OpenWeatherMap
             useWeather = intData[2];
             setUseWeather(useWeather);      // При сохранении - проверка на корректность
             useWeather = getUseWeather();   // Считываем корректные данные
             if (wifi_connected) {
               refresh_weather = true; weather_t = 0; weather_cnt = 0;
             }
             break;
           case 5:               // $12 5 I C C2; - Интервал обновления погоды с сервера в минутах, Код региона Yandex, Код региона OpenWeatherMap 
             SYNC_WEATHER_PERIOD = intData[2];
             regionID = intData[3];
             regionID2 = intData[4];
             setWeatherInterval(SYNC_WEATHER_PERIOD);
             setWeatherRegion(regionID);
             setWeatherRegion2(regionID2);
             weatherTimer.setInterval(1000L * 60 * SYNC_WEATHER_PERIOD);
             if (wifi_connected) {
               refresh_weather = true; weather_t = 0; weather_cnt = 0;
             }
             break;
           case 6:               // $12 6 X; - Использовать отображение температуры цветом 0 - нет; 1 - да
             useTemperatureColorNight = intData[2] == 1;
             setUseTemperatureColorNight(useTemperatureColorNight);
             break;
          default:
            err = true;
            #if (USE_MQTT == 1)
            notifyUnknownCommand(incomeBuffer);
            #endif
            break;
        }
        if (!err) {
          // Для команд, пришедших от MQTT отправлять только ACK;
          // Для команд, пришедших от UDP отправлять при необходимости другие данные, например - состояние элементов управления на странице от которой пришла команда 
          if (cmdSource == UDP) {
            sendPageParams(1, cmdSource);
          } else {
            sendAcknowledge(cmdSource);
          }
        }
        break;
      #endif

      // ----------------------------------------------------
      // 13 - Настройки бегущей cтроки  
      // - $13 0 N;   - активация для редактирования строки с номером N - запрос текста строки N - 0..35 
      // - $13 1 N;   - активация прокручивания строки с номером N - 0..35
      // - $13 2 I;   - запросить текст бегущей строки с индексом I как есть
      // - $13 3 I;   - запросить текст бегущей строки с индексом I после обработки макросов
      // - $13 9 I;   - сохранить настройку I - интервал в секундах отображения бегущей строки
      // - $13 11 X;  - Режим цвета бегущей строки X: 0,1,2,           
      // - $13 13 X;  - скорость прокрутки бегущей строки
      // - $13 15 00FFAA; - цвет текстовой строки для режима "монохромный", сохраняемый в globalTextColor
      // - $13 18 X;  - сохранить настройку X "Бегущая строка в эффектах" (общий, для всех эффектов)
      // ----------------------------------------------------

      case 13:
         switch (intData[1]) {
           case 0:               // $13 0 N; установить строку N (0..35) как редактируемую в приложении, отправить, содержимое
             // Вх. X - 0..35, преобразовать в char для editIdx '0'..'9','A'..'Z'
             editIdx = getAZIndex(intData[2]);
             break;
           case 1:               // $13 1 N; установить строку N = 0..35 как активную отображаемую строку          
             // Если строка 0 - управляющая - ее нельзя включать принудительно.
             b_tmp = intData[2] > 0 || (intData[2] == 0 && textLines[0].charAt(0) != '#');
             if (b_tmp) {              
               if (thisMode == MC_TEXT){
                  setRandomMode2();
               } 
               nextTextLineIdx = intData[2];  // nextTextLineIdx - индекс следующей принудительно отображаемой строки
               ignoreTextOverlaySettingforEffect = true;
               fullTextFlag = true;
               textLastTime = 0;
             }
             break;
           case 2:               // $13 2 I; - Запросить исходный текст бегущей строки с индексом I без обработки макросов
             if (intData[2] >= 0 && (intData[2]<(sizeof(textLines) / sizeof(String)))) {
               editIdx = getAZIndex(intData[2]);
               sendPageParams(91, cmdSource);
             }
             break;
           case 3:               // $13 3 I; - Запросить текст бегущей строки с индексом I с обработкой макросов
             if (intData[2] >= 0 && (intData[2]<(sizeof(textLines) / sizeof(String)))) {
               sendTextIdx = intData[2];
               sendPageParams(92, cmdSource);
             }
             break;
           case 9:               // $13 9 I; - Периодичность отображения бегущей строки (в секундах)
             TEXT_INTERVAL = intData[2];
             setTextInterval(TEXT_INTERVAL);
             break;
           case 11:               // $13 11 X; - Режим цвета бегущей строкой X: 0,1,2,           
             COLOR_TEXT_MODE = intData[2];
             if (COLOR_TEXT_MODE > 2) COLOR_TEXT_MODE = 0;
             setTextColor(COLOR_TEXT_MODE);
             break;
           case 13:               // $13 13 X; - скорость прокрутки бегущей строки
             setTextScrollSpeed(255 - intData[2]);
             setTimersForMode(thisMode);
             break;
           case 15:               // $13 15 00FFAA;
             // В строке цвет - "$13 15 00FFAA;" - цвет часов текстовой строкой для режима "монохромный", сохраняемый в globalTextColor
             str = String(incomeBuffer).substring(7,13);
             globalTextColor = (uint32_t)HEXtoInt(str);
             setGlobalTextColor(globalTextColor);
             break;
           case 18:               // $13 18 X; - сохранить настройку X "Бегущая строка в эффектах"
             textOverlayEnabled = intData[2] == 1;
             saveTextOverlayEnabled(textOverlayEnabled);
             if (!textOverlayEnabled) {
               showTextNow = false;
               ignoreTextOverlaySettingforEffect = false;
             }
             break;
          default:
            err = true;
            #if (USE_MQTT == 1)
            notifyUnknownCommand(incomeBuffer);
            #endif
            break;
        }
        if (!err) {
          // Для команд, пришедших от MQTT отправлять только ACK;
          // Для команд, пришедших от UDP отправлять при необходимости другие данные, например - состояние элементов управления на странице от которой пришла команда 
          if (cmdSource == UDP) {
            if (intData[1] != 2 && intData[1] != 3)
              sendPageParams(3, cmdSource);
            else  
              // Для команды "$13 3" отправляется значение, подтверждения отправлять не нужно
              sendAcknowledge(cmdSource);
          } else {
            // Для команды "$13 3" отправляется значение, подтверждения отправлять не нужно 
            if (intData[1] != 2 && intData[1] != 3) {
              sendAcknowledge(cmdSource);
            }
          }
        }
        break;
        
      // ----------------------------------------------------
      //  14 - быстрая установка ручных режимов с пред-настройками
      // - $14 0;  Черный экран (выкл);  
      // - $14 1;  Белый экран (освещение);  
      // - $14 2;  Цветной экран;  
      // - $14 3;  Огонь;  
      // - $14 4;  Конфетти;  
      // - $14 5;  Радуга;  
      // - $14 6;  Матрица;  
      // - $14 7;  Светлячки;  
      // - $14 8;  Часы ночные;
      // - $14 9;  Часы бегущей строкой;
      // - $14 10; Часы простые;  
      // ----------------------------------------------------

      case 14:
        if (intData[1] < 0 || intData[1] >= MAX_SPEC_EFFECT) {
          #if (USE_MQTT == 1)
          notifyUnknownCommand(incomeBuffer);
          #endif
        } else {
          if (intData[1] == 2) {
             // Если в строке цвет - "$14 2 00FFAA;" - цвет лампы, сохраняемый в globalColor
             str = String(incomeBuffer).substring(6,12); // $14 2 00FFAA;
             globalColor = (uint32_t)HEXtoInt(str);
             setGlobalColor(globalColor);
          }        
          setSpecialMode(intData[1]);
          // Для команд, пришедших от MQTT отправлять только ACK;
          // Для команд, пришедших от UDP отправлять при необходимости другие данные, например - состояние элементов управления на странице от которой пришла команда 
          if (cmdSource == UDP) {
            sendPageParams(1, cmdSource);
          } else {
            sendAcknowledge(cmdSource);
          }
        }
        break;
      
      // ----------------------------------------------------
      // 15 - скорость $15 скорость таймер; 0 - таймер эффектов
      // ----------------------------------------------------

      case 15: 
        if (intData[2] == 0) {
          if (intData[1] == 255) intData[1] = 254;
          effectSpeed = 255 - intData[1]; 
          saveEffectSpeed(thisMode, effectSpeed);
          if (thisMode == MC_FILL_COLOR) { 
            globalColor = getColorInt(CHSV(effectSpeed, effectScaleParam[MC_FILL_COLOR], 255));
            setGlobalColor(globalColor);
          }
          setTimersForMode(thisMode);           
          // Для команд, пришедших от MQTT отправлять только ACK;
          // Для команд, пришедших от UDP отправлять при необходимости другие данные, например - состояние элементов управления на странице от которой пришла команда 
          sendAcknowledge(cmdSource);
        } else {
          #if (USE_MQTT == 1)
          notifyUnknownCommand(incomeBuffer);
          #endif
        }
        break;

      // ----------------------------------------------------
      // 16 - Режим смены эффектов: $16 value; N: 0 - ручной режим;  1 - авторежим; 2 - PrevMode; 3 - NextMode; 5 - вкл/выкл случайный выбор следующего режима
      // ----------------------------------------------------

      case 16:
        if      (intData[1] == 0) setManualModeTo(true);
        else if (intData[1] == 1) { resetModes(); setManualModeTo(false); }
        else if (intData[1] == 2) prevMode();
        else if (intData[1] == 3) nextMode();
        else if (intData[1] == 5) useRandomSequence = intData[2] == 1;

        saveRandomMode(useRandomSequence);        
        setCurrentManualMode(manualMode ? (int8_t)thisMode : -1);
        
        if (manualMode) {
          setCurrentSpecMode(-1);
        }

        if (intData[1] == 1 && thisMode == MC_FILL_COLOR && globalColor == 0x000000) {
          // Было выключено, режим "Лампа" с черным цветом - включить случайный режим
          setRandomMode2();
        }
        
        // Для команд, пришедших от MQTT отправлять только ACK;
        // Для команд, пришедших от UDP отправлять при необходимости другие данные, например - состояние элементов управления на странице от которой пришла команда 
        if (cmdSource == UDP) {
          sendPageParams(1, cmdSource);
        } else {
          sendAcknowledge(cmdSource);
        }
        break;

      // ----------------------------------------------------
      // 17 - Время автосмены эффектов и бездействия: $17 сек
      // ----------------------------------------------------

      case 17: 
        autoplayTime = ((long)intData[1] * 1000L);   // секунды -> миллисек 
        idleTime = ((long)intData[2] * 60 * 1000L);  // минуты -> миллисек
        saveAutoplayTime(autoplayTime);
        saveIdleTime(idleTime);
        idleState = !manualMode;
        if (!manualMode) {
          autoplayTimer = millis();
        }
        if (idleTime == 0 || manualMode) // таймер отключен
          idleTimer.setInterval(4294967295);
        else
          idleTimer.setInterval(idleTime);
        idleTimer.reset();
        // Для команд, пришедших от MQTT отправлять только ACK;
        // Для команд, пришедших от UDP отправлять при необходимости другие данные, например - состояние элементов управления на странице от которой пришла команда 
        sendAcknowledge(cmdSource);
        break;

      // ----------------------------------------------------
      // 18 - Запрос текущих параметров программой: $18 page;
      //    page 1:   // Настройки
      //    page 2:   // Эффекты
      //    page 3:   // Настройки бегущей строки
      //    page 4:   // Настройки часов
      //    page 5:   // Настройки будильника
      //    page 6:   // Настройки подключения
      //    page 7:   // Настройки режимов автовключения по времени
      //    page 10:  // Загрузка картинок
      //    page 11:  // Рисование
      //    page 12:  // Игры
      //    page 91:  // Запрос текста бегущих строк как есть без обработки макросов
      //    page 92:  // Запрос текста бегущих строк с обработкой макросов для заполнения списка в программе
      //    page 93:  // Запрос списка звуков будильника
      //    page 94:  // Запрос списка звуков рассвета
      //    page 95:  // Ответ состояния будильника - сообщение по инициативе сервера
      //    page 96:  // Ответ демо-режима звука - сообщение по инициативе сервера
      //    page 99:  // Запрос списка эффектов
      // ----------------------------------------------------

      case 18: 
        if (intData[1] == 0) { // ping
          sendAcknowledge(cmdSource);
        } else {                          // запрос параметров страницы приложения
          // Для команд, пришедших от MQTT отправлять ответы так же как и для команд, полученных из UDP
          sendPageParams(intData[1], cmdSource);
        }
        break;

      // ----------------------------------------------------
      // 19 - работа с настройками часов
      //   $19 1 X; - сохранить настройку X "Часы в эффектах" (общий, для всех эффектов)
      //   $19 2 X; - Использовать синхронизацию часов NTP  X: 0 - нет, 1 - да
      //   $19 3 N Z; - Период синхронизации часов NTP и Часовой пояс
      //   $19 4 X; - Выключать индикатор TM1637 при выключении экрана X: 0 - нет, 1 - да
      //   $19 5 X; - Режим цвета часов оверлея X: 0,1,2,3
      //   $19 6 X; - Ориентация часов  X: 0 - горизонтально, 1 - вертикально
      //   $19 7 X; - Размер часов X: 0 - авто, 1 - малые 3х5, 2 - большие 5x7
      //   $19 8 YYYY MM DD HH MM; - Установить текущее время YYYY.MM.DD HH:MM
      //   $19 9 X; - Показывать температуру вместе с малыми часами 1 - да; 0 - нет
      //   $19 10 X; - Цвет ночных часов:  0 - R; 1 - G; 2 - B; 3 - C; 3 - M; 5 - Y; 6 - W;
      //   $19 11 X; - Яркость ночных часов:  0..255
      //   $19 12 X; - скорость прокрутки часов оверлея или 0, если часы остановлены по центру
      //   $19 14 00FFAA; - цвет часов оверлея, сохраняемый в globalClockColor
      //   $19 16 X; - Показывать дату в режиме часов  X: 0 - нет, 1 - да
      //   $19 17 D I; - Продолжительность отображения даты / часов (в секундах)
      // ----------------------------------------------------
      
      case 19: 
         switch (intData[1]) {
           case 1:               // $19 1 X; - сохранить настройку X "Часы в эффектах"
             clockOverlayEnabled = ((CLOCK_ORIENT == 0 && allowHorizontal) || (CLOCK_ORIENT == 1 && allowVertical)) ? intData[2] == 1 : false;
             saveClockOverlayEnabled(clockOverlayEnabled);
             if (specialMode) specialClock = clockOverlayEnabled;
             break;
           case 2:               // $19 2 X; - Использовать синхронизацию часов NTP  X: 0 - нет, 1 - да
             useNtp = intData[2] == 1;
             saveUseNtp(useNtp);
             if (wifi_connected) {
               refresh_time = true; ntp_t = 0; ntp_cnt = 0;
             }
             break;
           case 3:               // $19 3 N Z; - Период синхронизации часов NTP и Часовой пояс
             SYNC_TIME_PERIOD = intData[2];
             timeZoneOffset = (int8_t)intData[3];
             saveTimeZone(timeZoneOffset);
             saveNtpSyncTime(SYNC_TIME_PERIOD);
             ntpSyncTimer.setInterval(1000L * 60 * SYNC_TIME_PERIOD);
             if (wifi_connected) {
               refresh_time = true; ntp_t = 0; ntp_cnt = 0;
             }
             break;
           case 4:               // $19 4 X; - Выключать индикатор TM1637 при выключении экрана X: 0 - нет, 1 - да
             needTurnOffClock = intData[2] == 1;
             setTurnOffClockOnLampOff(needTurnOffClock);
             break;
           case 5:               // $19 5 X; - Режим цвета часов оверлея X: 0,1,2,3
             COLOR_MODE = intData[2];
             if (COLOR_MODE > 3) COLOR_MODE = 0;
             setClockColor(COLOR_MODE);
             break;
           case 6:               // $19 6 X; - Ориентация часов  X: 0 - горизонтально, 1 - вертикально
             CLOCK_ORIENT = intData[2] == 1 ? 1  : 0;             
             if (allowHorizontal || allowVertical) {
               if (CLOCK_ORIENT == 0 && !allowHorizontal) CLOCK_ORIENT = 1;
               if (CLOCK_ORIENT == 1 && !allowVertical) CLOCK_ORIENT = 0;              
             } else {
               clockOverlayEnabled = false;
               saveClockOverlayEnabled(clockOverlayEnabled);
             }
             // Центрируем часы по горизонтали/вертикали по ширине / высоте матрицы
             checkClockOrigin();
             saveClockOrientation(CLOCK_ORIENT);
             break;
           case 7:               // $19 7 X; - Размер часов  X: 0 - авто, 1 - малые 3х5, 2 - большие 5х7
             CLOCK_SIZE = intData[2];             
             checkClockOrigin();
             saveClockSize(CLOCK_SIZE);
             break;
           case 8:               // $19 8 YYYY MM DD HH MM; - Установить текущее время YYYY.MM.DD HH:MM
             setTime(intData[5],intData[6],0,intData[4],intData[3],intData[2]);
             init_time = true; refresh_time = false; ntp_cnt = 0;
             rescanTextEvents();
             break;
           case 9:               // $19 9 X; - Показывать температуру в режиме часов  X: 0 - нет, 1 - да
             if (allowHorizontal || allowVertical) {
               showWeatherInClock = intData[2] == 1;
             } else {
               showWeatherInClock = false;
             }
             setShowWeatherInClock(showWeatherInClock);
             break;
           case 10:               // $19 10 X; - Цвет ночных часов:  0 - R; 1 - G; 2 - B; 3 - C; 3 - M; 5 - Y; 6 - W;
             setNightClockColor(intData[2]);
             nightClockColor = getNightClockColor();
             if (isNightClock) {
                specialBrightness = nightClockBrightness < MIN_BRIGHT_FOR_NIGHT ? MIN_BRIGHT_FOR_NIGHT : nightClockBrightness;
                FastLED.setBrightness(specialBrightness);
             }             
             break;
           case 11:               // $19 11 X; - Яркость ночных часов:  1..255;
             setNightClockBrightness(intData[2]);
             nightClockBrightness = getNightClockBrightness();
             if (isNightClock) {
                specialBrightness = nightClockBrightness < MIN_BRIGHT_FOR_NIGHT ? MIN_BRIGHT_FOR_NIGHT : nightClockBrightness;
                FastLED.setBrightness(specialBrightness);
             }             
             break;
           case 12:               // $19 12 X; - скорость прокрутки часов оверлея или 0, если часы остановлены по центру
             setClockScrollSpeed(255 - intData[2]);
             setTimersForMode(thisMode);
             break;
           case 14:               // $19 14 00FFAA;
             // В строке цвет - "$19 14 00FFAA;" - цвет часов оверлея, сохраняемый в globalClockColor
             str = String(incomeBuffer).substring(7,13);
             globalClockColor = (uint32_t)HEXtoInt(str);
             setGlobalClockColor(globalClockColor);
             break;
           case 16:               // $19 16 X; - Показывать дату в режиме часов  X: 0 - нет, 1 - да
             if (allowHorizontal || allowVertical) {
               showDateInClock = intData[2] == 1;
             } else {
               clockOverlayEnabled = false;
               showDateInClock = false;
               saveClockOverlayEnabled(clockOverlayEnabled);
             }
             setShowDateInClock(showDateInClock);
             break;
           case 17:               // $19 17 D I; - Продолжительность отображения даты / часов (в секундах)
             showDateDuration = intData[2];
             showDateInterval = intData[3];
             setShowDateDuration(showDateDuration);
             setShowDateInterval(showDateInterval);
             break;
          default:
            err = true;
            #if (USE_MQTT == 1)
            notifyUnknownCommand(incomeBuffer);
            #endif
            break;
        }
        if (!err) {
          // Для команд, пришедших от MQTT отправлять только ACK;
          // Для команд, пришедших от UDP отправлять при необходимости другие данные, например - состояние элементов управления на странице от которой пришла команда 
          if (cmdSource == UDP) {
            if (intData[1] != 8) {
              sendPageParams(4, cmdSource);
            } else {
              sendAcknowledge(cmdSource);
            }
          } else {
            sendAcknowledge(cmdSource);
          }
        }
        break;

      // ----------------------------------------------------
      //  20 - настройки и управление будильников
      // - $20 0;       - отключение будильника (сброс состояния isAlarming)
      // - $20 2 X VV MA MB;
      //      X    - исп звук будильника X=0 - нет, X=1 - да 
      //     VV    - максимальная громкость
      //     MA    - номер файла звука будильника
      //     MB    - номер файла звука рассвета
      // - $20 3 X NN VV; - пример звука будильника
      //      X - 1 играть 0 - остановить
      //     NN - номер файла звука будильника из папки SD:/01
      //     VV - уровень громкости
      // - $20 4 X NN VV; - пример звука рассвета
      //      X - 1 играть 0 - остановить
      //     NN - номер файла звука рассвета из папки SD:/02
      //     VV - уровень громкости
      // - $20 5 VV; - установит уровень громкости проигрывания примеров (когда уже играет)
      //     VV - уровень громкости
      // ----------------------------------------------------
      
      case 20:
        switch (intData[1]) { 
          case 0:  
            // $20 0;       - отключение будильника (сброс состояния isAlarming)
            if (isAlarming || isPlayAlarmSound) stopAlarm();            
            break;
          case 2:
            #if (USE_MP3 == 1)          
            if (isDfPlayerOk) {
              // $20 2 X VV MA MB;
              //    X    - исп звук будильника X=0 - нет, X=1 - да 
              //   VV    - максимальная громкость
              //   MA    - номер файла звука будильника
              //   MB    - номер файла звука рассвета
              dfPlayer.stop();
              soundFolder = 0;
              soundFile = 0;
              isAlarming = false;
              isAlarmStopped = false;

              useAlarmSound = intData[2] == 1;
              maxAlarmVolume = constrain(intData[3],0,30);
              alarmSound = intData[4] - 2;  // Индекс от приложения: 0 - нет; 1 - случайно; 2 - 1-й файл; 3 - ... -> -1 - нет; 0 - случайно; 1 - 1-й файл и т.д
              dawnSound = intData[5] - 2;   // Индекс от приложения: 0 - нет; 1 - случайно; 2 - 1-й файл; 3 - ... -> -1 - нет; 0 - случайно; 1 - 1-й файл и т.д
              saveAlarmSounds(useAlarmSound, maxAlarmVolume, alarmSound, dawnSound);
            }
            #endif
            break;
          case 3:
            #if (USE_MP3 == 1)
            if (isDfPlayerOk) {
              // $20 3 X NN VV; - пример звука будильника
              //  X  - 1 играть 0 - остановить
              //  NN - номер файла звука будильника из папки SD:/01
              //  VV - уровень громкости
              if (intData[2] == 0) {
                StopSound(0);
                soundFolder = 0;
                soundFile = 0;
              } else {
                b_tmp = intData[3] - 2;  // Знач: -1 - нет; 0 - случайно; 1 и далее - файлы; -> В списке индексы: 1 - нет; 2 - случайно; 3 и далее - файлы
                if (b_tmp > 0 && b_tmp <= alarmSoundsCount) {
                  dfPlayer.stop();
                  soundFolder = 1;
                  soundFile = b_tmp;
                  dfPlayer.volume(constrain(intData[4],0,30));
                  dfPlayer.playFolder(soundFolder, soundFile);
                  dfPlayer.enableLoop();
                } else {
                  soundFolder = 0;
                  soundFile = 0;
                }
              }
            }  
            #endif
            break;
          case 4:
            #if (USE_MP3 == 1)
            if (isDfPlayerOk) {
              // $20 4 X NN VV; - пример звука рассвета
              //    X  - 1 играть 0 - остановить
              //    NN - номер файла звука рассвета из папки SD:/02
              //    VV - уровень громкости
              if (intData[2] == 0) {
                StopSound(0);
                soundFolder = 0;
                soundFile = 0;
              } else {
                dfPlayer.stop();
                b_tmp = intData[3] - 2; // Знач: -1 - нет; 0 - случайно; 1 и далее - файлы; -> В списке индексы: 1 - нет; 2 - случайно; 3 и далее - файлы
                if (b_tmp > 0 && b_tmp <= dawnSoundsCount) {
                  soundFolder = 2;
                  soundFile = b_tmp;
                  dfPlayer.volume(constrain(intData[4],0,30));
                  dfPlayer.playFolder(soundFolder, soundFile);
                  dfPlayer.enableLoop();
                } else {
                  soundFolder = 0;
                  soundFile = 0;
                }
              }
            }
            #endif
            break;
          case 5:
            #if (USE_MP3 == 1)
            if (isDfPlayerOk && soundFolder > 0) {
             // $20 5 VV; - установить уровень громкости проигрывания примеров (когда уже играет)
             //    VV - уровень громкости
             maxAlarmVolume = constrain(intData[2],0,30);
             dfPlayer.volume(maxAlarmVolume);
            }
            #endif
            break;
          default:
            err = true;
            #if (USE_MQTT == 1)
            notifyUnknownCommand(incomeBuffer);
            #endif
            break;
        }
        if (!err) {
          // Для команд, пришедших от MQTT отправлять только ACK;
          // Для команд, пришедших от UDP отправлять при необходимости другие данные, например - состояние элементов управления на странице от которой пришла команда 
          if (cmdSource == UDP) {
            if (intData[1] == 0) {
              sendPageParams(5, cmdSource);
            } else if (intData[1] == 1 || intData[1] == 2) { // Режимы установки параметров - сохранить
              // saveSettings();
              sendPageParams(5, cmdSource);
            } else {
              sendPageParams(96);
            }        
          } else {
            sendAcknowledge(cmdSource);
          }          
        }
        break;

      // ----------------------------------------------------
      // 21 - настройки подключения к сети / точке доступа
      //   $21 0 0 - не использовать точку доступа $21 0 1 - использовать точку доступа
      //   $21 1 IP1 IP2 IP3 IP4 - установить статический IP адрес подключения к локальной WiFi сети, пример: $21 1 192 168 0 106
      //   $21 2; Выполнить переподключение к сети WiFi
      // ----------------------------------------------------

      case 21:
        // Настройки подключения к сети
        switch (intData[1]) { 
          // $21 0 0 - не использовать точку доступа $21 0 1 - использовать точку доступа
          case 0:  
            useSoftAP = intData[2] == 1;
            setUseSoftAP(useSoftAP);
            if (useSoftAP && !ap_connected) 
              startSoftAP();
            else if (!useSoftAP && ap_connected) {
              if (wifi_connected) { 
                ap_connected = false;              
                WiFi.softAPdisconnect(true);
                Serial.println(F("Точка доступа отключена."));
              }
            }      
            break;
          case 1:  
            // $21 1 IP1 IP2 IP3 IP4 - установить статический IP адрес подключения к локальной WiFi сети, пример: $21 1 192 168 0 106
            // Локальная сеть - 10.х.х.х или 172.16.х.х - 172.31.х.х или 192.168.х.х
            // Если задан адрес не локальной сети - сбросить его в 0.0.0.0, что означает получение динамического адреса 
            if (!(intData[2] == 10 || (intData[2] == 172 && intData[3] >= 16 && intData[3] <= 31) || (intData[2] == 192 && intData[3] == 168))) {
              intData[2] = 0;
              intData[3] = 0;
              intData[4] = 0;
              intData[5] = 0;
            }
            saveStaticIP(intData[2], intData[3], intData[4], intData[5]);
            break;
          case 2:  
            // $21 2; Выполнить переподключение к сети WiFi
            saveSettings();
            delay(10);
            FastLED.clear();
            startWiFi(5000);     // Время ожидания подключения 5 сек
            showCurrentIP(true);
            break;
          default:
            err = true;
            #if (USE_MQTT == 1)
            notifyUnknownCommand(incomeBuffer);
            #endif
            break;
        }
        if (!err) {
          // Для команд, пришедших от MQTT отправлять только ACK;
          // Для команд, пришедших от UDP отправлять при необходимости другие данные, например - состояние элементов управления на странице от которой пришла команда 
          if (cmdSource == UDP) {
            if (intData[1] == 0 || intData[1] == 1) {
              sendAcknowledge(cmdSource);
            } else {
              sendPageParams(6, cmdSource);
            }
          } else {
            sendAcknowledge(cmdSource);
          }
        }
        break;

      // ----------------------------------------------------
      // 22 - настройки включения режимов матрицы в указанное время
      // - $22 HH1 MM1 NN1 HH2 MM2 NN2 HH3 MM3 NN3 HH4 MM4 NN4
      //     HHn - час срабатывания
      //     MMn - минуты срабатывания
      //     NNn - эффект: -3 - выключено; -2 - выключить матрицу; -1 - ночные часы; 0 - случайный режим и далее по кругу; 1 и далее - список режимов EFFECT_LIST 
      // ----------------------------------------------------

      case 22:
        AM1_hour = intData[1];
        AM1_minute = intData[2];
        AM1_effect_id = intData[3];
        if (AM1_hour < 0) AM1_hour = 0;
        if (AM1_hour > 23) AM1_hour = 23;
        if (AM1_minute < 0) AM1_minute = 0;
        if (AM1_minute > 59) AM1_minute = 59;
        if (AM1_effect_id < -3) AM1_effect_id = -3;
        setAM1params(AM1_hour, AM1_minute, AM1_effect_id);

        AM2_hour = intData[4];
        AM2_minute = intData[5];
        AM2_effect_id = intData[6];
        if (AM2_hour < 0) AM2_hour = 0;
        if (AM2_hour > 23) AM2_hour = 23;
        if (AM2_minute < 0) AM2_minute = 0;
        if (AM2_minute > 59) AM2_minute = 59;
        if (AM2_effect_id < -3) AM2_effect_id = -3;
        setAM2params(AM2_hour, AM2_minute, AM2_effect_id);

        AM3_hour = intData[7];
        AM3_minute = intData[8];
        AM3_effect_id = intData[9];
        if (AM3_hour < 0) AM3_hour = 0;
        if (AM3_hour > 23) AM3_hour = 23;
        if (AM3_minute < 0) AM3_minute = 0;
        if (AM3_minute > 59) AM3_minute = 59;
        if (AM3_effect_id < -3) AM3_effect_id = -3;
        setAM3params(AM3_hour, AM3_minute, AM3_effect_id);

        AM4_hour = intData[10];
        AM4_minute = intData[11];
        AM4_effect_id = intData[12];
        if (AM4_hour < 0) AM4_hour = 0;
        if (AM4_hour > 23) AM4_hour = 23;
        if (AM4_minute < 0) AM4_minute = 0;
        if (AM4_minute > 59) AM4_minute = 59;
        if (AM4_effect_id < -3) AM4_effect_id = -3;
        setAM4params(AM4_hour, AM4_minute, AM4_effect_id);

        saveSettings();
        // Для команд, пришедших от MQTT отправлять только ACK;
        // Для команд, пришедших от UDP отправлять при необходимости другие данные, например - состояние элементов управления на странице от которой пришла команда 
        if (cmdSource == UDP) {
          sendPageParams(7, cmdSource);
        } else {
          sendAcknowledge(cmdSource);
        }
        break;

      // ----------------------------------------------------
      // 23 - прочие настройки
      // - $23 0 VAL  - лимит по потребляемому току
      // ----------------------------------------------------

      case 23:
        // $23 0 VAL - лимит по потребляемому току
        switch(intData[1]) {
          case 0:
            setPowerLimit(intData[2]);
            CURRENT_LIMIT = getPowerLimit();
            FastLED.setMaxPowerInVoltsAndMilliamps(5, CURRENT_LIMIT == 0 ? 100000 : CURRENT_LIMIT);            
            break;
          default:
            err = true;
            #if (USE_MQTT == 1)
            notifyUnknownCommand(incomeBuffer);
            #endif
            break;
        }
        if (!err) {
          // Для команд, пришедших от MQTT отправлять только ACK;
          // Для команд, пришедших от UDP отправлять при необходимости другие данные, например - состояние элементов управления на странице от которой пришла команда 
          sendAcknowledge(cmdSource);
        }
        break;

      // ----------------------------------------------------
      default:
        #if (USE_MQTT == 1)
        notifyUnknownCommand(incomeBuffer);
        #endif
        break;

    }
  }

  // ****************** ПАРСИНГ *****************

  // Если предыдущий буфер еще не разобран - новых данных из сокета не читаем, продолжаем разбор уже считанного буфера
  haveIncomeData = bufIdx > 0 && bufIdx < packetSize; 

  #if (USE_MQTT == 1)
  if (!haveIncomeData) {
    // Есть ли поступившие по каналу MQTT команды?
    if (queueLength > 0) {
      String command = cmdQueue[queueReadIdx++];
      if (queueReadIdx >= QSIZE_IN) queueReadIdx = 0;
      queueLength--;
      
      cmdSource = MQTT;
      haveIncomeData = true;
      bufIdx = 0;
      packetSize = command.length();
      memcpy(incomeBuffer, command.c_str(), packetSize);

      Serial.print(F("MQTT пакeт размером "));
      Serial.println(packetSize);
    }
  }
  #endif
  
  if (!haveIncomeData) {
    packetSize = udp.parsePacket();      
    haveIncomeData = packetSize > 0;      
  
    if (haveIncomeData) {                
      // read the packet into packetBufffer
      int len = udp.read(incomeBuffer, BUF_MAX_SIZE);
      if (len > 0) {          
        incomeBuffer[len] = 0;
      }
      bufIdx = 0;

      cmdSource = UDP;
      
      delay(0);            // ESP8266 при вызове delay отрабатывает стек IP протокола, дадим ему поработать        

      Serial.print(F("UDP << ip='"));
      IPAddress remote = udp.remoteIP();
      Serial.print(remote.toString());
      Serial.print(":");
      Serial.print(udp.remotePort());
      Serial.print("'");
      if (udp.remotePort() == localPort) {
        Serial.print(F("; cmd='"));
        Serial.print(incomeBuffer);
        Serial.print("'");
      }
      if (udp.remotePort() == 123) {
        Serial.print(F("; ntp sync"));
      }
      Serial.println();

      Serial.print(F("UDP пакeт размером "));
      Serial.println(packetSize);
    }

    // NTP packet from time server
    if (haveIncomeData && udp.remotePort() == 123) {
      parseNTP();
      haveIncomeData = false;
      bufIdx = 0;      
    }
  }

  if (haveIncomeData) {         

    // Из-за ошибки в компоненте UdpSender в Thunkable - теряются половина отправленных 
    // символов, если их кодировка - двухбайтовый UTF8, т.к. оно вычисляет длину строки без учета двухбайтовости
    // Чтобы символы не терялись - при отправке строки из андроид-программы, она добивается с конца пробелами
    // Здесь эти конечные пробелы нужно предварительно удалить
    while (packetSize > 0 && incomeBuffer[packetSize-1] == ' ') packetSize--;
    incomeBuffer[packetSize] = 0;

    if (parseMode == TEXT) {                         // если нужно принять строку - принимаем всю

      // Оставшийся буфер преобразуем с строку
      if (intData[0] == 6) {  // текст
        receiveText = String(&incomeBuffer[bufIdx]);
        receiveText.trim();
      }
                
      incomingByte = ending;                       // сразу завершаем парс
      parseMode = NORMAL;
      bufIdx = 0; 
      packetSize = 0;                              // все байты из входящего пакета обработаны
    } else {
      incomingByte = incomeBuffer[bufIdx++];       // обязательно ЧИТАЕМ входящий символ
    } 
  }       
    
  if (haveIncomeData) {

    if (parseStarted) {                                             // если приняли начальный символ (парсинг разрешён)
      if (incomingByte != divider && incomingByte != ending) {      // если это не пробел И не конец
        string_convert += incomingByte;                             // складываем в строку
      } else {                                                      // если это пробел или ; конец пакета
        if (parse_index == 0) {
          byte cmdMode = string_convert.toInt();
          intData[0] = cmdMode;
          if (cmdMode == 6) {
            parseMode = TEXT;
          }
          else parseMode = NORMAL;
        }

        if (parse_index == 1) {       // для второго (с нуля) символа в посылке
          if (parseMode == NORMAL) intData[parse_index] = string_convert.toInt();           // преобразуем строку в int и кладём в массив}
          if (parseMode == COLOR) {                                                         // преобразуем строку HEX в цифру
            globalColor = (uint32_t)HEXtoInt(string_convert);           
            setGlobalColor(globalColor);
            if (intData[0] == 0) {
              incomingByte = ending;
              parseStarted = false;
            } else {
              parseMode = NORMAL;
            }
          }
        } else {
          intData[parse_index] = string_convert.toInt();  // преобразуем строку в int и кладём в массив
        }
        string_convert = "";                        // очищаем строку
        parse_index++;                              // переходим к парсингу следующего элемента массива
      }
    }

    if (incomingByte == header) {                   // если это $
      parseStarted = true;                          // поднимаем флаг, что можно парсить
      parse_index = 0;                              // сбрасываем индекс
      string_convert = "";                          // очищаем строку
    }

    if (incomingByte == ending) {                   // если таки приняли ; - конец парсинга
      parseMode = NORMAL;
      parseStarted = false;                         // сброс
      recievedFlag = true;                          // флаг на принятие
      bufIdx = 0;
    }

    if (bufIdx >= packetSize) {                     // Весь буфер разобран 
      bufIdx = 0;
      packetSize = 0;
    }
  }
}

void sendPageParams(int page) {
  sendPageParams(page, BOTH);
}

void sendPageParams(int page, eSources src) {

  String str = "", color, text;
  CRGB c1, c2;
  int8_t tmp_eff = -1;
  bool err = false;
  
  switch (page) { 
    case 1:  // Настройки
      str = getStateString("W|H|DM|PD|IT|AL|RM|PW|BR|WU|WT|WR|WS|WC|WN|WZ");
      break;
    case 2:  // Эффекты
      str = getStateString("EF|UE|UT|UC|SE|SS|SQ|BE");
      break;
    case 3:  // Настройки бегущей строки
      str = getStateString("TE|TI|CT|ST|C2|OM|TS|TA|TX|TY");
      break;
    case 4:  // Настройки часов
      str = getStateString("CE|CC|CO|CK|NC|SC|C1|DC|DD|DI|NP|NT|NZ|NS|DW|OF");
      break;
    case 5:  // Настройки будильника
      str = getStateString("AL|AW|AT|AD|AE|MX|MU|MD|MV|MA|MB|MP");
      break;
    case 6:  // Настройки подключения
      str = getStateString("AU|AN|AA|NW|NA|IP|QZ|QA|QP|QS|QU|QW|QD|QR");
      break;
    case 7:  // Настройки режимов автовключения по времени
      str = getStateString("AM1T|AM1A|AM2T|AM2A|AM3T|AM3A|AM4T|AM4A");
      break;
    case 10:  // Загрузка картинок
      str = getStateString("W|H|BR|CL|SD");
      break;
    case 11:  // Рисование
      str = getStateString("W|H|BR|CL|SD");
      break;
    case 12:  // Игры
      str = getStateString("W|H|BR|SE|SD");
      break;
    case 91:  // Запрос текста бегущих строк для заполнения списка в программе
      str = getStateString("TX");
      break;
    case 92:  // Запрос текста бегущих строк для заполнения списка в программе
      str = getStateString("TZ");
      break;
#if (USE_MP3 == 1)
    case 93:  // Запрос списка звуков будильника
      str = getStateString("S1");
      break;
    case 94:  // Запрос списка звуков рассвета
      str = getStateString("S2");
      break;
#else      
    case 93:  // Запрос списка звуков будильника
    case 94:  // Запрос списка звуков рассвета
      // Если MP3-плеер отключен - просто игнорировать
      break;
#endif      
    case 95:  // Ответ состояния будильника - сообщение по инициативе сервера
      str = getStateString("AL");
      cmd95 = str;
      src = BOTH;
      break;
    case 96:  // Ответ демо-режима звука - сообщение по инициативе сервера
      str = getStateString("MP");
      cmd96 = str;
      src = BOTH;
      break;
    case 99:  // Запрос списка эффектов
      str = getStateString("LE");
      break;
    default:
      err = true;
      #if (USE_MQTT == 1)
      DynamicJsonDocument doc(256);
      String out;
      doc["message"] = F("unknown page");
      doc["text"]    = String(F("нет страницы с номером ")) + String(page);
      serializeJson(doc, out);      
      NotifyError(out);
      #endif
      break;
  }

  if (!err) {
    if (str.length() > 0) {
      // Отправить клиенту запрошенные параметры страницы / режимов
      str = "$18 " + str + ";";
      sendStringData(str, src);
    } else {
      sendAcknowledge(cmdSource);
    }
  }
}

void sendStringData(String &str, eSources src) {
  #if (USE_MQTT == 1)
  if (src == MQTT || src == BOTH) {
    SendMQTT(str);
  }
  #endif
  if (src == UDP || src == BOTH) {
    int max_text_size = sizeof(incomeBuffer);        // Размер приемного буфера формирования текста загружаемой из EEPROM строки
    memset(incomeBuffer, '\0', max_text_size);
    str.toCharArray(incomeBuffer, str.length() + 1);        
    udp.beginPacket(udp.remoteIP(), udp.remotePort());
    udp.write((const uint8_t*) incomeBuffer, str.length()+1);
    udp.endPacket();
    Serial.println(String(F("UDP ")) + udp.remoteIP().toString() + ":" + String(udp.remotePort()) + " >> " + String(incomeBuffer));
  }
}

String getStateString(String keys) {
  String str = "", s_tmp, key;

  // Ключи буквы/цифры, разделенные пробелами или пайпами '|' 
  // Если строка ключей ограничена квадратными скобками или кавычками - удалить их;
  // В конце может быть ";" - не требуется - удалить ее (в середине ее быть не может)
  keys.replace(";","");
  keys.replace("[","");
  keys.replace("]","");
  keys.replace("\"","");

  // Ключи могут быть разделены '|' или пробелами
  keys.replace(" ","|");
  keys.replace("/r"," ");
  keys.replace("/n"," ");
  keys.trim();
  
  int16_t pos_start = 0;
  int16_t pos_end = keys.indexOf('|', pos_start);
  int16_t len = keys.length();
  if (pos_end < 0) pos_end = len;

  // Некоторые параметры зависят от текущего эффекта
  // Текущим эффектом может быть эффект, отсутствующий в списке эффектов и включенный как служебный эффект - 
  // например "Ночные часы" или "IP адрес". 
  // В этом случае в приложении эффект не будет найден - индекс в списке комбобокса
  // будет 0 и приложение на телефоне крашится. В этом случае отправляем параметры случайного эффекта, точно из списка.
  int8_t tmp_eff = thisMode;
  if (tmp_eff >= SPECIAL_EFFECTS_START) {
    tmp_eff = random8(0, MAX_EFFECT - 1);
  }

  // Строка keys содержит ключи запрашиваемых данных, разделенных знаком '|', например "CE|CC|CO|CK|NC|SC|C1|DC|DD|DI|NP|NT|NZ|NS|DW|OF"
  while (pos_start < len && pos_end >= pos_start) {
    if (pos_end > pos_start) {      
      key = keys.substring(pos_start, pos_end);
      if (key.length() > 0) {
        s_tmp = getStateValue(key, tmp_eff);
        if (s_tmp.length() > 0) {
          str += s_tmp + "|";
        }
      }      
    }
    pos_start = pos_end + 1;
    pos_end = keys.indexOf('|', pos_start);
    if (pos_end < 0) pos_end = len;
  }

  len = str.length();
  if (len > 0 && str.charAt(len - 1) == '|') {
    str = str.substring(0, len - 1);
  }
  return str;
}

String getStateValue(String &key, int8_t effect) {

  // W:число     ширина матрицы
  // H:число     высота матрицы
  // AA:[текст]  пароль точки доступа
  // AD:число    продолжительность рассвета, мин
  // AE:число    эффект, использующийся для будильника
  // AO:X        включен будильник 0-нет, 1-да
  // AL:X        сработал будильник 0-нет, 1-да
  // AM1H:HH     час включения режима 1     00..23
  // AM1M:MM     минуты включения режима 1  00..59
  // AM1E:NN     номер эффекта режима 1:   -3 - не используется; -2 - выключить матрицу; -1 - ночные часы; 0 - включить случайный с автосменой; 1 - номер режима из списка EFFECT_LIST
  // AM2H:HH     час включения режима 2     00..23
  // AM2M:MM     минуты включения режима 2  00..59
  // AM2E:NN     номер эффекта режима 1:   -3 - не используется; -2 - выключить матрицу; -1 - ночные часы;  0 - включить случайный с автосменой; 1 - номер режима из списка EFFECT_LIST
  // AM3H:HH     час включения режима 1     00..23
  // AM3M:MM     минуты включения режима 1  00..59
  // AM3E:NN     номер эффекта режима 1:   -3 - не используется; -2 - выключить матрицу; -1 - ночные часы; 0 - включить случайный с автосменой; 1 - номер режима из списка EFFECT_LIST
  // AM4H:HH     час включения режима 2     00..23
  // AM4M:MM     минуты включения режима 2  00..59
  // AM4E:NN     номер эффекта режима 1:   -3 - не используется; -2 - выключить матрицу; -1 - ночные часы;  0 - включить случайный с автосменой; 1 - номер режима из списка EFFECT_LIST
  // AN:[текст]  имя точки доступа
  // AT: DW HH MM  часы-минуты времени будильника для дня недели DW 1..7 -> например "AT:1 09 15"
  // AU:X        создавать точку доступа 0-нет, 1-да
  // AW:число    битовая маска дней недели будильника b6..b0: b0 - пн .. b7 - вс
  // BE:число    контрастность эффекта
  // BR:число    яркость
  // C1:цвет     цвет режима "монохром" часов оверлея; цвет: 192,96,96 - R,G,B
  // C2:цвет     цвет режима "монохром" бегущей строки; цвет: 192,96,96 - R,G,B
  // CС:X        режим цвета часов оверлея: 0,1,2
  // CE:X        оверлей часов вкл/выкл, где Х = 0 - выкл; 1 - вкл (использовать часы в эффектах)
  // CK:X        размер горизонтальных часов, где Х = 0 - авто; 1 - малые 3x5; 2 - большие 5x7 
  // CL:X        цвет рисования в формате RRGGBB
  // CO:X        ориентация часов: 0 - горизонтально, 1 - вертикально
  // CT:X        режим цвета текстовой строки: 0,1,2
  // DC:X        показывать дату вместе с часами 0-нет, 1-да
  // DD:число    время показа даты при отображении часов (в секундах)
  // DI:число    интервал показа даты при отображении часов (в секундах)
  // DM:Х        демо режим, где Х = 0 - ручное управление; 1 - авторежим
  // DW:X        показывать температуру вместе с малыми часами 0-нет, 1-да
  // EF:число    текущий эффект
  // IP:xx.xx.xx.xx Текущий IP адрес WiFi соединения в сети
  // IT:число    время бездействия в секундах
  // LE:[список] список эффектов, разделенный запятыми, ограничители [] обязательны        
  // MA:число    номер файла звука будильника из SD:/01
  // MB:число    номер файла звука рассвета из SD:/02
  // MD:число    сколько минут звучит будильник, если его не отключили
  // MP:папка.файл  номер папки и файла звука который проигрывается
  // MU:X        использовать звук в будильнике 0-нет, 1-да
  // MV:число    максимальная громкость будильника
  // MX:X        MP3 плеер доступен для использования 0-нет, 1-да
  // NA:[текст]  пароль подключения к сети
  // NB:Х        яркость цвета ночных часов, где Х = 1..255
  // NС:Х        цвет ночных часов, где Х = 0 - R; 1 - G; 2 - B; 3 - C; 4 - M; 5 - Y;
  // NP:Х        использовать NTP, где Х = 0 - выкл; 1 - вкл
  // NS:[текст]  сервер NTP, ограничители [] обязательны
  // NT:число    период синхронизации NTP в минутах
  // NW:[текст]  SSID сети подключения
  // NZ:число    часовой пояс -12..+12
  // OF:X        выключать часы вместе с лампой 0-нет, 1-да
  // OM:X        сколько ячеек осталось свободно для хранения строк
  // PD:число    продолжительность режима в секундах
  // QA:X        использовать MQTT 0-нет, 1-да
  // QD:число    задержка отправки сообщения MQTT
  // QP:число    порт подключения к MQTT серверу
  // QR:X        топик использует префикс в виде имени сервера для формирования топика
  // QS:[text]   имя MQTT сервера, например QS:[srv2.clusterfly.ru]
  // QU:[text]   имя пользователя MQTT соединения, например QU:[user_af7cd12a]
  // QW:[text]   пароль MQTT соединения, например QW:[pass_eb250bf5]
  // QZ:X        сборка поддерживает MQTT 0-нет, 1-да
  // PW:число    ограничение по току в миллиамперах
  // RM:Х        смена режимов в случайном порядке, где Х = 0 - выкл; 1 - вкл
  // S1:[список] список звуков будильника, разделенный запятыми, ограничители [] обязательны        
  // S2:[список] список звуков рассвета, разделенный запятыми, ограничители [] обязательны        
  // SC:число    скорость смещения часов оверлея
  // SD:X        наличие и доступность SD карты: Х = 0 - нат SD карты; 1 - SD карта доступна
  // SE:число    скорость эффектов
  // SS:число    параметр #1 эффекта
  // SQ:спец     параметр #2 эффекта; спец - "L>val>itrm1,item2,..itemN" - список, где val - текущее, далее список; "C>x>title" - чекбокс, где x=0 - выкл, x=1 - вкл; title - текст чекбокса
  // ST:число    скорость смещения бегущей строки
  // TA:X        активная кнопка, для которой отправляется текст - 0..9,A..Z
  // TE:X        оверлей текста бегущей строки вкл/выкл, где Х = 0 - выкл; 1 - вкл (использовать бегущую строку в эффектах)
  // TI:число    интервал отображения текста бегущей строки
  // TS:строка   строка состояния кнопок выбора текста из массива строк: 36 символов 0..5, где
  //               0 - серый - пустая
  //               1 - черный - отключена
  //               2 - зеленый - активна - просто текст? без макросов
  //               3 - голубой - активна, содержит макросы кроме даты
  //               4 - синий - активная, содержит макрос даты
  //               5 - красный - для строки 0 - это управляющая строка
  // TX:[текст]  текст для активной строки. Ограничители [] обязательны
  // TY:[Z:текст] обработанный текст для активной строки, после преобразования макросов, если они есть. Ограничители [] обязательны Z - индекс строки в списке 0..35
  // TZ:[Z:текст] обработанный текст для активной строки, после преобразования макросов, если они есть. Ограничители [] обязательны Z - индекс строки в списке 0..35 (в ответ на получения TZ телефон отправляет запрос на следующую строку, в TY - нет)
  // UC:X        использовать часы поверх эффекта 0-нет, 1-да
  // UE:X        использовать эффект в демо-режиме 0-нет, 1-да
  // UT:X        использовать бегущую строку поверх эффекта 0-нет, 1-да
  // W1          текущая погода ('ясно','пасмурно','дождь'и т.д.)
  // W2          текущая температура
  // WC:X        Использовать цвет для отображения температуры в дневных часах  X: 0 - выключено; 1 - включено
  // WN:X        Использовать цвет для отображения температуры в ночных часах  X: 0 - выключено; 1 - включено
  // WR:число    Регион погоды Yandex
  // WS:число    Регион погоды OpeenWeatherMap
  // WT:число    Период запроса сведений о погоде в минутах
  // WU:X        Использовать получение погоды с сервера: 0 - выключено; 1 - включено
  // WZ:X        Прошивка поддерживает погоду USE_WEATHER == 1 - 0 - выключено; 1 - включено

  String str = "";
  
  // Ширина матрицы
  if (key == "W")  return str + "W:" + String(WIDTH);

  // Высота матрицы
  if (key == "H")  return str + "H:" + String(HEIGHT);

  // Текущая яркость
  if (key == "BR") return str + "BR:" + (isNightClock ? String(nightClockBrightness) : String(globalBrightness));

  // Ручной / Авто режим
  if (key == "DM") return str + "DM:" + (manualMode ? "0" : "1");

  // Продолжительность режима в секундах
  if (key == "PD") return str + "PD:" + String(autoplayTime / 1000); 

  // Время бездействия в секундах
  if (key == "IT") return str + "IT:" + String(idleTime / 60 / 1000);

  // Сработал будильник 0-нет, 1-да
  if (key == "AL") return str + "AL:" + (((isAlarming || isPlayAlarmSound) && !isAlarmStopped)  ? "1" : "0"); 

  // Смена режимов в случайном порядке, где Х = 0 - выкл; 1 - вкл
  if (key == "RM") return str + "RM:" + (useRandomSequence ? "1" : "0");

  // Ограничение по току в миллиамперах
  if (key == "PW") return str + "PW:" + String(CURRENT_LIMIT);

  // Прошивка поддерживает погоду 
  if (key == "WZ") return str + "WZ:" + String(USE_WEATHER);

#if (USE_WEATHER == 1)                  
  // Использовать получение погоды с сервера: 0 - выключено; 1 - Yandex; 2 - OpenWeatherMap
  if (key == "WU") return str + "WU:" + String(useWeather);

  // Период запроса сведений о погоде в минутах
  if (key == "WT") return str + "WT:" + String(SYNC_WEATHER_PERIOD);

  // Регион погоды Yandex
  if (key == "WR") return str + "WR:" + String(regionID);

  // Регион погоды OpenWeatherMap
  if (key == "WS") return str + "WS:" + String(regionID2);

  // Использовать цвет для отображения температуры в дневных часах: 0 - выключено; 1 - включено
  if (key == "WC") return str + "WC:" + (useTemperatureColor ? "1" : "0");

  // Использовать цвет для отображения температуры в ночных часах: 0 - выключено; 1 - включено
  if (key == "WN") return str + "WN:" + (useTemperatureColorNight ? "1" : "0");

  // Текущая погода
  if (key == "W1") return str + "W1:[" + weather + ']';

  // Текущая температура
  if (key == "W2") return str + "W2:" + String(temperature);
#endif  

  // Текущий эффект 
  if (key == "EF") return str + "EF:" + String(effect+1); // +1 т.к эффекты считаются с нуля, а индекс в списке эффектов - с 1

  // Использовать в демо-режиме
  if (key == "UE") return str + "UE:" + String((getEffectUsage(effect) ? "1" : "0"));

  // Оверлей бегущей строки
  if (key == "UT") return str + "UT:" + (effect == MC_MAZE || effect == MC_SNAKE || effect == MC_TETRIS || effect == MC_ARKANOID || effect == MC_CLOCK
         ? "X":
         (String(getEffectTextOverlayUsage(effect) ? "1" : "0")));

  // Оверлей часов   
  if (key == "UC") return str + "UC:" + (effect == MC_MAZE || effect == MC_SNAKE || effect == MC_TETRIS || effect == MC_ARKANOID || effect == MC_CLOCK
         ? "X" 
         : (String(getEffectClockOverlayUsage(effect) ? "1" : "0")));

  // Настройка скорости
  if (key == "SE") return str + "SE:" + (effect == MC_PACIFICA || effect == MC_SHADOWS || effect == MC_CLOCK || effect == MC_WATERFALL || effect == MC_IMAGE || effect == MC_FIRE2
         ? "X" 
         : String(255 - constrain(map(getEffectSpeed(effect), D_EFFECT_SPEED_MIN,D_EFFECT_SPEED_MAX, 0,255), 0,255)));

  // Контраст
  if (key == "BE") return str + "BE:" + (effect == MC_PACIFICA || effect == MC_DAWN_ALARM || effect == MC_MAZE || effect == MC_SNAKE || effect == MC_TETRIS || effect == MC_ARKANOID ||
                                         effect == MC_CLOCK || effect == MC_SDCARD
         ? "X" 
         : String(effectContrast[effect]));

  // Эффекты не имеющие настройки вариации (параметр #1) отправляют значение "Х" - программа делает ползунок настройки недоступным
  if (key == "SS") return str + "SS:" + getParamForMode(effect);

  // Эффекты не имеющие настройки вариации (параметр #2) отправляют значение "Х" - программа делает ползунок настройки недоступным
  if (key == "SQ") return str + "SQ:" + getParam2ForMode(effect);

  // Разрешен оверлей бегущей строки
  if (key == "TE") return str + "TE:" + String(getTextOverlayEnabled());

  // Интервал показа бегущей строки
  if (key == "TI") return str + "TI:" + String(getTextInterval());

  // Режим цвета отображения бегущей строки
  if (key == "CT") return str + "CT:" + String(COLOR_TEXT_MODE);

  // Скорость прокрутки текста
  if (key == "ST") return str + "ST:" + String(255 - getTextScrollSpeed());

  // Цвет режима "монохром" часов
  if (key == "C1") {
    CRGB c1 = CRGB(globalClockColor);
    return str + "C1:" + String(c1.r) + "," + String(c1.g) + "," + String(c1.b);
  }

  // Цвет режима "монохром" бегущей строки
  if (key == "C2") {
    CRGB c2 = CRGB(globalTextColor);
    return str + "C2:" + String(c2.r) + "," + String(c2.g) + "," + String(c2.b);
  }

  // Сколько ячеек осталось свободно для хранения строк
  if (key == "OM") return str + "OM:" + String(memoryAvail);

  // Строка состояния заполненности строк текста
  if (key == "TS") return str + "TS:" + getTextStates();

  // Активная кнопка текста. Должна быть ПОСЛЕ строки статуса, т.к и та и другая устанавливает цвет, но активная должна ставиться ПОСЛЕ
  if (key == "TA") return str + "TA:" + String(editIdx);                 

  // Активная кнопка текста. Должна быть ПЕРЕД строкой текста, т.к сначала приложение должно знать в какую позицию списка помещать строку editIdx - '0'..'9'..'A'..'Z'
  if (key == "TX") return str + "TX:[" + getTextByAZIndex(editIdx) + ']'; 

  // Исходная строка без обработки
  if (key == "TY" && editIdx != '#' && editIdx != '0') {
     return str + "TY:[" + String(getTextIndex(editIdx)) + ":" + String(editIdx) + " > '" + getTextByAZIndex(editIdx) + "']";      
  }

  // Запрос текста бегущих строк для заполнения списка в программе
  if (key == "TZ" && sendTextIdx >= 0 && (sendTextIdx < (sizeof(textLines) / sizeof(String)))) {
    return str + "TZ:[" + String(sendTextIdx) + ":" + String(getAZIndex(sendTextIdx)) + " > '" + getTextByAZIndex(getAZIndex(sendTextIdx)) + "']";     // Исходная строка без обработки
  }

  // Оверлей часов вкл/выкл
  if (key == "CE") return str + "CE:" + (allowVertical || allowHorizontal ? String(getClockOverlayEnabled()) : "X");

  // Режим цвета часов оверлея
  if (key == "CC") return str + "CC:" + String(COLOR_MODE);

  // Цвет рисования
  if (key == "CL") {
    String sHex = "00000" + String(drawColor, HEX);
    byte len = sHex.length();
    if (len > 6) {
      sHex = sHex.substring(len - 6);
      sHex.toUpperCase();
    }
    return str + "CL:" + sHex;
  }

  // Ориентация часов
  if (key == "CO") return str + "CO:" + (allowVertical && allowHorizontal ? String(CLOCK_ORIENT) : "X");

  // Размер (режим) горизонтальных часов
  if (key == "CK") return str + "CK:" + String(CLOCK_SIZE);

  // Яркость цвета ночных часов
  if (key == "NB") return str + "NB:" + String(nightClockBrightness);

  // Код цвета ночных часов
  if (key == "NC") return str + "NC:" + String(nightClockColor);

  // Скорость смещения (прокрутки) часов оверлея
  if (key == "SC") return str + "SC:" + String(255 - getClockScrollSpeed());

  // Показывать дату вместе с часами
  if (key == "DC") return str + "DC:" + (showDateInClock ? "1" : "0");

  // Продолжительность отображения даты в режиме часов (сек)
  if (key == "DD") return str + "DD:" + String(showDateDuration);

  // Интервал отображения даты часов
  if (key == "DI") return str + "DI:" + String(showDateInterval);

  // Использовать получение времени с интернета
  if (key == "NP") return str + "NP:" + (useNtp ? "1" : "0");

  // Период синхронизации NTP в минутах
  if (key == "NT") return str + "NT:" + String(SYNC_TIME_PERIOD); 

  // Часовой пояс
  if (key == "NZ") return str + "NZ:" + String(timeZoneOffset);

  // Имя сервера NTP (url)
  if (key == "NS") return str + "NS:[" + String(ntpServerName)+"]";

  // Показывать температуру вместе с малыми часами 0-нет, 1-да
  if (key == "DW") return str + "DW:" + (showWeatherInClock ? "1" : "0");

  // Выключать часы TM1637 вместе с лампой 0-нет, 1-да
  if (key == "OF") return str + "OF:" + (needTurnOffClock ? "1" : "0"); 

  // Продолжительность рассвета, мин
  if (key == "AD") return str + "AD:" + String(dawnDuration);

  // Битовая маска дней недели будильника
  if (key == "AW") {
    str = "AW:";
    for (int i=0; i<7; i++) {
       if (((alarmWeekDay>>i) & 0x01) == 1) str+="1"; else str+="0";  
       if (i<6) str+='.';
    }
    return str;
  }

  // Часы-минуты времени будильника по дням недели
  if (key == "AT") {
    for (int i=0; i<7; i++) {      
      str+="|AT:"+String(i+1)+" "+String(alarmHour[i])+" "+String(alarmMinute[i]);
    }
    // Убрать первый '|'
    return str.substring(1);
  }

  // Эффект применяемый в рассвете: Индекс в списке в приложении смартфона начинается с 1
  if (key == "AE") return str + "AE:" + String(alarmEffect + 1);                   

  // Доступность MP3-плеера    
  if (key == "MX") return str + "MX:" + String(isDfPlayerOk ? "1" : "0");
      
#if (USE_MP3 == 1)
  // Использовать звук будильника
  if (key == "MU") return str + "MU:" + String(useAlarmSound ? "1" : "0"); 

  // Сколько минут звучит будильник, если его не отключили
  if (key == "MD") return str + "MD:" + String(alarmDuration); 

  // Максимальная громкость будильника
  if (key == "MV") return str + "MV:" + String(maxAlarmVolume); 

  // Номер файла звука будильника из SD:/01
  if (key == "MA") return str + "MA:" + String(alarmSound+2);                      // Знач: -1 - нет; 0 - случайно; 1 и далее - файлы; -> В списке индексы: 1 - нет; 2 - случайно; 3 и далее - файлы

  // Номер файла звука рассвета из SD:/02
  if (key == "MB") return str + "MB:" + String(dawnSound+2);                       // Знач: -1 - нет; 0 - случайно; 1 и далее - файлы; -> В списке индексы: 1 - нет; 2 - случайно; 3 и далее - файлы

  // Номер папки и файла звука который проигрывается
  if (key == "MP") return str + "MP:" + String(soundFolder) + '~' + String(soundFile+2); 

  // Запрос звуков будильника
  if (key == "S1") return str + "S1:[" + String(ALARM_SOUND_LIST).substring(0,BUF_MAX_SIZE-12) + "]"; 

  // Запрос звуков рассвета
  if (key == "S2") return str + "S2:[" + String(DAWN_SOUND_LIST).substring(0,BUF_MAX_SIZE-12) + "]"; 
#endif

  // создавать точку доступа
  if (key == "AU") return str + "AU:" + String(useSoftAP ? "1" : "0");  

  // Имя точки доступа
  if (key == "AN") return str + "AN:[" + String(apName) +  "]";

  // Пароль точки доступа
  if (key == "AA") return str + "AA:[" + String(apPass) +  "]";

  // Имя локальной сети (SSID)
  if (key == "NW") return str + "NW:[" + String(ssid) +  "]";

  // Пароль к сети
  if (key == "NA") return str + "NA:[" + String(pass) +  "]";

  // IP адрес
  if (key == "IP") return str + "IP:" + String(wifi_connected ? WiFi.localIP().toString() : F("нет подключения"));  

  // Время Режима №1
  if (key == "AM1T") return str + "AM1T:"+String(AM1_hour)+" "+String(AM1_minute);

  // Действие Режима №1
  if (key == "AM1A") return str + "AM1A:"+String(AM1_effect_id);

  // Время Режима №2
  if (key == "AM2T") return str + "AM2T:"+String(AM2_hour)+" "+String(AM2_minute);

  // Действие Режима №2
  if (key == "AM2A") return str + "AM2A:"+String(AM2_effect_id); 

  // Время Режима №3
  if (key == "AM3T") return str + "AM3T:"+String(AM3_hour)+" "+String(AM3_minute);

  // Действие Режима №3
  if (key == "AM3A") return str + "AM3A:"+String(AM3_effect_id); 

  // Время Режима №4
  if (key == "AM4T") return str + "AM4T:"+String(AM4_hour)+" "+String(AM4_minute);

  // Действие Режима №4
  if (key == "AM4A") return str + "AM4A:"+String(AM4_effect_id);

  // Список эффектов прошивки
  if (key == "LE") return str + "LE:[" + String(EFFECT_LIST).substring(0,BUF_MAX_SIZE-12) + "]"; 

#if (USE_SD == 1)
  // Наличие и доступность SD карты
  if (key == "SD") return str + "SD:" + String(isSdCardReady); 
#endif

  // Прошивка поддерживает MQTT 0-нет, 1-да
  if (key == "QZ") return str + "QZ:" + String(USE_MQTT == 1 ? "1" : "0");  

#if (USE_MQTT == 1)

  // Использовать MQTT 0-нет, 1-да
  if (key == "QA") return str + "QA:" + String(useMQTT ? "1" : "0");  

  // QP:число    порт подключения к MQTT серверу
  if (key == "QP") return str + "QP:" + String(mqtt_port);  

  // QS:[text]   имя MQTT сервера, например QS:[srv2.clusterfly.ru]
  if (key == "QS") return str + "QS:[" + String(mqtt_server) +  "]";
  
  // QU:[text]   имя пользователя MQTT соединения, например QU:[user_af7cd12a]
  if (key == "QU") return str + "QU:[" + String(mqtt_user) +  "]";

  // QW:[text]   пароль MQTT соединения, например QW:[pass_eb250bf5]
  if (key == "QW") return str + "QW:[" + String(mqtt_pass) +  "]";

  // QD:число    задержка между отправками сообщений к MQTT серверу
  if (key == "QD") return str + "QD:" + String(mqtt_send_delay);  

  // Используется префикс в виде имени пользователя для формирования топика 0-нет, 1-да
  if (key == "QR") return str + "QR:" + String(mqtt_use_prefix ? "1" : "0");  
#endif


  // Запрошенный ключ не найден - вернуть пустую строку
  return "";
}

// Первый параметр эффекта thisMode для отправки на телефон параметра "SS:"
String getParamForMode(byte mode) {
 // Эффекты не имеющие настройки "Вариант" (параметр #1) отправляют значение "Х" - программа делает ползунок настройки недоступным 
 String str; 
 switch (mode) {
   case MC_DAWN_ALARM:
   case MC_COLORS:
   case MC_SWIRL:
   case MC_FLICKER:
   case MC_PACIFICA:
   case MC_SHADOWS:
   case MC_MAZE:
   case MC_SNAKE:
   case MC_TETRIS:
   case MC_ARKANOID:
   case MC_PALETTE:
   case MC_ANALYZER:
   case MC_PRIZMATA:
   case MC_MUNCH:
   case MC_ARROWS:
   case MC_WATERFALL:
   case MC_IMAGE:
   case MC_WEATHER:
   case MC_LIFE:
   case MC_PATTERNS:
   case MC_SDCARD:
   case MC_FIRE2:
     str = "X";
     break;
   case MC_CLOCK:
     str = isNightClock 
       ? String(map(nightClockColor, 0,6, 1,255))
       : String(effectScaleParam[thisMode]);
     break;
   default:
     str = String(effectScaleParam[thisMode]);
     break;
 }
 return str;   
}

// Второй параметр эффекта thisMode для отправки на телефон параметра "SQ:"
String getParam2ForMode(byte mode) {
 // Эффекты не имеющие настройки вариации (параметр #2) отправляют значение "Х" - программа делает ползунок настройки недоступным 
 String str = "X"; 
 switch (mode) {
   case MC_RAINBOW:
     // Эффект "Радуга" имеет несколько вариантов - список выбора варианта отображения
     // Дополнительный параметр представлен в приложении списком выбора
     //           Маркер типа - список выбора         0,1,2,3,4               0         1            2              3            4
     str = String(F("L>")) + String(effectScaleParam2[thisMode]) + String(F(">Случайный выбор,Вертикальная радуга,Горизонтальная радуга,Диагональная радуга,Вращающаяся радуга"));
     break;
   case MC_ARROWS:
     // Эффект "Стрелки" имеет несколько вариантов - список выбора варианта отображения
     // Дополнительный параметр представлен в приложении списком выбора
     //           Маркер типа - список выбора         0,1,2,3,4               0               1       2       3       4          5
     str = String(F("L>")) + String(effectScaleParam2[thisMode]) + String(F(">Случайный выбор,1-центр,2-центр,4-центр,2-смещение,4-смещение"));
     break;
   case MC_PATTERNS:
     // Эффект "Узоры" имеет несколько вариантов - список выбора варианта отображения
     // Дополнительный параметр представлен в приложении списком выбора
     //           Маркер типа - список выбора         0,1,2,3,4               0               1      2    3    4      5    7      8       9      10     11    12    13       14       15
     str = String(F("L>")) + String(effectScaleParam2[thisMode]) + String(F(">Случайный выбор,Зигзаг,Ноты,Ромб,Сердце,Елка,Клетка,Смайлик,Зигзаг,Полосы,Волны,Чешуя,Портьера,Плетенка,Снежинка,Квадратики,Греция,Круги,Рулет,Узор 1,Узор 2,Узор 3,Узор 4,Узор 5,Узор 6,Узор 7,Узор 8,Узор 9,Узор 10,Узор 11,Узор 12,Узор 13,Узор 14"));
     break;
   #if (USE_SD == 1)     
   case MC_SDCARD:
     // Эффект "SD-card" имеет несколько вариантов - список выбора файла эффекта
     // Дополнительный параметр представлен в приложении списком выбора
     // Весь список и имена файлов могут иметь слишком большую длину, которая не влезет в передаваемую строку (ограничение буфера), поэтому
     // список формируется просто по номерам - 001.002,003... и так далее
     str = String(F("L>")) + String(effectScaleParam2[thisMode]) + String(F(">Случайный выбор"));
     for (int8_t i=1; i <= countFiles; i++) {
       String tmp = "00" + String(i);
       if (tmp.length() > 3) tmp = tmp.substring(tmp.length() - 3);
       str += "," + tmp;
     }
     break;
     #endif
   case MC_IMAGE:
     // Эффект "Анимация" имеет несколько вариантов - список выбора отображаемой картинки
     // Дополнительный параметр представлен в приложении списком выбора
     //           Маркер типа - список выбора         0,1,2,3,4               0                           1, 2, ...
     str = String(F("L>")) + String(effectScaleParam2[thisMode]) + String(F(">Случайный выбор,")) + String(IMAGE_LIST);
     break;
   case MC_PAINTBALL:
   case MC_SWIRL:
   case MC_CYCLON:
     // Эффект "Пейнтбол" имеет параметр - использовать сегменты.
     // Эффект "Водоворот" имеет параметр - использовать сегменты.
     // Эффект "Циклон" имеет параметр - использовать сегменты.
     // Выкл - эффект растягивается на всю ширину матрицы
     // Вкл  - эффект квадратный. Если места достаточно для размещения более чем одного квадрата (например 32x16) - отображается два квадратных сегмента
     //        Если ширина матрицы не кратна ширине сегмента - между сегментами добавляются промежутки
     // Дополнительный параметр представлен в приложении чекбоксов - 0-выкл, 1-вкл
     //       Маркер типа - чекбокс                             false true            Текст чекбокса
     str = String(F("C>")) + (effectScaleParam2[thisMode] == 0 ? "0" : "1") + String(F(">Сегменты")); // "C>1>Сегменты"
     break;
 }
 return str;   
}

void sendAcknowledge(eSources src) {
  #if (USE_MQTT == 1)
  if (src == MQTT) {
    NotifyAck();
  }  
  #endif
  if (src == UDP || src == BOTH) {
    // Отправить подтверждение, чтобы клиентский сокет прервал ожидание
    String reply = "";
    bool isCmd = false; 
    if (cmd95.length() > 0) { reply += cmd95; cmd95 = ""; isCmd = true;}
    if (cmd96.length() > 0) { reply += cmd96; cmd96 = ""; isCmd = true; }
    reply += "ack" + String(ackCounter++) + ";";  
    reply.toCharArray(replyBuffer, reply.length()+1);
    udp.beginPacket(udp.remoteIP(), udp.remotePort());
    udp.write((const uint8_t*) replyBuffer, reply.length()+1);
    udp.endPacket();
    delay(0);
    if (isCmd) {
      Serial.println(String(F("Ответ на ")) + udp.remoteIP().toString() + ":" + String(udp.remotePort()) + " >> " + String(replyBuffer));
    }
  }
}

void setSpecialMode(int spc_mode) {
        
  loadingFlag = true;
  isTurnedOff = false;
  isNightClock = false;
  specialModeId = -1;

  String str;
  int8_t tmp_eff = -1;
  specialBrightness = globalBrightness;
  specialClock = getClockOverlayEnabled();

  switch(spc_mode) {
    case 0:  // Черный экран (выкл);
      tmp_eff = MC_FILL_COLOR;
      globalColor = 0x000000;
      specialBrightness = 0;
      specialClock = false;
      isTurnedOff = true;
      break;
    case 1:  // Белый экран (освещение);
      tmp_eff = MC_FILL_COLOR;
      globalColor = 0xffffff;
      specialBrightness = 255;
      break;
    case 2:  // Лампа указанного цвета;
      tmp_eff = MC_FILL_COLOR;
      globalColor = getGlobalColor();
      break;
    case 3:  // Огонь;
      tmp_eff = MC_FIRE;
      break;
    case 4:  // Пейнтбол;
      tmp_eff = MC_PAINTBALL;
      break;
    case 5:  // Радуга;
      tmp_eff = MC_RAINBOW;
      break;
    case 6:  // Матрица;
      tmp_eff = MC_MATRIX;
      break;
    case 7:  // Светлячки;
      tmp_eff = MC_LIGHTERS;
      break;
    case 8:  // Ночные часы;  
      tmp_eff = MC_CLOCK;
      specialClock = false;
      isNightClock = true;
      specialBrightness = nightClockBrightness < MIN_BRIGHT_FOR_NIGHT ? MIN_BRIGHT_FOR_NIGHT : nightClockBrightness;
      break;
    case 9:  // Палитра;
      tmp_eff = MC_PALETTE;
      break;
    case 10:  // Часы (отдельным эффектом, а не оверлеем);  
      tmp_eff = MC_CLOCK;
      setGlobalColor(getGlobalClockColor());                // цвет часов в режиме "Монохром"
      specialClock = true;
      specialBrightness = globalBrightness;
      break;
  }
  
  if (tmp_eff >= 0) {    
    // Дальнейшее отображение изображения эффекта будет выполняться стандартной процедурой customRoutine()
    thisMode = tmp_eff;
    specialMode = true;
    setTimersForMode(thisMode);
    // Таймер возврата в авторежим отключен    
    idleTimer.setInterval(4294967295);
    idleTimer.reset();
    FastLED.setBrightness(specialBrightness);
    specialModeId = spc_mode;
  }  

  setManualModeTo(true);
  setCurrentSpecMode(spc_mode);
  setCurrentManualMode(-1);
}

void resetModes() {
  // Отключение спец-режима перед включением других
  specialMode = false;
  specialModeId = -1;
  isTurnedOff = false;
  isNightClock = false;
  loadingFlag = false;
  wifi_print_ip = false;
  wifi_print_ip_text = false;
  gamePaused = false;
  isAlarming = false; 
  isPlayAlarmSound = false;
  manualMode = false;
  autoplayTimer = millis();
  autoplayTime = getAutoplayTime();  
}

void setEffect(byte eff) {

  resetModes();
  if (eff >= MAX_EFFECT && eff < SPECIAL_EFFECTS_START) return;
  
  if (eff == MC_TEXT)
    loadingTextFlag = true;
  else
    loadingFlag = true;

  thisMode = eff;

  setTimersForMode(thisMode);  

  setCurrentSpecMode(-1);
  if (manualMode){
    setCurrentManualMode(thisMode);
  } else {
    autoplayTimer = millis();
  }

  if (thisMode != MC_DAWN_ALARM)
    FastLED.setBrightness(globalBrightness);      
}

void showCurrentIP(boolean autoplay) {
  setEffect(MC_TEXT);
  textHasMultiColor = false;
  wifi_print_ip = true;  
  wifi_print_ip_text = true;
  wifi_print_idx = 0; 
  wifi_current_ip = wifi_connected ? WiFi.localIP().toString() : F("Нет подключения к сети WiFi");
  // Если параметр autoplay == true - включить режим автосмены режимов так,
  // что строка IP адреса будет показана несколько раз, затем автоматически включится другой режим.
  // autoplay == true - при установке IP адреса из программы
  // autoplay == false - при вызове отображения текущеко IP адреса по пятикратному нажатию кнопки.
  if (autoplay) {
    setManualModeTo(false);
  }
}

void setRandomMode() {
    String s_tmp = String(EFFECT_LIST);    
    uint32_t cnt = CountTokens(s_tmp, ','); 
    byte ef = random8(0, cnt); 
    setEffect(ef);
}

void setRandomMode2() {
  
  byte newMode, cnt = 0;
  
  #if (USE_SD == 1)  
  // На SD арте содержится более 40 эффектов плюсом к 40 эффектам, заданных в прошивке
  // Когда эффект следующий выбирается случайным образом, вероятность что выпадет SD-карта достаточно мала.
  // Искусственным образом увеличиваем вероятность эффекта с SD-карты
  if (getEffectUsage(MC_SDCARD) && isSdCardReady && (random16(0, 200) % 10 == 0)) {
    newMode = MC_SDCARD;
    setEffect(newMode);
    return;
  }   
  #endif

  while (cnt < 10) {
    cnt++;
    newMode = random16(0, MAX_EFFECT);
    if (!getEffectUsage(newMode)) continue;
    
    #if (USE_SD == 1)  
    if (newMode == MC_SDCARD && !isSdCardReady) continue;
    #endif

    setEffect(newMode);
    break;
  }
  
  if (cnt >= 10) setEffect(0);
}

void setManualModeTo(bool isManual) {
  manualMode = isManual;
  saveAutoplay(manualMode);
  idleState = !manualMode;
  if (idleTime == 0 || manualMode || specialMode) {
    idleTimer.setInterval(4294967295);
  } else {
    idleTimer.setInterval(idleTime);    
  }
  idleTimer.reset();
  autoplayTimer = millis();
}

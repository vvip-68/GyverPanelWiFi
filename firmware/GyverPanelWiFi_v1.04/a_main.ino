
// ----------------------------------------------------

// Временно для вывода информации о времени цикла
// uint32_t last_ms = millis();  
// char data[100];

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
    switch (thisMode) {
      case MC_CLOCK:
        tmpSaveMode = thisMode;
        Serial.print(F("Включен эффект "));
        Serial.println(F("'Часы'"));
        break;
      case MC_TEXT:
        tmpSaveMode = thisMode;
        Serial.print(F("Включен эффект "));
        Serial.println(F("'Бегущая строка'"));
        break;
      default:
        // Определить какой эффект включился
        String s_tmp = String(EFFECT_LIST);    
        uint16_t len1 = s_tmp.length();
        s_tmp = GetToken(s_tmp, thisMode+1, ',');
        uint16_t len2 = s_tmp.length();
        if (len1 > len2) { 
          tmpSaveMode = thisMode;
          Serial.print(F("Включен эффект "));
          Serial.println("'" + s_tmp + "'");
        } else {
          // Если режим отсутствует в списке эффектов - включить случайный
          setRandomMode2(); 
        }    
        break;
    }
  }

  // на время принятия данных матрицу не обновляем!
  if (!parseStarted) {
                         
    if (wifi_connected) {

      // Если настройки программы предполагают синхронизацию с NTP сервером и время пришло - выполнить синхронизацию
      if (useNtp) {
        if (ntp_t > 0 && millis() - ntp_t > 5000) {
          Serial.println(F("Таймаут NTP запроса!"));
          ntp_t = 0;
          ntp_cnt++;
          if (init_time && ntp_cnt >= 10) {
            Serial.println(F("Не удалось установить соединение с NTP сервером."));  
            refresh_time = false;
          }
        }
        
        bool timeToSync = ntpSyncTimer.isReady();
        if (timeToSync) { ntp_cnt = 0; refresh_time = true; }
        if (timeToSync || (refresh_time && ntp_t == 0 && (ntp_cnt < 100 || !init_time))) {
          getNTP();
          if (ntp_cnt >= 100) {
            if (init_time) {
              udp.flush();
            } else {
              //ESP.restart();
              ntp_cnt = 0;
              connectToNetwork();
            }
          }        
        }
      }

      #if (USE_WEATHER == 1)  
        if (useWeather) {   
          // Если настройки программы предполагают получение сведений о текущей погоде - выполнить обновление данных с погодного сервера
          if (weather_t > 0 && millis() - weather_t > 5000) {
            Serial.println(F("Таймаут запроса погоды!"));
            weather_t = 0;
            weather_cnt++;
            if (init_weather && weather_cnt >= 50) {
              Serial.println(F("Не удалось установить соединение с сервером погоды."));  
              refresh_weather = false;
            }
          }
          
          bool timeToGetWeather = weatherTimer.isReady(); 
          if (timeToGetWeather) { weather_cnt = 0; weather_t = 0; refresh_weather = true; }
          if (timeToGetWeather || (refresh_weather && weather_t == 0 && (weather_cnt < 50 || !init_weather))) {            
            weather_t = millis();
            getWeather();
            if (weather_cnt >= 50) {
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
          Serial.println("Вкл: " + String(saveMode));
          setEffect(saveMode);
        }
      } else {
        // Выключить панель, запомнив текущий режим
        saveSpecialMode = specialMode;
        saveSpecialModeId = specialModeId;
        saveMode = thisMode;
        // Выключить панель - черный экран
        setSpecialMode(0);
        setCurrentManualMode(saveMode);
        Serial.println("Выкл: " + String(saveMode));
      }
    }
    
    // Прочие клики работают только если не выключено
    if (isTurnedOff) {
      // Выключить питание матрицы
      #if (USE_POWER == 1)
        digitalWrite(POWER_PIN, POWER_OFF);
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

#if (DEVICE_TYPE == 0)
      // Четырехкратное нажатие включает режим "Лампа" из любого режима, в т.ч. из сна (обработка - ниже)            
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

#if (DEVICE_TYPE == 0)
    // Если устройство - ламна - длительное нажатие кнопки включает яркий белый свет
    else if (isButtonHold) {
        // Включить панель - белый цвет
        specialBrightness = 255;
        globalBrightness = 255;
        globalColor = 0xFFFFFF;
        isButtonHold = false;
        setSpecialMode(1);
        FastLED.setBrightness(globalBrightness);
    }

    // Четверное нажатие - включить белую лампу независимо от того была она выключена или включен любой другой режим
    if (clicks == 4) {
      // Включить лампу - белый цвет
      specialBrightness = 255;
      globalBrightness = 255;
      globalColor = 0xFFFFFF;
      setSpecialMode(1);
      FastLED.setBrightness(globalBrightness);
    }      
#endif

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
  String str;
  byte b_tmp;
  int8_t tmp_eff;
  char c;

  byte alarmHourVal;
  byte alarmMinuteVal;
  
  /*
    Протокол связи, посылка начинается с режима. Режимы:
    4 - яркость - 
      $4 0 value   установить текущий уровень общей яркости
    6 - текст $6 N|some text, где N - назначение текста;
        0 - текст бегущей строки N|X|text - X - 0..9,A..Z - индекс строки
        1 - имя сервера NTP
        2 - SSID сети подключения
        3 - пароль для подключения к сети 
        4 - имя точки доступа
        5 - пароль к точке доступа
        6 - настройки будильников
        7 - строка запрашиваемых параметров для процедуры getStateString(), например - "CE|CC|CO|CK|NC|SC|C1|DC|DD|DI|NP|NT|NZ|NS|DW|OF"
    8 - эффект
      - $8 0 N; включить эффект N
      - $8 1 N D; D -> параметр #1 для эффекта N;
      - $8 2 N X; вкл/выкл использовать в демо-режиме; N - номер эффекта, X=0 - не использовать X=1 - использовать 
      - $8 3 N D; D -> параметр #2 для эффекта N;
      - $8 4 N X; вкл/выкл оверлей текста поверх эффекта; N - номер эффекта, X=0 - выкл X=1 - вкл 
      - $8 5 N X; вкл/выкл оверлей часов поверх эффекта; N - номер эффекта, X=0 - выкл X=1 - вкл 
      - $8 6 N D; D -> контрастность эффекта N;
    12 - Настройки погоды
      - $12 3 X;   - использовать цвет для отображения температуры X=0 - выкл X=1 - вкл в дневных часах
      - $12 4 X;   - использовать получение погоды с погодного сервера
      - $12 5 I С; - интервал получения погоды с сервера в минутах (I) и код региона C
      - $12 6 X;   - использовать цвет для отображения температуры X=0 - выкл X=1 - вкл в ночных часах
    13 - Настройки бкгущей cтроки  
      - $13 0 N; - активация для редактирования строки с номером N - запрос текста строки
      - $13 1 N; - активация прокручивания строки с номером N
      - $13 3 I; - запросить текст бегущей строки с индексом I
      - $13 4 X; - использовать получение погоды с погодного сервера
      - $13 5 I С; - интервал получения погоды с сервера в минутах (I) и код региона C
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
    16 - Режим смены эффектов: $16 value; N: 2 - PrevMode; 3 - NextMode
    17 - Время автосмены эффектов и бездействия: $17 сек сек;
    18 - Запрос текущих параметров программой: $18 page; page - страница настройки в программе на смартфоне (1..7) или специальный параметр (92..99)
         page 1:  // Настройки
         page 2:  // Эффекты
         page 3:  // Настройки бегущей строки
         page 4:  // Настройки часов
         page 5:  // Настройки будильника
         page 6:  // Настройки подключения
         page 7:  // Настройки режимов автовключения по времени
         page 92: // Запрос текста бегущих строк для заполнения списка в программе
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
      - $20 5 VV; - установит уровень громкости проигрывания примеров (когда уже играет)
          VV - уровень громкости
    21 - настройки подключения к сети / точке доступа
      - $21 0 0 - не использовать точку доступа $21 0 1 - использовать точку доступа
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

    // Режимы 16,17,18  не сбрасывают idleTimer
    if (intData[0] < 16 || intData[0] > 18) {
      idleTimer.reset();
      idleState = false;      
    }

    // Режимы кроме 4 (яркость), 13.3 14 (новый спец-режим) и 18 (запрос параметров страницы),
    // 19 (настройки часов), 20 (настройки будильника), 21 (настройки сети) 
    // 23 (доп.параметры) - сбрасывают спец-режим
    if (specialMode) {
        if (intData[0] != 4 && !(intData[0] == 13 && intData[1] == 3) && 
            intData[0] != 14 && intData[0] != 18 && intData[0] != 19 &&
            intData[0] != 20 && intData[0] != 21 && intData[0] != 23) {
        idleTimer.setInterval(idleTime);
        idleTimer.reset();
        specialMode = false;
        isTurnedOff = false;
        isNightClock = false;
        specialModeId = -1;
      }
    }

    // Режимы кроме 18 останавливают будильник, если он работает (идет рассвет)
    if (intData[0] != 18) {
      wifi_print_ip = false;
      stopAlarm();
    }
    
    switch (intData[0]) {

      // ----------------------------------------------------
      // 4 - яркость - 
      //  $4 0 value   установить текущий уровень общей яркости
      // ----------------------------------------------------
      
      case 4:
        if (intData[1] == 0) {
          if (intData[1] == 0) {
            globalBrightness = intData[2];
            saveMaxBrightness(globalBrightness);
          }

          if (!isNightClock) {
            if (specialMode) specialBrightness = globalBrightness;
            FastLED.setBrightness(globalBrightness);
          }
        }
        sendAcknowledge();
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
      //   7 - строка запрашиваемых параметров для процедуры getStateString(), например - "CE|CC|CO|CK|NC|SC|C1|DC|DD|DI|NP|NT|NZ|NS|DW|OF"
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
              break;
            case 1:
              str.toCharArray(ntpServerName, 30);
              setNtpServer(str);
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

              // Настройки содержат 18 элеиентов (см. формат выше)
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
              // Передать строку для формирования, затем отправить параметры в приложение
              str = "$18 " + getStateString(str) + ";";
              break;
           }
        }

        // При сохранении текста бегущей строки (b_tmp == 0) не нужно сразу автоматически сохранять ее в EEPROM - Сохранение будет выполнено по таймеру. 
        // При получении запроса параметров (b_tmp == 7) ничего сохранять не нужно - просто отправить требуемые параметры
        // Остальные полученные строки - сохранять сразу, ибо это настройки сети, будильники и другая критически важная информация
        if (b_tmp != 0 && b_tmp != 7) {
          saveSettings();
        }
        
        if (b_tmp == 0) 
          sendPageParams(3);
        else if (b_tmp == 6) 
          sendPageParams(4);
        else if (b_tmp == 7) 
          sendStringData(str);
        else
          sendAcknowledge();
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
          // Если в приложении выбраны часы, но они недоступны из за размеров матрицы - брать другой случайный эффект
          if (tmp_eff == MC_CLOCK){
             if (!(allowHorizontal || allowVertical)) {
               setRandomMode2();
             }
          } 
          */         
          // Если в приложении выбраны эффект "Ночные часы", но они недоступны из за размеров матрицы - выключить матрицу
          if (tmp_eff == MC_CLOCK){
            if (!(allowHorizontal || allowVertical)) {
              setSpecialMode(0); // Выключить
            } else {
              setSpecialMode(8); 
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
          setScaleForEffect (tmp_eff,   intData[3]);
          effectScaleParam  [tmp_eff] = intData[3];
          if (tmp_eff == MC_FILL_COLOR) {  
            globalColor = getColorInt(CHSV(getEffectSpeed(MC_FILL_COLOR), effectScaleParam[MC_FILL_COLOR], 255));
            setGlobalColor(globalColor);
          } else 
          if (thisMode == tmp_eff && tmp_eff == MC_BALLS) {
            // При получении параметра эффекта "Шарики" (кол-во шариков) - надо переинициализировать эфект
            loadingFlag = true;
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
            // При получении параметра 2 эффекта "Радуга" (тип радуги) - надо переинициализировать эфект
            // Если установлен тип радуги - "случайный" - продолжаем показывать тот что был
            loadingFlag = effectScaleParam2[tmp_eff] != 0;
          } else
          if (thisMode == tmp_eff && tmp_eff == MC_ARROWS) {
            // При получении параметра 2 эффекта "Стрелки" -  вид - надо переинициализировать эфект
            // Если установлен тип - "случайный" - продолжаем показывать тот что был
            loadingFlag = effectScaleParam2[tmp_eff] != 0;
          } else
          if (thisMode == tmp_eff && tmp_eff == MC_PAINTBALL) {
            // При получении параметра 2 эффекта "Пэйнтбол" (сегменты) - надо переинициализировать эфект
            loadingFlag = true;
          } else
          if (thisMode == tmp_eff && tmp_eff == MC_SWIRL) {
            // При получении параметра 2 эффекта "Водоворот" (сегменты) - надо переинициализировать эфект
            loadingFlag = true;
          } else
          if (thisMode == tmp_eff && tmp_eff == MC_CYCLON) {
            // При получении параметра 2 эффекта "Циклон" (сегменты) - надо переинициализировать эфект
            loadingFlag = true;
          } else
          if (thisMode == tmp_eff && tmp_eff == MC_PATTERNS) {
            // При получении параметра 2 эффекта "Узоры" -  вид - надо переинициализировать эфект
            // Если установлен узор - "случайный" - продолжаем показывать тот что был
            loadingFlag = effectScaleParam2[tmp_eff] != 0;
          } else
          if (thisMode == tmp_eff && tmp_eff == MC_SDCARD) {
            // При получении параметра 2 эффекта "SD-карта" -  вид - надо переинициализировать эфект
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
        if (intData[1] != 1) {
          sendPageParams(2);
        } else { 
          sendAcknowledge();
        }
        break;

      // ----------------------------------------------------
      // 12 - Настройки погоды
      // - $12 3 X;   - использовать цвет для отображения температуры X=0 - выкл X=1 - вкл в дневных часах
      // - $12 4 X;   - использовать получение погоды с погодного сервера
      // - $12 5 I С; - интервал получения погоды с сервера в минутах (I) и код региона C
      // - $12 6 X;   - использовать цвет для отображения температуры X=0 - выкл X=1 - вкл в ночных часах
      // ----------------------------------------------------

      #if (USE_WEATHER == 1)                  
      case 12:
         switch (intData[1]) {
           case 3:               // $12 3 X; - Использовать отображение температуры цветом 0 - нет; 1 - да
             useTemperatureColor = intData[2] == 1;
             setUseTemperatureColor(useTemperatureColor);
             break;
           case 4:               // $12 4 X; - Использовать получение погоды с сервера 0 - нет; 1 - да
             useWeather = intData[2] == 1;
             setUseWeather(useWeather);
             if (wifi_connected) {
               refresh_weather = true; weather_t = 0; weather_cnt = 0;
             }
             break;
           case 5:               // $12 5 I C; - Интервал обновления погоды с сервера в минутах и Код региона
             SYNC_WEATHER_PERIOD = intData[2];
             regionID = intData[3];
             setWeatherInterval(SYNC_WEATHER_PERIOD);
             setWeatherRegion(regionID);
             weatherTimer.setInterval(1000L * 60 * SYNC_WEATHER_PERIOD);
             if (wifi_connected) {
               refresh_weather = true; weather_t = 0; weather_cnt = 0;
             }
             break;
           case 6:               // $12 6 X; - Использовать отображение температуры цветом 0 - нет; 1 - да
             useTemperatureColorNight = intData[2] == 1;
             setUseTemperatureColorNight(useTemperatureColorNight);
             break;
        }
        sendPageParams(1);
        break;
      #endif

      // ----------------------------------------------------
      // 13 - Настройки бегущей cтроки  
      // - $13 0 N;   - активация для редактирования строки с номером N - запрос текста строки N - 0..35 
      // - $13 1 N;   - активация прокручивания строки с номером N - 0..35
      // - $13 3 I;   - запросить текст бегущей строки с индексом I
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
           case 3:               // $13 3 I; - Запросить текст бегущей строки с индексом I
             if (intData[2] >= 1 && (intData[2]<(sizeof(textLines) / sizeof(String)))) {
               sendTextIdx = intData[2];
               sendPageParams(92);
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
             break;
        }
        if (intData[1] != 3) {
          sendPageParams(3);
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

        if (intData[1] == 2) {
           // Если в строке цвет - "$14 2 00FFAA;" - цвет лампы, сохраняемый в globalColor
           str = String(incomeBuffer).substring(6,12); // $14 2 00FFAA;
           globalColor = (uint32_t)HEXtoInt(str);
           setGlobalColor(globalColor);
        }
        setSpecialMode(intData[1]);
        sendPageParams(1);
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
        }
        sendAcknowledge();
        break;

      // ----------------------------------------------------
      // 16 - Режим смены эффектов: $16 value; N: 0 - ручной режим;  1 - авторежим; 2 - PrevMode; 3 - NextMode; 5 - вкл/выкл случайный выбор следующего режима
      // ----------------------------------------------------
      
      case 16:
        if      (intData[1] == 0) setManualModeTo(true);
        else if (intData[1] == 1) setManualModeTo(false);
        else if (intData[1] == 2) prevMode();
        else if (intData[1] == 3) nextMode();
        else if (intData[1] == 5) useRandomSequence = intData[2] == 1;

        saveRandomMode(useRandomSequence);        
        setCurrentManualMode(manualMode ? (int8_t)thisMode : -1);
        if (manualMode) {
          setCurrentSpecMode(-1);
        }

        sendPageParams(1);
        break;

      // ----------------------------------------------------
      // 17 - Время автосмены эффектов и бездействия: $17 сек
      ;
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
        if (idleTime == 0 || manualMode) // тамймер отключен
          idleTimer.setInterval(4294967295);
        else
          idleTimer.setInterval(idleTime);
        idleTimer.reset();
        sendAcknowledge();
        break;

      // ----------------------------------------------------
      // 18 - Запрос текущих параметров программой: $18 page;
      //    page 1:  // Настройки
      //    page 2:  // Эффекты
      //    page 3:  // Настройки бегущей строки
      //    page 4:  // Настройки часов
      //    page 5:  // Настройки будильника
      //    page 6:  // Настройки подключения
      //    page 7:  // Настройки режимов автовключения по времени
      //    page 92:  // Запрос текста бегущих строк для заполнения списка в программе
      //    page 93:  // Запрос списка звуков будильника
      //    page 94:  // Запрос списка звуков рассвета
      //    page 95:  // Ответ состояния будильника - сообщение по инициативе сервера
      //    page 96:  // Ответ демо-режима звука - сообщение по инициативе сервера
      //    page 99:  // Запрос списка эффектов
      // ----------------------------------------------------

      case 18: 
        if (intData[1] == 0) { // ping
          sendAcknowledge();
        } else {                          // запрос параметров страницы приложения
          sendPageParams(intData[1]);
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
                specialBrightness = MIN_BRIGHT_FOR_NIGHT;
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
             // ------ временно!
             // Приложение Thunkable для управления со смартфона достигло своего предела, при котором добавление новых элементов и блоков кода
             // уже невозможно - программа перестает собираться. По этой причине отображение температуры совместно с малыми часами
             // управляется синхронно с "Отображать дату в малых часах" - тем же чекбоксом
             // Когда / Если будет написана отдельная программа управления с телефона не на Thunkable, а на Android SDK - 
             // тогда для вкл/выкл отображения температуры в часах нужно завести отдельный контрол, который будет вкл/выкл отображение температуры
             // независимо от настроек даты в часах.
             // Специальная уоманда  $19 9 X; ужереализована (см. выше)
             showWeatherInClock = showDateInClock;
             setShowWeatherInClock(showWeatherInClock);
             // ------             
             break;
           case 17:               // $19 17 D I; - Продолжительность отображения даты / часов (в секундах)
             showDateDuration = intData[2];
             showDateInterval = intData[3];
             setShowDateDuration(showDateDuration);
             setShowDateInterval(showDateInterval);
             break;
        }
        if (intData[1] != 8) {
          sendPageParams(4);
        } else {
          sendAcknowledge();
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
        }
        if (intData[1] == 0) {
          sendPageParams(4);
        } else if (intData[1] == 1 || intData[1] == 2) { // Режимы установки параметров - сохранить
          // saveSettings();
          sendPageParams(4);
        } else {
          sendPageParams(96);
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
            startWiFi();
            showCurrentIP(true);
            break;
        }
        if (intData[1] == 0 || intData[1] == 1) {
          sendAcknowledge();
        } else {
          sendPageParams(5);
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
        sendPageParams(6);
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
        }
        break;

      // ----------------------------------------------------

    }
  }

  // ****************** ПАРСИНГ *****************

  // Если предыдущий буфер еще не разобран - новых данных из сокета не читаем, продолжаем разбор уже считанного буфера
  haveIncomeData = bufIdx > 0 && bufIdx < packetSize; 
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
      
      delay(0);            // ESP8266 при вызове delay отпрабатывает стек IP протокола, дадим ему поработать        

      Serial.print(F("UDP пакeт размером "));
      Serial.print(packetSize);
      Serial.print(F(" от "));
      IPAddress remote = udp.remoteIP();
      for (int i = 0; i < 4; i++) {
        Serial.print(remote[i], DEC);
        if (i < 3) {
          Serial.print(F("."));
        }
      }
      Serial.print(F(", порт "));
      Serial.println(udp.remotePort());
      if (udp.remotePort() == localPort) {
        Serial.print(F("Содержимое: "));
        Serial.println(incomeBuffer);
      }
    }

    // NTP packet from time server
    if (haveIncomeData && udp.remotePort() == 123) {
      parseNTP();
      haveIncomeData = false;
      bufIdx = 0;      
    }
  }

  if (haveIncomeData) {         

    // Из за ошибки в компоненте UdpSender в Thunkable - теряются половина отправленных 
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

  String str = "", color, text;
  CRGB c1, c2;
  int8_t tmp_eff = -1;
  
  switch (page) { 
    case 1:  // Настройки
      str = getStateString("W|H|DM|PD|ID|AL|RM|PW|BR|WU|WT|WR|WC|WN");
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
      str = getStateString("AL|AW|AT|AE|MX|MU|MD|MV|MA|MB|MP");
      break;
    case 6:  // Настройки подключения
      str = getStateString("AU|AN|AA|NW|NA|IP");
      break;
    case 7:  // Настройки режимов автовключения по времени
      str = getStateString("AM1T|AM1A|AM2T|AM2A|AM3T|AM3A|AM4T|AM4A");
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
#endif      
    case 95:  // Ответ состояния будильника - сообщение по инициативе сервера
      str = getStateString("AL");
      cmd95 = str;
      break;
    case 96:  // Ответ демо-режима звука - сообщение по инициативе сервера
      str = getStateString("MP");
      cmd96 = str;
      break;
    case 99:  // Запрос списка эффектов
      str = getStateString("LE");
      break;
  }
  
  if (str.length() > 0) {
    // Отправить клиенту запрошенные параметры страницы / режимов
    str = "$18 " + str + ";";
    sendStringData(str);
  } else {
    sendAcknowledge();
  }
}

void sendStringData(String &str) {
  str.toCharArray(incomeBuffer, str.length()+1);    
  udp.beginPacket(udp.remoteIP(), udp.remotePort());
  udp.write((const uint8_t*) incomeBuffer, str.length()+1);
  udp.endPacket();
  Serial.println(String(F("Ответ на ")) + udp.remoteIP().toString() + ":" + String(udp.remotePort()) + " >> " + String(incomeBuffer));
}

String getStateString(String keys) {
  String str = "", s_tmp, key;
  int16_t pos_start = 0;
  int16_t pos_end = keys.indexOf('|', pos_start);
  int16_t len = keys.length();
  if (pos_end < 0) pos_end = len;

  // Некоторые параметры зависят от текущего эффекта
  // Текущим эффектом может быть эффект, отсутствующий в списке эффектов и включенный как служебный эффект - 
  // например "Ночные часы" или "IP адрес". 
  // В этом случаее в приложении эффект не будет найден - индекс в списке комбобокса
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
  // DM:Х        демо режим, где Х = 0 - ручное управление; 1 - авторежим
  // AP:Х        автосменарежимов, где Х = 0 - выкл; 1 - вкл
  // RM:Х        смена режимов в случайном порядке, где Х = 0 - выкл; 1 - вкл
  // PD:число    продолжительность режима в секундах
  // IT:число    время бездействия в секундах
  // BR:число    яркость
  // BE:число    контрастность эффекта
  // EF:число    текущий эффект
  // SE:число    скорость эффектов
  // SS:число    параметр #1 эффекта
  // SQ:спец     параметр #2 эффекта; спец - "L>val>itrm1,item2,..itemN" - список, где val - текущее, далее список; "C>x>title" - чекбокс, где x=0 - выкл, x=1 - вкл; title - текст чекбокса
  // NP:Х        использовать NTP, где Х = 0 - выкл; 1 - вкл
  // NT:число    период синхронизации NTP в минутах
  // NZ:число    часовой пояс -12..+12
  // NS:[текст]  сервер NTP, ограничители [] обязательны
  // NС:Х        цвет ночных часов, где Х = 0 - R; 1 - G; 2 - B; 3 - C; 4 - M; 5 - Y;
  // OF:X        выключать часы вместе с лампой 0-нет, 1-да
  // DC:X        показывать дату вместе с часами 0-нет, 1-да
  // DD:число    время показа даты при отображении часов (в секундах)
  // DI:число    интервал показа даты при отображении часов (в секундах)
  // UE:X        использовать эффект в демо-режиме 0-нет, 1-да
  // UT:X        использовать бегущую строку поверх эффекта 0-нет, 1-да
  // UC:X        использовать часы поверх эффекта 0-нет, 1-да
  // LE:[список] список эффектов, разделенный запятыми, ограничители [] обязательны        
  // AL:X        сработал будильник 0-нет, 1-да
  // AT: DW HH MM  часы-минуты времени будильника для дня недели DW 1..7 -> например "AT:1 09 15"
  // AW:число    битовая маска дней недели будильника b6..b0: b0 - пн .. b7 - вс
  // AD:число    продолжительность рассвета, мин
  // AE:число    эффект, использующийся для будильника
  // AO:X        включен будильник 0-нет, 1-да
  // NW:[текст]  SSID сети подключения
  // NA:[текст]  пароль подключения к сети
  // AU:X        создавать точку доступа 0-нет, 1-да
  // AN:[текст]  имя точки доступа
  // AA:[текст]  пароль точки доступа
  // MX:X        MP3 плеер доступен для использования 0-нет, 1-да
  // MU:X        использовать звук в будильнике 0-нет, 1-да
  // MD:число    сколько минут звучит будильник, если его не отключили
  // MV:число    максимальная громкость будильника
  // MA:число    номер файла звука будильника из SD:/01
  // MB:число    номер файла звука рассвета из SD:/02
  // MP:папка.файл  номер папки и файла звука который проигрывается
  // IP:xx.xx.xx.xx Текущий IP адрес WiFi соединения в сети
  // AM1H:HH     час включения режима 1     00..23
  // AM1M:MM     минуты включения режима 1  00..59
  // AM1E:NN     номер эффекта режима 1:   -3 - не используется; -2 - выключить матрицу; -1 - ночные часы; 0 - включить случайный с автосменой; 1 - номер режима из спписка EFFECT_LIST
  // AM2H:HH     час включения режима 2     00..23
  // AM2M:MM     минуты включения режима 2  00..59
  // AM2E:NN     номер эффекта режима 1:   -3 - не используется; -2 - выключить матрицу; -1 - ночные часы;  0 - включить случайный с автосменой; 1 - номер режима из спписка EFFECT_LIST
  // AM3H:HH     час включения режима 1     00..23
  // AM3M:MM     минуты включения режима 1  00..59
  // AM3E:NN     номер эффекта режима 1:   -3 - не используется; -2 - выключить матрицу; -1 - ночные часы; 0 - включить случайный с автосменой; 1 - номер режима из спписка EFFECT_LIST
  // AM4H:HH     час включения режима 2     00..23
  // AM4M:MM     минуты включения режима 2  00..59
  // AM4E:NN     номер эффекта режима 1:   -3 - не используется; -2 - выключить матрицу; -1 - ночные часы;  0 - включить случайный с автосменой; 1 - номер режима из спписка EFFECT_LIST
  // PW:число    ограничение по току в миллиамперах
  // CK:X        размер горизонтальных часов, где Х = 0 - авто; 1 - малые 3x5; 2 - большие 5x7 
  // CE:X        оверлей часов вкл/выкл, где Х = 0 - выкл; 1 - вкл (использовать часы в эффектах)
  // CС:X        режим цвета часов оверлея: 0,1,2
  // CT:X        режим цвета текстовой строки: 0,1,2
  // CO:X        ориентация часов: 0 - горизонтально, 1 - вертикально
  // TE:X        оверлей текста бегущей строки вкл/выкл, где Х = 0 - выкл; 1 - вкл (использовать бегущую строку в эффектах)
  // TI:число    интервал отображения текста бегущей строки
  // SC:число    скорость смещения часов оверлея
  // ST:число    скорость смещения бегущей строки
  // C1:цвет     цвет режима "монохром" часов оверлея; цвет: 192,96,96 - R,G,B
  // C2:цвет     цвет режима "монохром" бегущей строки; цвет: 192,96,96 - R,G,B
  // S1:[список] список звуков будильника, разделенный запятыми, ограничители [] обязательны        
  // S2:[список] список звуков рассвета, разделенный запятыми, ограничители [] обязательны        
  // TS:строка   строка состояния кнопок выбора текста из массива строк: 36 символов 0..5, где
  //               0 - серый - пустая
  //               1 - черный - отключена
  //               2 - зеленый - активна - просто текст? без макросов
  //               3 - голубой - активна, содержит макросы кроме даты
  //               4 - синий - активная, содержит макрос даты
  //               5 - красный - для строки 0 - это управляющая строка
  // TA:X        активная кнопка, для которой отправляется текст - 0..9,A..Z
  // TX:[текст]  текст для активной строки. Ограничители [] обязательны
  // TY:[Z:текст] обработанный текст для активной строки, после преобразования макросов, если они есть. Ограничители [] обязательны Z - индекс строки в списке 0..35
  // TZ:[Z:текст] обработанный текст для активной строки, после преобразования макросов, если они есть. Ограничители [] обязательны Z - индекс строки в списке 0..35 (в ответ на получения TZ телефон отправляет запрос на следующую строку, в TY - нет)
  // OM:X        сколько ячеек осталось свободно для хранения строк
  // WU:X        Использовать получение погоды с сервераж X: 0 - выключено; 1 - включено
  // WT:число    Период запроса сведений о погоде в минутах
  // WR:число    Регион погоды - https://tech.yandex.ru/xml/doc/dg/reference/regions-docpage/
  // WC:X        Использовать цвет для отображения температуры в дневных часах  X: 0 - выключено; 1 - включено
  // WN:X        Использовать цвет для отображения температуры в ночных часах  X: 0 - выключено; 1 - включено
  // DW:X        показывать температуру вместе с малыми часами 0-нет, 1-да

  String str = "";
  
  // Ширина матрицы
  if (key == "W")  return str + "W:" + String(WIDTH);

  // Высота матрицы
  if (key == "H")  return str + "H:" + String(HEIGHT);

  // Текущая яркость
  if (key == "BR") return str + "BR:" + String(globalBrightness);

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
  
#if (USE_WEATHER == 1)                  
  // Использовать получение погоды с сервераж: 0 - выключено; 1 - включено
  if (key == "WU") return str + "WU:" + (useWeather ? "1" : "0");

  // Период запроса сведений о погоде в минутах
  if (key == "WT") return str + "WT:" + String(SYNC_WEATHER_PERIOD);

  // Регион погоды
  if (key == "WR") return str + "WR:" + String(regionID);

  // Использовать цвет для отображения температуры в дневных часах: 0 - выключено; 1 - включено
  if (key == "WC") return str + "WC:" + (useTemperatureColor ? "1" : "0");

  // Использовать цвет для отображения температуры в ночных часах: 0 - выключено; 1 - включено
  if (key == "WN") return str + "WN:" + (useTemperatureColorNight ? "1" : "0");
#endif  

  // Текущий эффект 
  if (key == "EF") return str + "EF:" + String(effect+1); // +1 т.к эффекты считаются с нуля, а индекс в списке эффектов - с 1

  // Использовать в демо-режиме
  if (key == "UE") return str + "UE:" + (effect == MC_CLOCK
         ? "X":
         (String((getEffectUsage(effect) ? "1" : "0"))));

  // Оверлей бегущей строки
  if (key == "UЕ") return str + "UT:" + (effect == MC_MAZE || effect == MC_SNAKE || effect == MC_TETRIS || effect == MC_CLOCK
         ? "X":
         (String(getEffectTextOverlayUsage(effect) ? "1" : "0")));

  // Оверлей часов   
  if (key == "UC") return str + "UC:" + (effect == MC_MAZE || effect == MC_SNAKE || effect == MC_TETRIS || effect == MC_CLOCK
         ? "X" 
         : (String(getEffectClockOverlayUsage(effect) ? "1" : "0")));

  // Настройка скорости
  if (key == "SE") return str + "SE:" + (effect == MC_PACIFICA || effect == MC_SHADOWS || effect == MC_CLOCK || effect == MC_WATERFALL || effect == MC_IMAGE
         ? "X" 
         : String(255 - constrain(map(getEffectSpeed(effect), D_EFFECT_SPEED_MIN,D_EFFECT_SPEED_MAX, 0,255), 0,255)));

  // Контраст
  if (key == "BE") return str + "BE:" + (effect == MC_PACIFICA || effect == MC_DAWN_ALARM || effect == MC_MAZE || effect == MC_SNAKE || effect == MC_TETRIS || 
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

  // // Исходная строка без обработки
  if (key == "TY" && editIdx != '#' && editIdx != '0') {
     return str + "TY:[" + String(getTextIndex(editIdx)) + ":" + String(editIdx) + " > '" + getTextByAZIndex(editIdx) + "']";      
  }

  // Оверлей часов вкл/выкл
  if (key == "CE") return str + "CE:" + (allowVertical || allowHorizontal ? String(getClockOverlayEnabled()) : "X");

  // Режим цвета часов оверлея
  if (key == "CC") return str + "CC:" + String(COLOR_MODE);

  // Ориентация часов
  if (key == "CO") return str + "CO:" + (allowVertical && allowHorizontal ? String(CLOCK_ORIENT) : "X");

  // Размер (режим) горизонтальных часов
  if (key == "CK") return str + "CK:" + String(CLOCK_SIZE);

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
  if (key == "S1") return str + "S1:[" + String(ALARM_SOUND_LIST).substring(0,BUF_MAX_SIZE-12) + "];"; 

  // Запрос звуков рассвета
  if (key == "S2") return str + "S2:[" + String(DAWN_SOUND_LIST).substring(0,BUF_MAX_SIZE-12) + "];"; 
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

  // Запрос текста бегущих строк для заполнения списка в программе
  if (key == "TZ" && sendTextIdx >= 1 && (sendTextIdx < (sizeof(textLines) / sizeof(String)))) {
    return str + "TZ:[" + String(sendTextIdx) + ":" + String(getAZIndex(sendTextIdx)) + " > '" + getTextByAZIndex(getAZIndex(sendTextIdx)) + "']";     // Исходная строка без обработки
  }

  // Список эффектов прошивки
  if (key == "LE") return str + "LE:[" + String(EFFECT_LIST).substring(0,BUF_MAX_SIZE-12) + "]"; 

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
   case MC_CLOCK:
   case MC_SDCARD:
     str = "X";
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
     str = String(F("L>")) + String(effectScaleParam2[thisMode]) + String(F(">Случайный выбор,Зигзаг,Ноты,Ромб,Сердце,Елка,Клетка,Смайлик,Зигзаг,Полосы,Волны,Чешуя,Портьера,Плетенка,Снежинка"));
     break;
   #if (USE_SD == 1)     
   case MC_SDCARD:
     // Эффект "SD-card" имеет несколько вариантов - список выбора файла эффекта
     // Дополнительный параметр представлен в приложении списком выбора
     // Весь список и имена файлов согут иметь слишком большую длину, которая не влезет в передаваемую строку (ограничение буфера), поэтому
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
     // Вкл  - эффект квадратный. Если влазит болеее одного квадрата (например 32x16) - отображается два квадратных сегмента
     //        Если ширина матрицы не кратна ширине сегмента - между сегментами добавляются промежутки
     // Дополнительный параметр представлен в приложении чекбоксов - 0-выкл, 1-вкл
     //       Маркер типа - чекбокс                             false true            Текст чекбокса
     str = String(F("C>")) + (effectScaleParam2[thisMode] == 0 ? "0" : "1") + String(F(">Сегменты")); // "C>1>Сегменты"
     break;
 }
 return str;   
}

void sendAcknowledge() {
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
      specialBrightness = MIN_BRIGHT_FOR_NIGHT;
      break;
    case 9:  // Палитра;
      tmp_eff = MC_PALETTE;
      break;
    case 10:  // Часы (отдельным эффектом, а не оверлеем);  
      tmp_eff = MC_CLOCK;
      setGlobalColor(getGlobalClockColor());                // цвет часов в режиме "Монохром"
      specialClock = true;
      break;
  }
  
  if (tmp_eff >= 0) {    
    // Дльнейшее отображение изображения эффекта будет выполняться стандартной процедурой customRoutine()
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
  isTurnedOff = false;
  isNightClock = false;
  specialModeId = -1;
  loadingFlag = false;
  wifi_print_ip = false;
  wifi_print_ip_text = false;
}

void setEffect(byte eff) {

  resetModes();
  if (eff >= MAX_EFFECT) return;
  
  if (eff == MC_TEXT)
    loadingTextFlag = true;
  else
    loadingFlag = true;

  thisMode = eff;

  setTimersForMode(thisMode);  

  setCurrentSpecMode(-1);
  if (manualMode)
    setCurrentManualMode(thisMode);

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
  // Искуственным образом увеличиваем вероятность эффекта с SD-карты
  if (getEffectUsage(MC_SDCARD) && (random16(0, 200) % 10 == 0)) {
    newMode = MC_SDCARD;
    setEffect(newMode);
    return;
  }   
  #endif

  while (cnt < 10) {
    cnt++;
    newMode = random16(0, MAX_EFFECT);
    if (!getEffectUsage(newMode)) continue;

    setEffect(newMode);
    break;
  }
  
  if (cnt >= 10) setEffect(0);
}

void setManualModeTo(bool isManual) {
  manualMode = isManual;
  saveAutoplay(!manualMode);
  idleState = !manualMode;
  if (idleTime == 0 || manualMode || specialMode) {
    idleTimer.setInterval(4294967295);
  } else {
    idleTimer.setInterval(idleTime);    
  }
  idleTimer.reset();
  if (!manualMode) {
    autoplayTimer = millis(); // При включении автоматического режима сбросить таймер автосмены режимов
  }
}

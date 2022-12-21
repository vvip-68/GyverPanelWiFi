void loadSettings() {

  // Адреса в EEPROM:
  //    0 - если EEPROM_OK - EEPROM инициализировано, если другое значение - нет                             // EEPROMread(0)                 // EEPROMWrite(0, EEPROM_OK)
  //    1 - максимальная яркость ленты 1-255                                                                 // getMaxBrightness()            // putMaxBrightness(globalBrightness)
  //    2 - автосмена режима в демо: вкл/выкл                                                                // getAutoplay();                // putAutoplay(manualMode)
  //    3 - время автосмены режимов в сек                                                                    // getAutoplayTime()             // putAutoplayTime(autoplayTime / 1000L)     // autoplayTime - мс; в ячейке - в сек
  //    4 - время бездействия до переключения в авторежим в минутах                                          // getIdleTime()                 // putIdleTime(idleTime / 60L / 1000L)       // idleTime - мс; в ячейке - в мин
  //    5 - использовать синхронизацию времени через NTP                                                     // getUseNtp()                   // putUseNtp(useNtp)
  //  6,7 - период синхронизации NTP (int16_t - 2 байта) в минутах                                           // getNtpSyncTime()              // putNtpSyncTime(SYNC_TIME_PERIOD)
  //    8 - time zone UTC+X                                                                                  // getTimeZone();                // putTimeZone(timeZoneOffset)
  //    9 - выключать индикатор часов при выключении лампы true - выключать / false - не выключать           // getTurnOffClockOnLampOff()    // putTurnOffClockOnLampOff(needTurnOffClock)
  //   10 - IP[0]                                                                                            // getStaticIP()                 // putStaticIP(IP_STA[0], IP_STA[1], IP_STA[2], IP_STA[3])
  //   11 - IP[1]                                                                                            // - " -                         // - " -
  //   12 - IP[2]                                                                                            // - " -                         // - " -
  //   13 - IP[3]                                                                                            // - " -                         // - " -
  //   14 - Использовать режим точки доступа                                                                 // getUseSoftAP()                // putUseSoftAP(useSoftAP)
  //   15 - ориентация часов горизонтально / вертикально                                                    // getClockOrientation()         // putClockOrientation(CLOCK_ORIENT)
  //   16 - Отображать с часами текущую дату                                                                 // getShowDateInClock()          // putShowDateInClock(showDateInClock)
  //   17 - Кол-во секунд отображения даты                                                                   // getShowDateDuration()         // putShowDateDuration(showDateDuration)
  //   18 - Отображать дату каждые N секунд                                                                  // getShowDateInterval()         // putShowDateInterval(showDateInterval)
  //   19 - тип часов горизонтальной ориентации 0-авто 1-малые 3х5 2 - большие 5х7                           // getClockSize()                // putClockSize(CLOCK_SIZE)
  //   20 - Будильник, дни недели                                                                            // getAlarmWeekDay()             // putAlarmParams(alarmWeekDay, dawnDuration, alarmEffect, alarmDuration)
  //   21 - Будильник, продолжительность "рассвета"                                                          // getDawnDuration()             // putAlarmParams(alarmWeekDay, dawnDuration, alarmEffect, alarmDuration)
  //   22 - Будильник, эффект "рассвета"                                                                     // getAlarmEffect()              // putAlarmParams(alarmWeekDay, dawnDuration, alarmEffect, alarmDuration)
  //   23 - Будильник, использовать звук                                                                     // getUseAlarmSound()            // putAlarmSounds(useAlarmSound, maxAlarmVolume, alarmSound, dawnSound)
  //   24 - Будильник, играть звук N минут после срабатывания                                                // getAlarmDuration()            // putAlarmParams(alarmWeekDay, dawnDuration, alarmEffect, alarmDuration)
  //   25 - Будильник, Номер мелодии будильника (из папки 01 на SD карте)                                    // getAlarmSound()               // putAlarmSounds(useAlarmSound, maxAlarmVolume, alarmSound, dawnSound)
  //   26 - Будильник, Номер мелодии рассвета (из папки 02 на SD карте)                                      // getDawnSound()                // putAlarmSounds(useAlarmSound, maxAlarmVolume, alarmSound, dawnSound)
  //   27 - Будильник, Максимальная громкость будильника                                                     // getMaxAlarmVolume()           // putAlarmSounds(useAlarmSound, maxAlarmVolume, alarmSound, dawnSound)
  //   28 - Номер последнего активного спец-режима или -1, если были включены обычные эффекты                // getCurrentSpecMode()          // putCurrentSpecMode(xxx)                     
  //   29 - Номер последнего активированного вручную режима                                                  // getCurrentManualMode()        // putCurrentManualMode(xxx)
  //   30 - Отображать часы оверлеем в режимах                                                               // getClockOverlayEnabled()      // putClockOverlayEnabled(clockOverlayEnabled)
  //   31 - Использовать случайную последовательность в демо-режиме                                          // getRandomMode()               // putRandomMode(useRandomSequence)
  //   32 - Отображать с часами текущую температуру                                                          // getShowWeatherInClock()       // putShowWeatherInClock(showWeatherInClock)
  //   33 - Режим 1 по времени - часы                                                                        // getAM1hour()                  // putAM1hour(AM1_hour)
  //   34 - Режим 1 по времени - минуты                                                                      // getAM1minute()                // putAM1minute(AM1_minute) 
  //   35 - Режим 1 по времени - -3 - выкл. (не исп.); -2 - выкл. (черный экран); -1 - ночн.часы, 0 - случ., // getAM1effect()                // putAM1effect(AM1_effect_id)
  //   36 - Режим 2 по времени - часы                                      ^^^ 1,2..N - эффект EFFECT_LIST  // getAM2hour()                  // putAM2hour(AM2_hour) 
  //   37 - Режим 2 по времени - минуты                                                                      // getAM2minute()                // putAM2minute(AM2_minute)
  //   38 - Режим 2 по времени - = " = как для режима 1                                                      // getAM2effect()                // putAM2effect(AM2_effect_id)
  //   39 - Цвет ночных часов:  0 - R; 1 - G; 2 - B; 3 - C; 4 - M; 5 - Y; 6 - W;                             // getNightClockColor()          // putNightClockColor(nightClockColor)          
  //   40 - Будильник, время: понедельник : часы                                                             // getAlarmHour(1)               // putAlarmTime(1, alarmHour[0], alarmMinute[0])  // for (byte i=0; i<7; i++) alarmHour[i] = getAlarmHour(i+1)
  //   41 - Будильник, время: понедельник : минуты                                                           // getAlarmMinute(1)             // putAlarmTime(1, alarmHour[0], alarmMinute[0])  // for (byte i=0; i<7; i++) alarmMinute[i] = getAlarmMinute(i+1)
  //   42 - Будильник, время: вторник : часы                                                                 // getAlarmHour(2)               // putAlarmTime(2, alarmHour[1], alarmMinute[1])  // for (byte i=0; i<7; i++) putAlarmTime(i+1, alarmHour[i], alarmMinute[i])
  //   43 - Будильник, время: вторник : минуты                                                               // getAlarmMinute(2)             // putAlarmTime(2, alarmHour[1], alarmMinute[1])
  //   44 - Будильник, время: среда : часы                                                                   // getAlarmHour(3)               // putAlarmTime(3, alarmHour[2], alarmMinute[2])
  //   45 - Будильник, время: среда : минуты                                                                 // getAlarmMinute(3)             // putAlarmTime(3, alarmHour[2], alarmMinute[2])
  //   46 - Будильник, время: четверг : часы                                                                 // getAlarmHour(4)               // putAlarmTime(4, alarmHour[3], alarmMinute[3])
  //   47 - Будильник, время: четверг : минуты                                                               // getAlarmMinute(4)             // putAlarmTime(4, alarmHour[3], alarmMinute[3])
  //   48 - Будильник, время: пятница : часы                                                                 // getAlarmHour(5)               // putAlarmTime(5, alarmHour[4], alarmMinute[4])
  //   49 - Будильник, время: пятница : минуты                                                               // getAlarmMinute(5)             // putAlarmTime(5, alarmHour[4], alarmMinute[4])
  //   50 - Будильник, время: суббота : часы                                                                 // getAlarmHour(6)               // putAlarmTime(6, alarmHour[5], alarmMinute[5])
  //   51 - Будильник, время: суббота : минуты                                                               // getAlarmMinute(6)             // putAlarmTime(6, alarmHour[5], alarmMinute[5])
  //   52 - Будильник, время: воскресенье : часы                                                             // getAlarmHour(7)               // putAlarmTime(7, alarmHour[6], alarmMinute[6])
  //   53 - Будильник, время: воскресенье : минуты                                                           // getAlarmMinute(7)             // putAlarmTime(7, alarmHour[6], alarmMinute[6])
  //  54-63   - имя точки доступа    - 10 байт                                                               // getSoftAPName().toCharArray(apName, 10)       // putSoftAPName(String(apName))       // char apName[11] = ""
  //  64-79   - пароль точки доступа - 16 байт                                                               // getSoftAPPass().toCharArray(apPass, 17)       // putSoftAPPass(String(apPass))       // char apPass[17] = "" 
  //   80 - ширина сегмента матрицы
  //   81 - высота сегмента матрицы
  //   82 - тип сегмента матрицы (параллельная / зигзаг)
  //   83 - угол подключения к сегменту матрицы
  //   84 - направление линейки диодов из угла
  //   85 - количество сегментов по ширине матрицы
  //   86 - количество сегментов по высоте матрицы
  //   87 - тип мета-матрицы (параллельная / зигзаг)
  //   88 - угол подключения к мета-матрице
  //   89 - направление сегментов из угла мета-матрицы
  //   90 - режим работы - Standalone/Master/Slave
  //   91 - тип данных в пакете - Phisic/Logic/Command
  //   92 - группа синхронизации
  //   94 - полная ширина матрицы при использовании карты индексов
  //   95 - полная высота матрицы при использовании карты индексов
  //**96-119  - не используется
  //  120-149 - имя NTP сервера      - 30 байт                                                               // getNtpServer().toCharArray(ntpServerName, 31) // putNtpServer(String(ntpServerName)) // char ntpServerName[31] = ""
  //  150,151 - лимит по току в миллиамперах                                                                 // getPowerLimit()                // putPowerLimit(CURRENT_LIMIT)
  //  152 - globalClockColor.r -  цвет часов в режиме MC_COLOR, режим цвета "Монохром"                       // getGlobalClockColor()          // putGlobalClockColor(globalClockColor)              // uint32_t globalClockColor
  //  153 - globalClockColor.g                                                                               // - " -                          // - " -
  //  154 - globalClockColor.b                                                                               // - " -                          // - " -
  //  155 - globalTextColor.r -  цвет текста в режиме MC_TEXT, режим цвета "Монохром"                        //  getGlobalTextColor()          // putGlobalTextColor(globalTextColor)                // uint32_t globalTextColor 
  //  156 - globalTextColor.g                                                                                // - " -                          // - " -
  //  157 - globalTextColor.b                                                                                // - " -                          // - " -
  //  158 - globalColor.r     -  цвет панели в режиме "лампа"                                                // getGlobalColor()               // putGlobalColor(globalColor)                        // uint32_t globalColor
  //  159 - globalColor.g                                                                                    // - " -                          // - " -
  //  160 - globalColor.b                                                                                    // - " -                          // - " -
  //  161 - Режим 3 по времени - часы                                                                        // getAM3hour()                   // putAM3hour(AM3_hour)
  //  162 - Режим 3 по времени - минуты                                                                      // getAM3minute()                 // putAM3minute(AM3_minute) 
  //  163 - Режим 3 по времени - так же как для режима 1                                                     // getAM3effect()                 // putAM3effect(AM3_effect_id)
  //  164 - Режим 4 по времени - часы                                                                        // getAM4hour()                   // putAM4hour(AM4_hour)
  //  165 - Режим 4 по времени - минуты                                                                      // getAM4minute()                 // putAM4minute(AM4_minute)
  //  166 - Режим 4 по времени - так же как для режима 1                                                     // getAM4effect()                 // putAM4effect(AM4_effect_id)
  //  167,168 - интервал отображения текста бегущей строки                                                   // getTextInterval()              // putTextInterval(TEXT_INTERVAL)
  //  169 - Режим цвета оверлея часов X: 0,1,2,3                                                             // getClockColor()                // putClockColor(COLOR_MODE)
  //  170 - Скорость прокрутки оверлея часов                                                                 // getClockScrollSpeed()          // putClockScrollSpeed(speed_value)  
  //  171 - Режим цвета оверлея текста X: 0,1,2,3                                                            // getTextColor()                 // putTextColor(COLOR_TEXT_MODE)  
  //  172 - Скорость прокрутки оверлея текста                                                                // getTextScrollSpeed()           // putTextScrollSpeed(speed_value)  
  //  173 - Отображать бегущую строку оверлеем в режимах                                                     // getTextOverlayEnabled()        // putTextOverlayEnabled(textOverlayEnabled)
  //  174 - Использовать сервис получения погоды 0- нет, 1 - Yandex; 2 - OpenWeatherMap                      // getUseWeather()                // putUseWeather(useWeather)
  //  175 - Период запроса информации о погоде в минутах                                                     // getWeatherInterval()           // putWeatherInterval(SYNC_WEATHER_PERIOD)
  // 176,177,178,179 - Код региона Yandex для получения погоды (4 байта - uint32_t)                          // getWeatherRegion()             // putWeatherRegion(regionID)
  //  180 - цвет температуры в дневных часах: 0 - цвет часов; 1 - цвет в зависимости от температуры         // getUseTemperatureColor()       // putUseTemperatureColor(useTemperatureColor)
  //  181 - цвет температуры в ночных часах:  0 - цвет часов; 1 - цвет в зависимости от температуры         // getUseTemperatureColorNight()  // putUseTemperatureColorNight(useTemperatureColorNight)
  // 182-206 - MQTT сервер (24 симв)                                                                         // getMqttServer().toCharArray(mqtt_server, 24)  // putMqttServer(String(mqtt_server))       // char mqtt_server[25] = ""
  // 207-221 - MQTT user (14 симв)                                                                           // getMqttUser().toCharArray(mqtt_user, 14)      // putMqttUser(String(mqtt_user))           // char mqtt_user[15] = ""
  // 222-236 - MQTT pwd (14 симв)                                                                            // getMqttPass().toCharArray(mqtt_pass, 14)      // putMqttPass(String(mqtt_pass))           // char mqtt_pass[15] = ""
  // 237,238 - MQTT порт                                                                                     // getMqttPort()                  // putMqttPort(mqtt_port)
  // 239 - использовать MQTT канал управления: 0 - нет 1 - да                                                // getUseMqtt()                   // putUseMqtt(useMQTT)  
  // 240 - яркость ночных часов                                                                              // getNightClockBrightness()      // putNightClockBrightness(nightClockBrightness)
  //**241 - не используется
  //**242 - не используется
  //  243 - Режим по времени "Рассвет" - так же как для режима 1                                             // getAM5effect()                 // putAM5effect(dawn_effect_id)
  // 244,245,246,247 - Код региона OpenWeatherMap для получения погоды (4 байта - uint32_t)                  // getWeatherRegion2()            // putWeatherRegion2(regionID2)
  //  248 - Режим по времени "Закат" - так же как для режима 1                                               // getAM6effect()                 // putAM6effect(dawn_effect_id)
  //**249 - не используется
  // 250-279 - префикс топика сообщения (30 симв)                                                            // getMqttPrefix()                // putMqttPrefix(mqtt_prefix)
  //**280 - не используется
  //  ...
  //**399 - не используется
  //  400 - 400+(Nэфф*5)   - скорость эффекта
  //  401 - 400+(Nэфф*5)+1 - эффект в авторежиме: 1 - использовать; 0 - не использовать
  //  402 - 400+(Nэфф*5)+2 - специальный параметр эффекта #1
  //  403 - 400+(Nэфф*5)+3 - специальный параметр эффекта #2
  //  404 - 400+(Nэфф*5)+4 - контраст эффекта
  //********
  // 1000 - текст строк бегущей строки сплошным массивом, строки разделены символом '\r'                     // loadTexts()                    // saveTexts()
  //********

  // Сначала инициализируем имя сети/точки доступа, пароли и имя NTP-сервера значениями по умолчанию.
  // Ниже, если EEPROM уже инициализирован - из него будут загружены актуальные значения
  strcpy(apName, DEFAULT_AP_NAME);
  strcpy(apPass, DEFAULT_AP_PASS);
  strcpy(ntpServerName, DEFAULT_NTP_SERVER);    
  ssid = NETWORK_SSID;
  pass = NETWORK_PASS;

  #if (USE_MQTT == 1)
  strcpy(mqtt_server, DEFAULT_MQTT_SERVER);
  strcpy(mqtt_user,   DEFAULT_MQTT_USER);
  strcpy(mqtt_pass,   DEFAULT_MQTT_PASS);
  strcpy(mqtt_prefix, DEFAULT_MQTT_PREFIX);
  #endif

  // Инициализировано ли EEPROM
  bool isInitialized = EEPROMread(0) == EEPROM_OK;  
  
  if (isInitialized) {    

    mapWIDTH          = getMatrixMapWidth();
    mapHEIGHT         = getMatrixMapHeight();

    sWIDTH            = getMatrixSegmentWidth();
    sHEIGHT           = getMatrixSegmentHeight();
    sMATRIX_TYPE      = getMatrixSegmentType();
    sCONNECTION_ANGLE = getMatrixSegmentAngle();
    sSTRIP_DIRECTION  = getMatrixSegmentDirection();

    mWIDTH            = getMetaMatrixWidth();
    mHEIGHT           = getMetaMatrixHeight();
    mTYPE             = getMetaMatrixType();
    mANGLE            = getMetaMatrixAngle();
    mDIRECTION        = getMetaMatrixDirection();

    if (sMATRIX_TYPE ==2) {
      pWIDTH          = mapWIDTH;
      pHEIGHT         = mapHEIGHT;
    } else {
      pWIDTH          = sWIDTH * mWIDTH;
      pHEIGHT         = sHEIGHT * mHEIGHT;
    }
    
    NUM_LEDS          = pWIDTH * pHEIGHT;
    maxDim            = max(pWIDTH, pHEIGHT);
    minDim            = min(pWIDTH, pHEIGHT);

    #if (BIG_FONT == 0)
      // Шрифт размером 5x8
      OVERLAY_SIZE = pHEIGHT < 17 ? pWIDTH * pHEIGHT : pWIDTH * 17;
    #elif (BIG_FONT == 1)
      // Шрифт размером 10x16
      OVERLAY_SIZE  =  pWIDTH * 21;                    // высота шрифта 16 + 3 строки диакритич символов над знакоместом и две - под знакоместом
    #else
      // Шрифт размером 8x13
      OVERLAY_SIZE =   pWIDTH * 18;                    // высота шрифта 13 + 3 строки диакритич символов над знакоместом и две - под знакоместом
    #endif
    
    globalBrightness = getMaxBrightness();

    autoplayTime        = getAutoplayTime();
    idleTime            = getIdleTime();

    useNtp              = getUseNtp();
    timeZoneOffset      = getTimeZone();
    clockOverlayEnabled = getClockOverlayEnabled();
    textOverlayEnabled  = getTextOverlayEnabled();

    SYNC_TIME_PERIOD    = getNtpSyncTime();
    manualMode          = getAutoplay();
    CLOCK_ORIENT        = getClockOrientation();
    COLOR_MODE          = getClockColor();
    CLOCK_SIZE          = getClockSize();
    COLOR_TEXT_MODE     = getTextColor();
    CURRENT_LIMIT       = getPowerLimit();
    TEXT_INTERVAL       = getTextInterval();
    
    useRandomSequence   = getRandomMode();
    nightClockColor     = getNightClockColor();
    nightClockBrightness = getNightClockBrightness();
    showDateInClock     = getShowDateInClock();  
    showDateDuration    = getShowDateDuration();
    showDateInterval    = getShowDateInterval();

    alarmWeekDay        = getAlarmWeekDay();
    alarmEffect         = getAlarmEffect();
    alarmDuration       = getAlarmDuration();
    dawnDuration        = getDawnDuration();

    needTurnOffClock    = getTurnOffClockOnLampOff();

    // Загрузить недельные будильники / часы, минуты /
    for (uint8_t i=0; i<7; i++) {
      alarmHour[i] = getAlarmHour(i+1);
      alarmMinute[i] = getAlarmMinute(i+1);
    }
 
    // Загрузить параметры эффектов #1, #2
    for (uint8_t i=0; i<MAX_EFFECT; i++) {
      effectScaleParam[i]  = getScaleForEffect(i); 
      effectScaleParam2[i] = getScaleForEffect2(i);
      effectContrast[i]    = getEffectContrast(i);
      effectSpeed[i]       = getEffectSpeed(i);
    }

    #if (USE_MP3 == 1)
      useAlarmSound     = getUseAlarmSound();
      alarmSound        = getAlarmSound();
      dawnSound         = getDawnSound();
      maxAlarmVolume    = getMaxAlarmVolume();
    #endif

    #if (USE_E131 == 1)
      workMode          = getSyncWorkMode();
      syncMode          = getSyncDataMode();
      syncGroup         = getSyncGroup();;    
    #endif
    
    globalColor         = getGlobalColor();         // цвет лампы, задаваемый пользователем
    globalClockColor    = getGlobalClockColor();    // цвет часов в режиме MC_COLOR, режим цвета "Монохром"
    globalTextColor     = getGlobalTextColor();     // цвет часов бегущей строки в режиме цвета "Монохром"

    useSoftAP = getUseSoftAP();
    getSoftAPName().toCharArray(apName, 10);        //  54-63   - имя точки доступа    ( 9 байт макс) + 1 байт '\0'
    getSoftAPPass().toCharArray(apPass, 17);        //  64-79   - пароль точки доступа (16 байт макс) + 1 байт '\0'
    getNtpServer().toCharArray(ntpServerName, 31);  //  120-149 - имя NTP сервера      (30 байт макс) + 1 байт '\0'

    ssid = getSsid();                               //          - имя сети WiFi
    pass = getPass();                               //          - пароль сети WiFi
    
    if (strlen(apName) == 0) strcpy(apName, DEFAULT_AP_NAME);
    if (strlen(apPass) == 0) strcpy(apPass, DEFAULT_AP_PASS);
    if (strlen(ntpServerName) == 0) strcpy(ntpServerName, DEFAULT_NTP_SERVER);

    #if (USE_MQTT == 1)
    useMQTT = getUseMqtt();
    getMqttServer().toCharArray(mqtt_server, 25);   //  182-206 - mqtt сервер          (24 байт макс) + 1 байт '\0'
    getMqttUser().toCharArray(mqtt_user, 15);       //  207-221 - mqtt user            (14 байт макс) + 1 байт '\0'
    getMqttPass().toCharArray(mqtt_pass, 15);       //  222-236 - mqtt password        (14 байт макс) + 1 байт '\0'
    getMqttPrefix().toCharArray(mqtt_prefix, 31);   //  250-279 - mqtt password        (30 байт макс) + 1 байт '\0'
    if (strlen(mqtt_server) == 0) strcpy(mqtt_server, DEFAULT_MQTT_SERVER);
    if (strlen(mqtt_user)   == 0) strcpy(mqtt_user,   DEFAULT_MQTT_USER);
    if (strlen(mqtt_pass)   == 0) strcpy(mqtt_pass,   DEFAULT_MQTT_PASS);
    if (strlen(mqtt_prefix) == 0) strcpy(mqtt_prefix, DEFAULT_MQTT_PREFIX);
    mqtt_port = getMqttPort();
    #endif

    AM1_hour       = getAM1hour();
    AM1_minute     = getAM1minute();
    AM1_effect_id  = getAM1effect();
    AM2_hour       = getAM2hour();
    AM2_minute     = getAM2minute();
    AM2_effect_id  = getAM2effect();
    AM3_hour       = getAM3hour();
    AM3_minute     = getAM3minute();
    AM3_effect_id  = getAM3effect();
    AM4_hour       = getAM4hour();
    AM4_minute     = getAM4minute();
    AM4_effect_id  = getAM4effect();
    dawn_effect_id = getAM5effect();
    dusk_effect_id = getAM6effect();

  #if (USE_WEATHER == 1)     
    useWeather          = getUseWeather();
    regionID            = getWeatherRegion();
    regionID2           = getWeatherRegion2();
    SYNC_WEATHER_PERIOD = getWeatherInterval();
    useTemperatureColor = getUseTemperatureColor();
    useTemperatureColorNight = getUseTemperatureColorNight();
    showWeatherInClock  = getShowWeatherInClock();
  #endif  

    loadTexts();
    getStaticIP();
    
  } else {

    DEBUGLN(F("Инициализация EEPROM..."));

    // Значения переменных по умолчанию определяются в месте их объявления - в файле a_def_soft.h
    // Здесь выполняются только инициализация массивов и некоторых специальных параметров
    clearEEPROM();

    for (uint8_t i=0; i<MAX_EFFECT; i++) {
      effectScaleParam[i]  = 50;             // среднее значение для параметра. Конкретное значение зависит от эффекта
      effectScaleParam2[i] = 0;              // второй параметр эффекта по умолчанию равен 0. Конкретное значение зависит от эффекта
      effectContrast[i]    = 128;            // контраст эффекта
      effectSpeed[i]       = D_EFFECT_SPEED; // скорость эффекта
    }

    // Значения текстовых строк по умолчанию - 
    textLines[0] = "##";
    for (uint8_t i = 1; i<36; i++) textLines[i] = "";

    // После первой инициализации значений - сохранить их принудительно
    saveDefaults();
    saveSettings();
    
    DEBUGLN();
  }  

  #if (USE_MQTT == 1) 
  changed_keys = "";
  #endif
}

void clearEEPROM() {
  for (uint16_t addr = 1; addr < EEPROM_MAX; addr++) {
    EEPROM.write(addr, 0);
  }
}

void saveDefaults() {

  putMatrixMapWidth(mapWIDTH);
  putMatrixMapHeight(mapHEIGHT);
  
  putMatrixSegmentWidth(sWIDTH);
  putMatrixSegmentHeight(sHEIGHT);
  putMatrixSegmentType(sMATRIX_TYPE);
  putMatrixSegmentAngle(sCONNECTION_ANGLE);
  putMatrixSegmentDirection(sSTRIP_DIRECTION);

  putMetaMatrixWidth(mWIDTH);
  putMetaMatrixHeight(mHEIGHT);
  putMetaMatrixType(mTYPE);
  putMetaMatrixAngle(mANGLE);
  putMetaMatrixDirection(mDIRECTION);

  putMaxBrightness(globalBrightness);

  putAutoplayTime(autoplayTime / 1000L);
  putIdleTime(constrain(idleTime / 60L / 1000L, 0, 255));

  putUseNtp(useNtp);
  putTimeZone(timeZoneOffset);
  putClockOverlayEnabled(clockOverlayEnabled);
  putTextOverlayEnabled(textOverlayEnabled);

  putNtpSyncTime(SYNC_TIME_PERIOD);
  putAutoplay(manualMode);

  putClockOrientation(CLOCK_ORIENT);
  putPowerLimit(CURRENT_LIMIT);
  putTextInterval(TEXT_INTERVAL);
  
  putRandomMode(useRandomSequence);
  putNightClockColor(nightClockColor);  // Цвет ночных часов: 0 - R; 1 - G; 2 - B; 3 - C; 4 - M; 5 - Y;
  putNightClockBrightness(nightClockBrightness);
  putShowDateInClock(showDateInClock);
  putShowDateDuration(showDateDuration);
  putShowDateInterval(showDateInterval);
  putTurnOffClockOnLampOff(needTurnOffClock);

  putAlarmParams(alarmWeekDay,dawnDuration,alarmEffect,alarmDuration);
  #if (USE_MP3 == 1)
    putAlarmSounds(useAlarmSound, maxAlarmVolume, alarmSound, dawnSound);
  #endif

  for (uint8_t i=0; i<7; i++) {
    putAlarmTime(i+1, alarmHour[i], alarmMinute[i]);
  }

  // Настройки по умолчанию для эффектов
  for (uint8_t i = 0; i < MAX_EFFECT; i++) {
    putEffectParams(i, effectSpeed[i], true, true, true, effectScaleParam[i], effectScaleParam2[i], effectContrast[i]);
  }

  // Специальные настройки отдельных эффектов
  putEffectUsage(MC_CLOCK, false);
  putClockScrollSpeed(250);  
  putEffectTextOverlayUsage(MC_CLOCK, false); 
  putEffectTextOverlayUsage(MC_MAZE, false);
  putEffectTextOverlayUsage(MC_SNAKE, false);
  putEffectTextOverlayUsage(MC_TETRIS, false);
  putEffectTextOverlayUsage(MC_ARKANOID, false);
  putEffectTextOverlayUsage(MC_WEATHER, false);
  putEffectTextOverlayUsage(MC_LIFE, false);
  putEffectTextOverlayUsage(MC_IMAGE, false);
  putEffectTextOverlayUsage(MC_SLIDE, false);
  putEffectClockOverlayUsage(MC_CLOCK, false);
  putEffectClockOverlayUsage(MC_MAZE, false);
  putEffectClockOverlayUsage(MC_SNAKE, false);
  putEffectClockOverlayUsage(MC_TETRIS, false);
  putEffectClockOverlayUsage(MC_ARKANOID, false);
  putEffectClockOverlayUsage(MC_WEATHER, false);
  putEffectClockOverlayUsage(MC_LIFE, false);
  putEffectClockOverlayUsage(MC_IMAGE, false);
  putEffectClockOverlayUsage(MC_SLIDE, false);

  putClockScrollSpeed(250);
  putTextScrollSpeed(186);
  
  putScaleForEffect(MC_FIRE, 0);            // Огонь красного цвета
  putScaleForEffect2(MC_PAINTBALL, 1);      // Использовать сегменты для эффекта Пэйнтбол на широких матрицах
  putScaleForEffect2(MC_SWIRL, 1);          // Использовать сегменты для эффекта Водоворот на широких матрицах
  putScaleForEffect2(MC_RAINBOW, 0);        // Использовать рандомный выбор эффекта радуга 0 - random; 1 - диагональная; 2 - горизонтальная; 3 - вертикальная; 4 - вращающаяся  

  uint8_t ball_size = minDim / 4;
  if (ball_size > 5) ball_size = 5;
  putScaleForEffect(MC_BALL, ball_size);    // Размер кубика по умолчанию
  
  putGlobalColor(globalColor);              // Цвет панели в режиме "Лампа"
  putGlobalClockColor(globalClockColor);    // Цвет часов в режиме "Монохром" 
  putGlobalTextColor(globalTextColor);      // Цвет текста в режиме "Монохром"

  putUseSoftAP(useSoftAP);

  strcpy(apName, DEFAULT_AP_NAME);
  strcpy(apPass, DEFAULT_AP_PASS);
  ssid = NETWORK_SSID;
  pass = NETWORK_PASS;

  #if (USE_MQTT == 1)
  strcpy(mqtt_server, DEFAULT_MQTT_SERVER);
  strcpy(mqtt_user, DEFAULT_MQTT_USER);
  strcpy(mqtt_pass, DEFAULT_MQTT_PASS);
  #endif  

  #if (USE_E131 == 1)
    workMode = STANDALONE;      // По умолчанию - самостоятельный режим работы
    syncMode = LOGIC;           // По умолчанию - размещение данных в логическом порядке - левый верхний угол, далее вправо и вниз.
    syncGroup = 0;
    putSyncWorkMode(workMode);
    putSyncDataMode(syncMode);
    putSyncGroup(syncGroup);
  #endif

  putSoftAPName(String(apName));
  putSoftAPPass(String(apPass));
  putSsid(ssid);
  putPass(pass);

  #if (USE_MQTT == 1)
  putMqttServer(String(mqtt_server));
  putMqttUser(String(mqtt_user));
  putMqttPass(String(mqtt_pass));
  putMqttPrefix(String(mqtt_prefix));
  putMqttPort(mqtt_port);
  putUseMqtt(useMQTT);
  #endif

  strcpy(ntpServerName, DEFAULT_NTP_SERVER);
  putNtpServer(String(ntpServerName));

  putAM1hour(AM1_hour);                 // Режим 1 по времени - часы
  putAM1minute(AM1_minute);             // Режим 1 по времени - минуты
  putAM1effect(AM1_effect_id);          // Режим 1 по времени - действие: -3 - выключено (не используется); -2 - выключить матрицу (черный экран); -1 - огонь, 0 - случайный, 1 и далее - эффект EFFECT_LIST
  putAM2hour(AM2_hour);                 // Режим 2 по времени - часы
  putAM2minute(AM2_minute);             // Режим 2 по времени - минуты
  putAM2effect(AM2_effect_id);          // Режим 2 по времени - действие: -3 - выключено (не используется); -2 - выключить матрицу (черный экран); -1 - огонь, 0 - случайный, 1 и далее - эффект EFFECT_LIST
  putAM3hour(AM3_hour);                 // Режим 3 по времени - часы
  putAM3minute(AM3_minute);             // Режим 3 по времени - минуты
  putAM3effect(AM3_effect_id);          // Режим 3 по времени - действие: -3 - выключено (не используется); -2 - выключить матрицу (черный экран); -1 - огонь, 0 - случайный, 1 и далее - эффект EFFECT_LIST
  putAM4hour(AM4_hour);                 // Режим 4 по времени - часы
  putAM4minute(AM4_minute);             // Режим 4 по времени - минуты
  putAM4effect(AM4_effect_id);          // Режим 4 по времени - действие: -3 - выключено (не используется); -2 - выключить матрицу (черный экран); -1 - огонь, 0 - случайный, 1 и далее - эффект EFFECT_LIST
  putAM5effect(dawn_effect_id);         // Режим по времени "Рассвет" - действие: -3 - выключено (не используется); -2 - выключить матрицу (черный экран); -1 - огонь, 0 - случайный, 1 и далее - эффект EFFECT_LIST
  putAM6effect(dusk_effect_id);         // Режим по времени "Закат"   - действие: -3 - выключено (не используется); -2 - выключить матрицу (черный экран); -1 - огонь, 0 - случайный, 1 и далее - эффект EFFECT_LIST

#if (USE_WEATHER == 1)       
  putUseWeather(useWeather);
  putWeatherRegion(regionID);
  putWeatherRegion2(regionID2);
  putWeatherInterval(SYNC_WEATHER_PERIOD);
  putUseTemperatureColor(useTemperatureColor);
  putShowWeatherInClock(showWeatherInClock);
#endif
       
  putStaticIP(IP_STA[0], IP_STA[1], IP_STA[2], IP_STA[3]);

#if (INITIALIZE_TEXTS == 1)
  textLines[0]   = "##";
  textLines[1]   = "Всё будет хорошо!";
  textLines[2]   = "До {C#00D0FF}Нового года {C#FFFFFF}осталось {C#10FF00}{R01.01.***+}{S01.12.****#31.12.**** 23:59:59}{E21}";
  textLines[3]   = "До {C#0019FF}Нового года{C#FFFFFF} {P01.01.****#4}";
  textLines[4]   = "С {C#00D0FF}Новым {C#0BFF00}{D:yyyy} {C#FFFFFF}годом!{S01.01.****#31.01.**** 23:59:59}{E21}";
  textLines[5]   = "В {C#10FF00}Красноярске {C#FFFFFF}{WS} {WT}°C";
  textLines[6]   = "Show must go on!{C#000002}";
  textLines[7]   = "{C#FF000F}Крибле! {C#000001}Крабле!! {C#00FF00}Бумс!!!{E24}";
  textLines[8]   = "Крепитесь, люди - скоро {C#FF0300}лето!{S01.01.****#10.04.****}";
  textLines[9]   = "Это {C#0081FF}\"ж-ж-ж\"{C#FFFFFF} - неспроста!";
  textLines[10]  = "Элементарно, Ватсон!";
  textLines[11]  = "Дело было вечером, делать было нечего...";
  textLines[12]  = "Ах, какая прелесть!";
  textLines[13]  = "Это нужно обдумать...";
  textLines[14]  = "В этом что-то есть...";
  textLines[15]  = "В {C#10FF00}Красноярске {C#FFFFFF}{WS} {WT}°C";
  textLines[16]  = "Вот оно что, {C#FF0000}Михалыч{C#FFFFFF}!..";
  textLines[17]  = "Сегодня {D:dd MMMM yyyy} года, {D:dddd}";
  textLines[18]  = "Лень - двигатель прогресса";
  textLines[19]  = "Чем бы дитя не тешилось...";
  textLines[20]  = "С Рождеством!{S07.01.****}";
  textLines[21]  = "С днем Победы!{S09.05.**** 7:00#09.05.**** 21:30}";
  textLines[22]  = "Сегодня {D:dddd dd MMMM}, на улице {WS}, {WT}°C{E25}";
  textLines[23]  = "С праздником, соседи!";
  textLines[24]  = "Не скучайте!";
  textLines[25]  = "Счастья всем! И пусть никто не уйдет обиженным.";
  textLines[26]  = "В отсутствии солнца сияйте сами!";
  textLines[27]  = "Почему? Потому!";
  textLines[28]  = "Время принимать решения!";
  textLines[29]  = "Время делать выводы!";
  textLines[30]  = "Лето, ах лето!{S15.05.****#01.09.****}";
  textLines[31]  = "Нет предела совершенству!";
  textLines[32]  = "Чего изволите, ваша светлость?";
  textLines[33]  = "Курочка по зёрнышку, копеечка к копеечке!";  
  textLines[34]  = "Подъем через {P7:30#Z#60#60#12345}!";
  textLines[35]  = "-Доброе утро!";
#endif

  saveTexts();

  putCurrentSpecMode(-1);               // Текущий спец-режим - это не спец-режим
  putCurrentManualMode(-1);             // Текущий вручную включенный режим
}

void saveSettings() {

  saveSettingsTimer.reset();
  if (!eepromModified) return;
  
  // Поставить отметку, что EEPROM инициализировано параметрами эффектов
  EEPROMwrite(0, EEPROM_OK);
  
  EEPROM.commit();
  DEBUGLN(F("Настройки сохранены в EEPROM"));
  eepromModified = false;
}

void putMatrixMapWidth(uint8_t width) {
  if (width < 8) width = 8;
  if (width > 127) width = 127;
  if (width != getMatrixMapWidth()) {
    EEPROMwrite(94, width);
  }  
}
uint8_t getMatrixMapWidth() {
  uint8_t value = EEPROMread(94);
  if (value < 8) value = 8;
  if (value > 127) value = 127;  
  return value;
}

void putMatrixMapHeight(uint8_t height) {
  if (height < 8) height = 8;
  if (height > 127) height = 127;  
  if (height != getMatrixMapHeight()) {
    EEPROMwrite(95, height);
  }  
}
uint8_t getMatrixMapHeight() {
  uint8_t value = EEPROMread(95);
  if (value < 8) value = 8;
  if (value > 127) value = 127;  
  return value;
}

void putMatrixSegmentWidth(uint8_t width) {
  if (width == 0 && width > 127) width = 16;
  if (width != getMatrixSegmentWidth()) {
    EEPROMwrite(80, width);
  }  
}
uint8_t getMatrixSegmentWidth() {
  uint8_t value = EEPROMread(80);
  if (value == 0 || value > 127) value = 16;
  return value;
}

void putMatrixSegmentHeight(uint8_t height) {
  if (height == 0 && height > 127) height = 16;
  if (height > 0 && height < 128 && height != getMatrixSegmentHeight()) {
    EEPROMwrite(81, height);
  }  
}
uint8_t getMatrixSegmentHeight() {
  uint8_t value = EEPROMread(81);
  if (value == 0 || value > 127) value = 16;
  return value;
}

void putMatrixSegmentType(uint8_t type) {
  if (type > 2) type = 0;
  if (type != getMatrixSegmentType()) {
    EEPROMwrite(82, type);
  }  
}
uint8_t getMatrixSegmentType() {
  uint8_t value = EEPROMread(82);
  if (value > 2) value = 0;
  return value;  
}

void putMatrixSegmentAngle(uint8_t angle) {
  if (angle > 3) angle = 0;
  if (angle != getMatrixSegmentAngle()) {
    EEPROMwrite(83, angle);
  }  
}
uint8_t getMatrixSegmentAngle() {
  uint8_t value = EEPROMread(83);
  if (value > 3) value = 0;
  return value;  
}

void putMatrixSegmentDirection(uint8_t dir) {
  if (dir > 3) dir = 0;
  if (dir != getMatrixSegmentDirection()) {
    EEPROMwrite(84, dir);
  }  
}
uint8_t getMatrixSegmentDirection() {
  uint8_t value = EEPROMread(84);
  if (value > 3) value = 0;
  return value;  
}

void putMetaMatrixWidth(uint8_t width) {
  if (width == 0 && width > 15) width = 1;
  if (width != getMetaMatrixWidth()) {
    EEPROMwrite(85, width);
  }  
}
uint8_t getMetaMatrixWidth() {
  uint8_t value = EEPROMread(85);
  if (value == 0 || value > 15) value = 1;
  return value;
}

void putMetaMatrixHeight(uint8_t height) {
  if (height == 0 && height > 15) height = 1;
  if (height != getMetaMatrixHeight()) {
    EEPROMwrite(86, height);
  }  
}
uint8_t getMetaMatrixHeight() {
  uint8_t value = EEPROMread(86);
  if (value == 0 || value > 15) value = 1;
  return value;
}

void putMetaMatrixType(uint8_t type) {
  if (type > 1) type = 0;
  if (type != getMetaMatrixType()) {
    EEPROMwrite(87, type);
  }  
}
uint8_t getMetaMatrixType() {
  uint8_t value = EEPROMread(87);
  if (value > 1) value = 0;
  return value;
}

void putMetaMatrixAngle(uint8_t angle) {
  if (angle > 3) angle = 0;
  if (angle != getMetaMatrixAngle()) {
    EEPROMwrite(88, angle);
  }  
}
uint8_t getMetaMatrixAngle() {
  uint8_t value = EEPROMread(88);
  if (value > 3) value = 0;
  return value;
}

void putMetaMatrixDirection(uint8_t dir) {
  if (dir > 3) dir = 0;
  if (dir != getMetaMatrixDirection()) {
    EEPROMwrite(89, dir);
  }  
}
uint8_t getMetaMatrixDirection() {
  uint8_t value = EEPROMread(89);
  if (value > 3) value = 0;
  return value;
}

void putEffectParams(uint8_t effect, uint8_t spd, bool use, bool use_text_overlay, bool use_clock_overlay, uint8_t value1, uint8_t value2, uint8_t contrast) {
  uint8_t value = 0;

  if (use)               value |= 0x01;
  if (use_text_overlay)  value |= 0x02;
  if (use_clock_overlay) value |= 0x04;
  
  EEPROMwrite(EFFECT_EEPROM + effect*5, constrain(map(spd, D_EFFECT_SPEED_MIN, D_EFFECT_SPEED_MAX, 0, 255), 0, 255));        // Скорость эффекта  
  EEPROMwrite(EFFECT_EEPROM + effect*5 + 1, value);                                                                            // b0 - Вкл/Выкл эффект в демо-режиме; b1 - Вкл/выкл оверлей текста; ; b1 - Вкл/выкл оверлей часов   
  EEPROMwrite(EFFECT_EEPROM + effect*5 + 2, value1);                                                                           // Параметр эффекта #1 
  EEPROMwrite(EFFECT_EEPROM + effect*5 + 3, value2);                                                                           // Параметр эффекта #2 
  EEPROMwrite(EFFECT_EEPROM + effect*5 + 4, contrast);                                                                         // Контраст эффекта 
  effectScaleParam[effect] = value1;
  effectScaleParam2[effect] = value2;
  effectContrast[effect] = contrast;
  effectSpeed[effect] = spd;
}

bool getEffectUsage(uint8_t effect) {
  if (effect >= MAX_EFFECT) return false;
  uint8_t value = EEPROMread(EFFECT_EEPROM + effect*5 + 1);
  return (value & 0x01) != 0;                                  // b0 - использовать эффект в демо-режиме
}

void putEffectUsage(uint8_t effect, bool use) {
  uint8_t value = EEPROMread(EFFECT_EEPROM + effect*5 + 1);
  uint8_t new_value = use ? (value | 0x01) : (value & ~0x01);
  if (value != new_value) {
    EEPROMwrite(EFFECT_EEPROM + effect*5 + 1, new_value);      // b0 - использовать эффект в демо-режиме
  }
}

bool getEffectTextOverlayUsage(uint8_t effect) {
  uint8_t value = EEPROMread(EFFECT_EEPROM + effect*5 + 1);
  return (value & 0x02) != 0;                                  // b1 - использовать в эффекте бегущую строку поверх эффекта
}

void putEffectTextOverlayUsage(uint8_t effect, bool use) {
  uint8_t value = EEPROMread(EFFECT_EEPROM + effect*5 + 1);
  uint8_t new_value = use ? (value | 0x02) : (value & ~0x02);
  if (value != new_value) {
    EEPROMwrite(EFFECT_EEPROM + effect*5 + 1, new_value);               // b1 - использовать в эффекте бегущую строку поверх эффекта
  }
}

bool getEffectClockOverlayUsage(uint8_t effect) {
  uint8_t value = EEPROMread(EFFECT_EEPROM + effect*5 + 1);
  return (value & 0x04) != 0;                                  // b2 - использовать в эффекте часы поверх эффекта
}

void putEffectClockOverlayUsage(uint8_t effect, bool use) {
  uint8_t value = EEPROMread(EFFECT_EEPROM + effect*5 + 1);
  uint8_t new_value = use ? (value | 0x04) : (value & ~0x04);
  if (value != new_value) {
    EEPROMwrite(EFFECT_EEPROM + effect*5 + 1, new_value);               // b2 - использовать в эффекте часы поверх эффекта
  }
}

void putScaleForEffect(uint8_t effect, uint8_t value) {
  if (value != getScaleForEffect(effect)) {
    EEPROMwrite(EFFECT_EEPROM + effect*5 + 2, value);
    effectScaleParam[effect] = value;
  }  
}

uint8_t getScaleForEffect(uint8_t effect) {
  uint8_t value = EEPROMread(EFFECT_EEPROM + effect*5 + 2);
  effectScaleParam[effect] = value;
  return value;
}

void putScaleForEffect2(uint8_t effect, uint8_t value) {
  if (value != getScaleForEffect2(effect)) {
    EEPROMwrite(EFFECT_EEPROM + effect*5 + 3, value);
    effectScaleParam2[effect] = value;
  }  
}

uint8_t getScaleForEffect2(uint8_t effect) {
  uint8_t value = EEPROMread(EFFECT_EEPROM + effect*5 + 3);
  effectScaleParam2[effect] = value;
  return value;
}

uint8_t getEffectContrast(uint8_t effect) {
  uint8_t contrast = constrain(EEPROMread(EFFECT_EEPROM + effect*5 + 4),10,255);
  effectContrast[effect] = contrast;
  return contrast;
}

void putEffectContrast(uint8_t effect, uint8_t contrast) {
  if (contrast != getEffectContrast(effect)) {
    effectContrast[effect] = constrain(contrast,10,255);
    EEPROMwrite(EFFECT_EEPROM + effect*5 + 4, effectContrast[effect]);
  }  
}

void putEffectSpeed(uint8_t effect, uint8_t speed) {
  if (speed != getEffectSpeed(effect)) {
    effectSpeed[effect] = constrain(map(speed, D_EFFECT_SPEED_MIN, D_EFFECT_SPEED_MAX, 0, 255), 0, 255);
    EEPROMwrite(EFFECT_EEPROM + effect*5, effectSpeed[effect]);        // Скорость эффекта    
  }
}

uint8_t getEffectSpeed(uint8_t effect) {
  uint8_t speed = map8(EEPROMread(EFFECT_EEPROM + effect*5),D_EFFECT_SPEED_MIN,D_EFFECT_SPEED_MAX);
  effectSpeed[effect] = speed;
  return speed; 
}

uint8_t getMaxBrightness() {
  return EEPROMread(1);
}

void putMaxBrightness(uint8_t brightness) {
  if (brightness != getMaxBrightness()) {
    EEPROMwrite(1, brightness);
  }
}

void putAutoplay(bool value) {
  if (value != getAutoplay()) {
    EEPROMwrite(2, value ? 1 : 0);
  }  
}

bool getAutoplay() {
  return EEPROMread(2) != 0;
}

void putAutoplayTime(uint32_t value) {
  if (value != getAutoplayTime()) {
    EEPROMwrite(3, constrain(value / 1000UL, 0, 255));
  }
}

uint32_t getAutoplayTime() {
  uint32_t time = EEPROMread(3) * 1000UL;  
  if (time == 0) time = ((uint32_t)AUTOPLAY_PERIOD * 1000UL);
  return time;
}

void putIdleTime(uint32_t value) {
  if (value != getIdleTime()) {
    EEPROMwrite(4, constrain(value / 60 / 1000UL, 0, 255));
  }
}

uint16_t getUpTimeSendInterval() {
  return EEPROM_int_read(280);
}

void putUpTimeSendInterval(uint16_t value) {
  if (value != getUpTimeSendInterval()) {
    EEPROM_int_write(280, value);
  }
}

uint32_t getIdleTime() {
  uint32_t time = EEPROMread(4) * 60 * 1000UL;  
  return time;
}

bool getClockOverlayEnabled() {
  return EEPROMread(30) != 0;
}

void putClockOverlayEnabled(bool enabled) {
  if (enabled != getClockOverlayEnabled()) {
    EEPROMwrite(30, enabled ? 1 : 0);
  }
}

bool getTextOverlayEnabled() {
  return (EEPROMread(173) != 0);
}

void putTextOverlayEnabled(bool enabled) {
  if (enabled != getTextOverlayEnabled()) {
    EEPROMwrite(173, enabled ? 1 : 0);
  }
}

void putUseNtp(bool value) {
  if (value != getUseNtp()) {
    EEPROMwrite(5, value);
  }
}

bool getUseNtp() {
  return EEPROMread(5) != 0;
}

void putNtpSyncTime(uint16_t value) {
  if (value != getNtpSyncTime()) {
    EEPROM_int_write(6, value);
  }
}

uint16_t getNtpSyncTime() {
  uint16_t time = EEPROM_int_read(6);  
  if (time == 0) time = 60;
  return time;
}

void putTimeZone(int8_t value) {
  if (value != getTimeZone()) {
    EEPROMwrite(8, (uint8_t)value);
  }
}

int8_t getTimeZone() {
  return (int8_t)EEPROMread(8);
}

bool getTurnOffClockOnLampOff() {
  return EEPROMread(9) != 0;
}

void putTurnOffClockOnLampOff(bool flag) {
  if (flag != getTurnOffClockOnLampOff()) {
    EEPROMwrite(9, flag ? 1 : 0);
  }  
}

uint8_t getClockOrientation() {  
  uint8_t val = EEPROMread(15) != 0 ? 1 : 0;
  if (val == 0 && !allowHorizontal) val = 1;
  if (val == 1 && !allowVertical) val = 0;
  return val;
}

void putClockOrientation(uint8_t orientation) {
  if (orientation != getClockOrientation()) {
    EEPROMwrite(15, orientation != 0 ? 1 : 0);
  }
}

bool getShowWeatherInClock() {
  bool val = EEPROMread(32) != 0;
  return val;
}

void putShowWeatherInClock(bool use) {  
  if (use != getShowWeatherInClock()) {
    EEPROMwrite(32, use ? 1 : 0);
  }
}

bool getShowDateInClock() {
  bool val = EEPROMread(16) != 0;
  return val;
}

void putShowDateInClock(bool use) {  
  if (use != getShowDateInClock()) {
    EEPROMwrite(16, use ? 1 : 0);
  }
}

uint8_t getShowDateDuration() {
  return EEPROMread(17);
}

void putShowDateDuration(uint8_t Duration) {
  if (Duration != getShowDateDuration()) {
    EEPROMwrite(17, Duration);
  }
}

uint8_t getShowDateInterval() {
  return EEPROMread(18);
}

void putShowDateInterval(uint8_t Interval) {
  if (Interval != getShowDateInterval()) {
    EEPROMwrite(18, Interval);
  }
}

uint8_t getClockSize() {
  return EEPROMread(19);
}

void putClockSize(uint8_t c_size) {
  if (c_size != getClockSize()) {
    EEPROMwrite(19, c_size);
  }
}

void putAlarmParams(uint8_t alarmWeekDay, uint8_t dawnDuration, uint8_t alarmEffect, uint8_t alarmDuration) {  
  if (alarmWeekDay != getAlarmWeekDay()) {
    EEPROMwrite(20, alarmWeekDay);
  }
  if (dawnDuration != getDawnDuration()) {
    EEPROMwrite(21, dawnDuration);
  }
  if (alarmEffect != getAlarmEffect()) {
    EEPROMwrite(22, alarmEffect);
  }
  //   24 - Будильник, длительность звука будильника, минут
  if (alarmDuration != getAlarmDuration()) {
    EEPROMwrite(24, alarmDuration);
  }
}

uint8_t getAlarmHour(uint8_t day) { 
  return constrain(EEPROMread(40 + 2 * (day - 1)), 0, 23);
}

uint8_t getAlarmMinute(uint8_t day) { 
  return constrain(EEPROMread(40 + 2 * (day - 1) + 1), 0, 59);
}

void putAlarmTime(uint8_t day, uint8_t hour, uint8_t minute) { 
  if (hour != getAlarmHour(day)) {
    EEPROMwrite(40 + 2 * (day - 1), constrain(hour, 0, 23));
  }
  if (minute != getAlarmMinute(day)) {
    EEPROMwrite(40 + 2 * (day - 1) + 1, constrain(minute, 0, 59));
  }
}

uint8_t getAlarmWeekDay() { 
  return EEPROMread(20);
}

uint8_t getDawnDuration() { 
  return constrain(EEPROMread(21),1,59);
}

void putAlarmSounds(bool useSound, uint8_t maxVolume, int8_t alarmSound, int8_t dawnSound) {
  //   23 - Будильник звук: вкл/выкл 1 - вкл; 0 -выкл
  //   25 - Будильник, мелодия будильника
  //   26 - Будильник, мелодия рассвета
  //   27 - Будильник, максимальная громкость
  if (alarmEffect != getAlarmEffect()) {
    EEPROMwrite(22, alarmEffect);
  }
  if (useSound != getUseAlarmSound()) {
    EEPROMwrite(23, useSound ? 1 : 0);
  }
  if (alarmSound != getAlarmSound()) {
    EEPROMwrite(25, (uint8_t)alarmSound);
  }
  if (dawnSound != getDawnSound()) {
    EEPROMwrite(26, (uint8_t)dawnSound);
  }
  if (maxVolume != getMaxAlarmVolume()) {
    EEPROMwrite(27, maxVolume);
  }
}

uint8_t getAlarmEffect() { 
  return EEPROMread(22);
}

bool getUseAlarmSound() { 
  return EEPROMread(23) != 0;
}

uint8_t getAlarmDuration() { 
  return constrain(EEPROMread(24),1,10);
}

uint8_t getMaxAlarmVolume() { 
  return constrain(EEPROMread(27),0,30);
}

int8_t getAlarmSound() { 
  return constrain((int8_t)EEPROMread(25),-1,127);
}

int8_t getDawnSound() { 
  return constrain((int8_t)EEPROMread(26),-1,127);
}

bool getUseSoftAP() {
  return EEPROMread(14) != 0;
}

void putUseSoftAP(bool use) {  
  if (use != getUseSoftAP()) {
    EEPROMwrite(14, use ? 1 : 0);
  }
}

String getSoftAPName() {
  return EEPROM_string_read(54, 10);
}

void putSoftAPName(String SoftAPName) {
  if (SoftAPName != getSoftAPName()) {
    EEPROM_string_write(54, SoftAPName, 10);
  }
}

String getSoftAPPass() {
  return EEPROM_string_read(64, 16);
}

void putSoftAPPass(String SoftAPPass) {
  if (SoftAPPass != getSoftAPPass()) {
    EEPROM_string_write(64, SoftAPPass, 16);
  }
}

// ssid – символьная строка, содержащая SSID точки доступа, к которой мы хотим подключиться (может содержать не более 32 символов).
String getSsid() {
  String ssid = "";
  File file;
  bool ok = true;
  #if defined(ESP32)
    String fileName = "/ssid";
  #else
    String fileName = "ssid";
  #endif  
  file = LittleFS.open(fileName, "r");
  if (file) {
    // считываем содержимое файла ssid
    char buf[33];
    memset(buf, '\0', 33);
    size_t len = file.read((uint8_t*)buf, 33);
    ok = len > 0;    
    file.close();
    if (ok) ssid = String(buf);
  } else {
    DEBUGLN(String(F("Нет конфигурации сети: SSID не задан")));    
  }
  return ssid;
}

bool putSsid(String Ssid) {
  File file;
  bool ok = true;
  #if defined(ESP32)
    String fileName = "/ssid";
  #else
    String fileName = "ssid";
  #endif  
  if (LittleFS.exists(fileName)) {
    ok = LittleFS.remove(fileName);
  }
  if (ok) {
    file = LittleFS.open(fileName, "w");
    if (file) {
      size_t len = Ssid.length()+1, lenw = 0;
      if (len > 32) len = 32;
      char buf[33];
      memset(buf, '\0', 33);
      Ssid.toCharArray(buf, len);
      lenw = file.write((uint8_t*)buf, len);
      ok = lenw == len;       
      file.close();
    } else {
      ok = false;
    }
  }
  if (!ok) {
    DEBUGLN(String(F("Ошибка сохранения SSID")));
  } 
  return ok; 
}

// password – это пароль к точке доступа в виде символьной строки, которая может содержать от 8 до 64 символов
String getPass() {
  String pass = "";
  File file;
  bool ok = true;
  #if defined(ESP32)
    String fileName = "/pass";
  #else
    String fileName = "pass";
  #endif  
  file = LittleFS.open(fileName, "r");
  if (file) {
    // считываем содержимое файла pass
    char buf[65];
    memset(buf, '\0', 65);
    size_t len = file.read((uint8_t*)buf, 65);
    ok = len > 0; 
    file.close();
    if (ok) pass = String(buf);
  } else {
    DEBUGLN(String(F("Нет конфигурации сети: пароль не задан")));    
  }
  return pass;
}

bool putPass(String Pass) {
  File file;
  bool ok = true;
  #if defined(ESP32)
    String fileName = "/pass";
  #else
    String fileName = "pass";
  #endif  
  if (LittleFS.exists(fileName)) {
    ok = LittleFS.remove(fileName);
  }
  if (ok) {
    file = LittleFS.open(fileName, "w");
    if (file) {
      size_t len = Pass.length()+1, lenw = 0;
      if (len > 64) len = 64;
      char buf[65];
      memset(buf, '\0', 65);
      Pass.toCharArray(buf, len);
      lenw = file.write((uint8_t*)buf, len);
      ok = lenw == len;
      file.close();
    } else {
      ok = false;
    }
  }
  if (!ok) {
    DEBUGLN(String(F("Ошибка сохранения пароля")));
  } 
  return ok; 
}

// -----------------------------

String getNtpServer() {
  return EEPROM_string_read(120, 30);
}

void putNtpServer(String server) {
  if (server != getNtpServer()) {
    EEPROM_string_write(120, server, 30);
  }
}

void putAM1params(uint8_t hour, uint8_t minute, int8_t effect) { 
  putAM1hour(hour);
  putAM1minute(minute);
  putAM1effect(effect);
}

uint8_t getAM1hour() { 
  uint8_t hour = EEPROMread(33);
  if (hour>23) hour = 0;
  return hour;
}

void putAM1hour(uint8_t hour) {
  if (hour != getAM1hour()) {
    EEPROMwrite(33, hour);
  }
}

uint8_t getAM1minute() {
  uint8_t minute = EEPROMread(34);
  if (minute > 59) minute = 0;
  return minute;
}

void putAM1minute(uint8_t minute) {
  if (minute != getAM1minute()) {
    EEPROMwrite(34, minute);
  }
}

int8_t getAM1effect() {
  int8_t value = (int8_t)EEPROMread(35);
  if (value < -3) value = -3;
  return value;
}

void putAM1effect(int8_t effect) {
  if (effect != getAM1effect()) {
    EEPROMwrite(35, (uint8_t)effect);
  }
}

void putAM2params(uint8_t hour, uint8_t minute, int8_t effect) { 
  putAM2hour(hour);
  putAM2minute(minute);
  putAM2effect(effect);
}

uint8_t getAM2hour() { 
  uint8_t hour = EEPROMread(36);
  if (hour>23) hour = 0;
  return hour;
}

void putAM2hour(uint8_t hour) {
  if (hour != getAM2hour()) {
    EEPROMwrite(36, hour);
  }
}

uint8_t getAM2minute() {
  uint8_t minute = EEPROMread(37);
  if (minute > 59) minute = 0;
  return minute;
}

void putAM2minute(uint8_t minute) {
  if (minute != getAM2minute()) {
    EEPROMwrite(37, minute);
  }
}

int8_t getAM2effect() {
  int8_t value = (int8_t)EEPROMread(38);
  if (value < -3) value = -3;
  return value;
}

void putAM2effect(int8_t effect) {
  if (effect != getAM2effect()) {
    EEPROMwrite(38, (uint8_t)effect);
  }
}

void putAM3params(uint8_t hour, uint8_t minute, int8_t effect) { 
  putAM3hour(hour);
  putAM3minute(minute);
  putAM3effect(effect);
}

uint8_t getAM3hour() { 
  uint8_t hour = EEPROMread(161);
  if (hour>23) hour = 0;
  return hour;
}

void putAM3hour(uint8_t hour) {
  if (hour != getAM3hour()) {
    EEPROMwrite(161, hour);
  }
}

uint8_t getAM3minute() {
  uint8_t minute = EEPROMread(162);
  if (minute > 59) minute = 0;
  return minute;
}

void putAM3minute(uint8_t minute) {
  if (minute != getAM3minute()) {
    EEPROMwrite(162, minute);
  }
}

int8_t getAM3effect() {
  int8_t value = (int8_t)EEPROMread(163);
  if (value < -3) value = -3;
  return value;
}

void putAM3effect(int8_t effect) {
  if (effect != getAM3effect()) {
    EEPROMwrite(163, (uint8_t)effect);
  }
}

void putAM4params(uint8_t hour, uint8_t minute, int8_t effect) { 
  putAM4hour(hour);
  putAM4minute(minute);
  putAM4effect(effect);
}

uint8_t getAM4hour() { 
  uint8_t hour = EEPROMread(164);
  if (hour>23) hour = 0;
  return hour;
}

void putAM4hour(uint8_t hour) {
  if (hour != getAM4hour()) {
    EEPROMwrite(164, hour);
  }
}

uint8_t getAM4minute() {
  uint8_t minute = EEPROMread(165);
  if (minute > 59) minute = 0;
  return minute;
}

void putAM4minute(uint8_t minute) {
  if (minute != getAM4minute()) {
    EEPROMwrite(165, minute);
  }
}

int8_t getAM4effect() {
  int8_t value = (int8_t)EEPROMread(166);
  if (value < -3) value = -3;
  return value;
}

void putAM4effect(int8_t effect) {
  if (effect != getAM4effect()) {
    EEPROMwrite(166, (uint8_t)effect);
  }
}

int8_t getAM5effect() {
  int8_t value = (int8_t)EEPROMread(243);
  if (value < -3) value = -3;
  return value;
}

void putAM5effect(int8_t effect) {
  if (effect != getAM5effect()) {
    EEPROMwrite(243, (uint8_t)effect);
  }
}

int8_t getAM6effect() {
  int8_t value = (int8_t)EEPROMread(248);
  if (value < -3) value = -3;
  return value;
}

void putAM6effect(int8_t effect) {
  if (effect != getAM6effect()) {
    EEPROMwrite(248, (uint8_t)effect);
  }
}

int8_t getCurrentManualMode() {
  return (int8_t)EEPROMread(29);
}

void putCurrentManualMode(int8_t mode) {
  if (mode != getCurrentManualMode()) {
    EEPROMwrite(29, (uint8_t)mode);
  }
}

void getStaticIP() {
  IP_STA[0] = EEPROMread(10);
  IP_STA[1] = EEPROMread(11);
  IP_STA[2] = EEPROMread(12);
  IP_STA[3] = EEPROMread(13);
}

void putStaticIP(uint8_t p1, uint8_t p2, uint8_t p3, uint8_t p4) {
  EEPROMwrite(10, p1);
  EEPROMwrite(11, p2);
  EEPROMwrite(12, p3);
  EEPROMwrite(13, p4);
}

uint32_t getGlobalColor() {
  uint8_t r,g,b;
  r = EEPROMread(158);
  g = EEPROMread(159);
  b = EEPROMread(160);
  return (uint32_t)r<<16 | (uint32_t)g<<8 | (uint32_t)b;
}

void putGlobalColor(uint32_t color) {
  if (color != getGlobalColor()) {
    CRGB cl = CRGB(color);
    EEPROMwrite(158, cl.r); // R
    EEPROMwrite(159, cl.g); // G
    EEPROMwrite(160, cl.b); // B
  }
}

uint32_t getGlobalClockColor() {
  uint8_t r,g,b;
  r = EEPROMread(152);
  g = EEPROMread(153);
  b = EEPROMread(154);
  return (uint32_t)r<<16 | (uint32_t)g<<8 | (uint32_t)b;
}

void putGlobalClockColor(uint32_t color) {
  if (color != getGlobalClockColor()) {
    CRGB cl = CRGB(color);
    EEPROMwrite(152, cl.r); // R
    EEPROMwrite(153, cl.g); // G
    EEPROMwrite(154, cl.b); // B
  }
}

uint32_t getGlobalTextColor() {
  uint8_t r,g,b;
  r = EEPROMread(155);
  g = EEPROMread(156);
  b = EEPROMread(157);
  return (uint32_t)r<<16 | (uint32_t)g<<8 | (uint32_t)b;
}

void putGlobalTextColor(uint32_t color) {
  if (color != getGlobalTextColor()) {
    CRGB cl = CRGB(color);
    EEPROMwrite(155, cl.r); // R 
    EEPROMwrite(156, cl.g); // G
    EEPROMwrite(157, cl.b); // B
  }
}

int8_t getCurrentSpecMode() {
  return (int8_t)EEPROMread(28);
}

void putCurrentSpecMode(int8_t mode) {
  if (mode != getCurrentSpecMode()) {
    EEPROMwrite(28, (uint8_t)mode);
  }
}

void putRandomMode(bool randomMode) {
  if (randomMode != getRandomMode()) {
    EEPROMwrite(31, randomMode ? 1 : 0);
  }  
}

bool getRandomMode() {
 return EEPROMread(31) != 0;
}

void putPowerLimit(uint16_t limit) {
  if (limit != getPowerLimit()) {
    EEPROM_int_write(150, limit);
  }
}

uint16_t getPowerLimit() {
  uint16_t val = (uint16_t)EEPROM_int_read(150);
  if (val !=0 && val < 1000) val = 1000;
  return val;
}

void putNightClockColor(uint8_t clr) {
  if (clr != getNightClockColor()) {
    EEPROMwrite(39, clr);
  }  
}

uint8_t getNightClockColor() {
  uint8_t clr = EEPROMread(39);
  if (clr > 6) clr=0;
  return clr;
}

// Интервал включения режима бегущей строки
uint16_t getTextInterval() {
  uint16_t val = (uint16_t)EEPROM_int_read(167);
  return val;
}

void putTextInterval(uint16_t interval) {
  if (interval != getTextInterval()) {
    EEPROM_int_write(167, interval);
  }  
}

// Загрузка массива строк "Бегущей строки"
void loadTexts() {

  uint16_t addr = TEXT_EEPROM;  
  uint16_t size = sizeof(textLines) / sizeof(String);   // Размер массива текста бегущих строк
  uint16_t max_text_size = sizeof(incomeBuffer);        // Размер приемного буфера формирования текста загружаемой из EEPROM строки
  uint8_t  idx = 0;                           
  bool     finished = false;

  memoryAvail = (EEPROM_MAX - TEXT_EEPROM) / 2;    // UTF8 кириллицы - один символ 2 байта

  while (addr < EEPROM_MAX && idx < size && !finished) {
   
    memset(incomeBuffer, '\0', max_text_size);
    int16_t i = 0;
   
    while (i < max_text_size) {
      uint8_t c = EEPROMread(addr++);
      finished = c == '\0' || addr == EEPROM_MAX - 1;
      if (finished || c == '\r') break;
      incomeBuffer[i++] = c;
    }

    // Сформировать строку из загруженного буфера
    textLines[idx] = String(incomeBuffer);

    idx++;
  }

  memoryAvail = (EEPROM_MAX - addr) / 2;  // UTF8 кириллицы - один символ 2 байта
  if (memoryAvail < 0) memoryAvail = 0;

  if (addr == TEXT_EEPROM + 1) {
    DEBUGLN(F("Нет сохраненных строк"));
  } else {
    DEBUG(F("Загрузка строк выполнена.\nИспользованы адреса EEPROM "));
    DEBUGLN(String(TEXT_EEPROM) + " - " + String(addr - 1));
  }
  if (addr >= EEPROM_MAX) {
    DEBUGLN(F("Память заполнена."));
  }
  DEBUG(F("Свободно ячеек "));
  DEBUGLN(String(EEPROM_MAX - addr));

  // Заполнить оставшиеся строки массива пустой строкой
  for (uint8_t i=idx; i<size; i++) {
     textLines[i] = "";
  }

  /*
  // Контрольная печать загруженных строк
  for (uint8_t i=0; i<size; i++) {
     DEBUGLN("Строка " + String(i) + " = '" + textLines[i] + "'");
  }
  */
  
  // Подсчитать контрольную сумму строк в массиве. Это позволит определить были ли изменения, нужно ли сохранять массив в EEPROM,
  // если флаг eepromModified == true
  crc = 0;
  for (uint8_t i=0; i<size; i++) {
    String text = textLines[i];
    uint16_t len = text.length();
    if (len > BUF_MAX_SIZE) len = BUF_MAX_SIZE;
    if (len>0) {
      text.toCharArray(incomeBuffer, len);
      crc ^= getCrc16((uint8_t*)incomeBuffer, len);
    }
  }

  if (textLines[0] == "#") textLines[0] = "##";
  
  /*
  // Это пример макросов в строках, использовались для отладки режимов замены макросов в тексте бегущей строки
  //
  textLines[0]  = "#123456789ABDCEFGHI";
  textLines[1]  = "-Это просто строка";
  textLines[2]  = "-Эту строку не отображать";
  textLines[3]  = "";
  textLines[4]  = "Эту строку не {-} отображать";
  textLines[5]  = "-Эта желтая строка на красном фоне{C#FFFF00}{B#FF0000}";
  textLines[6]  = "-Эта строка разнобуквица{C#000002}";
  textLines[7]  = "-Эта строка на фоне эффекта {E5}'Огонь'";
  textLines[8]  = "-Эта строка на фоне эффекта 'Палитра'{E29}";
  textLines[9]  = "-Эта строка на фоне эффекта {E28}'Тетрис'";
  textLines[10] = "-Эта строка на 40 секунд {T40}";  
  textLines[11] = "-Эта строка на 4 раза {N4}";
  textLines[12] = "-После этой строки (1) - строка 19 на фоне Analyzer{#19}";
  textLines[13] = "-После этой строки (2) - строка 19 на фоне Analyzer{#J}";
  textLines[14] = "-Текущее время: {D}";
  textLines[15] = "-Время: {D:HH:mm:ss}";
  textLines[16] = "-Дата: {D:D MMMM YYYY года}";
  textLines[17] = "-Дата и время: {D:DD.MM.YY hh:mm tt}";
  textLines[18] = "До Нового года осталось {R01.01.2021#20}";
  textLines[19] = "Это строка 19 на фоне {E30}Analyzer";
  textLines[20] = "С Новым годом!{-}";  // отображение должно быть отключено, иначе строка будет отображаться как обычная, а не только как заместитель при наступлении события
  */

  /*
  textLines[0]  = "#56789A";
  textLines[5]  = "Это зеленая строка на красном фоне{C#00FF00}{B#FF0000}";
  textLines[6]  = "Это строка c {C#000002}разноцветными {C#FFFFFF}буквами";
  textLines[7]  = "Это зеленая строка{C#00FF00}";
  textLines[8]  = "{C#0000FF}Это синяя строка";
  textLines[9]  = "Белый, {C#0000FF}Синий, {C#FF0000}Красный, {C#00FF00}Зеленый";
  textLines[10] = "До {C#0000FF}Нового года {C#FFFFFF}осталось {C#00FF00}{R23.08.2020#11}";
  textLines[11] = "-С Новым годом!";  // отображение должно быть отключено, иначе строка будет отображаться как обычная, а не только как заместитель при наступлении события
  textLines[12] = "До начала {P23.08.2020 20:00#13#60#60}";
  textLines[13] = "-Дождались!";  // отображение должно быть отключено, иначе строка будет отображаться как обычная, а не только как заместитель при наступлении события
  textLines[14] = "Подъем через {P07:00#13#60#60#12345}";
  textLines[15] = "-Доброе утро!";  // отображение должно быть отключено, иначе строка будет отображаться как обычная, а не только как заместитель при наступлении события
  */

  // Если строка 0 - управляющая - установить индекс последовательности отображения на 1-й символ (т.к. нулевой - это '#')
  sequenceIdx = isFirstLineControl() ? 1 : -1;
}

// Сохранения массива строк "Бегущей строки"
// Весть текст (все строки массива) сохраняются сплошным потоком, начиная с адреса TEXT_EEPROM до EEPROM_MAX, то есть примерно доступно 3K байта под строки.
// UTF8 для кириллицы имеет преимущественно два байта на символ - можно всего сохранить порядка 1500 букв
// Строки массива разделяются символом '\r'; Последний символ сохранения данных - '\0'

bool saveTexts() {
  uint16_t addr = TEXT_EEPROM;  
  uint8_t  size = sizeof(textLines) / sizeof(String);

  set_memoryAvail((EEPROM_MAX - TEXT_EEPROM) / 2);  // UTF8 кириллицы - один символ 2 байта

  // Подсчитать контрольную сумму строк в массиве. Это позволит определить были ли изменения, нужно ли сохранять массив в EEPROM,
  uint16_t new_crc = 0; 
  for (uint8_t i=0; i<size; i++) {
    String text = textLines[i];
    uint16_t len = text.length();
    if (len > BUF_MAX_SIZE) len = BUF_MAX_SIZE;
    text.toCharArray(incomeBuffer, len);
    new_crc ^= getCrc16((uint8_t*)incomeBuffer, len);
  }

  // CRC совпадает - массив строк не изменен - сохранять нечего
  if (crc == new_crc) return true;

  bool completed = true;
  
  for (uint8_t i=0; i<size; i++) {
        
    uint16_t len = textLines[i].length();
    uint16_t j = 0;
  
    // Сохранить новое значение
    while (j < len) {
      EEPROMwrite(addr++, textLines[i][j++]);
      if (addr == EEPROM_MAX - 1) break;
    }

    if (addr == EEPROM_MAX - 1) {
      completed = false;
      break;
    }

    if (completed) EEPROMwrite(addr++, '\r');
    
    if (addr == EEPROM_MAX - 1) {
      completed = false;
      break;
    }
  }

  // Символ завершение данных после того, как все строки записаны
  if (addr < EEPROM_MAX - 1) {  
    EEPROMwrite(addr, '\0');
  }

  set_memoryAvail((EEPROM_MAX - addr) / 2);  // UTF8 кириллицы - один символ 2 байта
  if (memoryAvail < 0) set_memoryAvail(0);
  
  DEBUG(F("Сохранение строк выполнено.\nИспользованы адреса EEPROM "));
  DEBUGLN(String(TEXT_EEPROM) + " - " + String(addr - 1));
  if (addr >= EEPROM_MAX - 1) {
    DEBUGLN(F("Память заполнена."));
  }
  DEBUG(F("Свободно ячеек "));
  DEBUGLN(String(EEPROM_MAX - addr));
  if (!completed) {
    DEBUGLN(F("Не все строки были загружены."));
  }

  return addr < EEPROM_MAX;
}

// Режим цвета часов оверлея X: 0,1,2,3
void putClockColor(uint8_t clr) {
  if (clr != getClockColor()) {
    EEPROMwrite(169, clr);
  }  
}

uint8_t getClockColor() {
  uint8_t clr = EEPROMread(169);
  if (clr > 3) clr=0;
  return clr;
}

// Скорость прокрутки часов
void putClockScrollSpeed(uint8_t clr) {
  if (clr != getClockScrollSpeed()) {
    EEPROMwrite(170, clr);
  }  
}

uint8_t getClockScrollSpeed() {
  uint8_t clr = EEPROMread(170);
  return clr;
}

// Режим цвета оверлея бегущей строки X: 0,1,2,3
void putTextColor(uint8_t clr) {
  if (clr != getTextColor()) {
    EEPROMwrite(171, clr);
  }  
}

uint8_t getTextColor() {
  uint8_t clr = EEPROMread(171);
  if (clr > 2) clr=0;
  return clr;
}

// Скорость прокрутки бегущей строки
void putTextScrollSpeed(uint8_t clr) {
  if (clr != getTextScrollSpeed()) {
    EEPROMwrite(172, clr);
  }  
}

uint8_t getTextScrollSpeed() {
  uint8_t clr = EEPROMread(172);
  return clr;
}

uint8_t getUseWeather() {
  return EEPROMread(174);
}

void putUseWeather(uint8_t id) {
  if (id != getUseWeather()) {
    EEPROMwrite(174, id);
  }
}

uint8_t getWeatherInterval() {
  uint8_t value = EEPROMread(175);
  if (value < 10) value = 10;
  return value;
}

// Интервал обновления погоды
void putWeatherInterval(uint8_t interval) {
  if (interval != getWeatherInterval()) {
    EEPROMwrite(175, interval);
  }  
}

uint32_t getWeatherRegion() {
  uint32_t region = EEPROM_long_read(176);  
  return region;
}

void putWeatherRegion(uint32_t value) {
  if (value != getWeatherRegion()) {
    EEPROM_long_write(176, value);
  }
}

uint32_t getWeatherRegion2() {
  uint32_t region = EEPROM_long_read(244);  
  return region;
}

void putWeatherRegion2(uint32_t value) {
  if (value != getWeatherRegion2()) {
    EEPROM_long_write(244, value);
  }
}

bool getUseTemperatureColor() {
  return EEPROMread(180) != 0;
}

void putUseTemperatureColor(bool use) {
  if (use != getUseTemperatureColor()) {
    EEPROMwrite(180, use ? 1 : 0);
  }
}

bool getUseTemperatureColorNight() {
  return EEPROMread(181) != 0;
}

void putUseTemperatureColorNight(bool use) {
  if (use != getUseTemperatureColorNight()) {
    EEPROMwrite(181, use ? 1 : 0);
  }
}

#if (USE_MQTT == 1)

bool getUseMqtt() {
  return EEPROMread(239) != 0;
}

void putUseMqtt(bool use) {  
  if (use != getUseMqtt()) {
    EEPROMwrite(239, use ? 1 : 0);
  }
}

uint16_t getMqttPort() {
  uint16_t val = (uint16_t)EEPROM_int_read(237);
  return val;
}

void putMqttPort(uint16_t port) {
  if (port != getMqttPort()) {
    EEPROM_int_write(237, port);
  }  
}

String getMqttServer() {
  return EEPROM_string_read(182, 24);
}

void putMqttServer(String server) {
  if (server != getMqttServer()) {
    EEPROM_string_write(182, server, 24);
  }
}

String getMqttUser() {
  return EEPROM_string_read(207, 24);
}

void putMqttUser(String user) {
  if (user != getMqttUser()) {
    EEPROM_string_write(207, user, 14);
  }
}

String getMqttPass() {
  return EEPROM_string_read(222, 24);
}

void putMqttPass(String pass) {
  if (pass != getMqttPass()) {
    EEPROM_string_write(222, pass, 14);
  }
}

String getMqttPrefix() {
  return EEPROM_string_read(250, 30);
}

void putMqttPrefix(String prefix) {  
  if (prefix != getMqttPrefix()) {
    EEPROM_string_write(250, prefix, 30);
  }
}

#endif

#if (USE_E131 == 1)

eWorkModes getSyncWorkMode() {
  uint8_t value = EEPROMread(90);
  if (value > 2) value = 0;
  return (eWorkModes)value;
}
void putSyncWorkMode(eWorkModes value) {
  uint8_t val = (uint8_t)value;
  if (val > 2) val = 0;
  if (val != (uint8_t)getSyncWorkMode()) {
    EEPROMwrite(90, val);
  }  
}

eSyncModes getSyncDataMode() {
  uint8_t value = EEPROMread(91);
  if (value > 2) value = 1;
  return (eSyncModes)value;
}
void putSyncDataMode(eSyncModes value) {
  uint8_t val = (uint8_t)value;
  if (val > 2) val = 1;
  if (val != (uint8_t)getSyncDataMode()) {
    EEPROMwrite(91, val);
  }  
}

uint8_t getSyncGroup() {
  uint8_t value = EEPROMread(92);
  if (value > 9) value = 9;
  return value;
}
void putSyncGroup(uint8_t value) {
  if (value > 9) value = 9;
  if (value != getSyncGroup()) {
    EEPROMwrite(92, value);
  }  
}

#endif

uint8_t getNightClockBrightness()   {
  uint8_t br = EEPROMread(240);
  if (br < MIN_NIGHT_CLOCK_BRIGHTNESS) br = MIN_NIGHT_CLOCK_BRIGHTNESS;
  return br;
}

void putNightClockBrightness(uint8_t brightness) {
  if (brightness < MIN_NIGHT_CLOCK_BRIGHTNESS) brightness = MIN_NIGHT_CLOCK_BRIGHTNESS;
  if (brightness != getNightClockBrightness()) {
    EEPROMwrite(240, brightness);
  }  
}

// ----------------------------------------------------------

uint8_t EEPROMread(uint16_t addr) {    
  return EEPROM.read(addr);
}

void EEPROMwrite(uint16_t addr, uint8_t value) {    
  EEPROM.write(addr, value);
  eepromModified = true;
  saveSettingsTimer.reset();
}

// чтение uint16_t
uint16_t EEPROM_int_read(uint16_t addr) {    
  uint8_t raw[2];
  for (uint8_t i = 0; i < 2; i++) raw[i] = EEPROMread(addr+i);
  uint16_t &num = (uint16_t&)raw;
  return num;
}

// запись uint16_t
void EEPROM_int_write(uint16_t addr, uint16_t num) {
  uint8_t raw[2];
  (uint16_t&)raw = num;
  for (uint8_t i = 0; i < 2; i++) EEPROMwrite(addr+i, raw[i]);
}

// чтение uint32_t
uint32_t EEPROM_long_read(uint16_t addr) {    
  uint8_t raw[4];
  for (uint8_t i = 0; i < 4; i++) raw[i] = EEPROMread(addr+i);
  uint32_t &num = (uint32_t&)raw;
  return num;
}

// запись uint32_t
void EEPROM_long_write(uint16_t addr, uint32_t num) {
  uint8_t raw[4];
  (uint32_t&)raw = num;
  for (uint8_t i = 0; i < 4; i++) EEPROMwrite(addr+i, raw[i]);
}

String EEPROM_string_read(uint16_t addr, int16_t len) {
   char buffer[len+1];
   memset(buffer,'\0',len+1);
   int16_t i = 0;
   while (i < len) {
     uint8_t c = EEPROMread(addr+i);
     if (c == 0) break;
     buffer[i++] = c;
   }
   return String(buffer);
}

void EEPROM_string_write(uint16_t addr, String buffer, uint16_t max_len) {
  uint16_t len = buffer.length();
  uint16_t i = 0;

  // Принудительно очистить "хвосты от прежнего значения"
  while (i < max_len) {
    EEPROMwrite(addr+i, 0x00);
    i++;
  }
  
  // Обрезать строку, если ее длина больше доступного места
  if (len > max_len) len = max_len;

  // Сохранить новое значение
  i = 0;
  while (i < len) {
    EEPROMwrite(addr+i, buffer[i]);
    i++;
  }
}

// Проверка наличия сохраненной резервной копии
// Возврат: 0 - не найден; 1 - найден в FS микроконтроллера; 2 - найден на SD-карте; 3 - найден в FS и на SD
uint8_t checkEepromBackup() {
  File file;
  String  fileName = F("/eeprom.bin");
  uint8_t existsFS = 0; 
  uint8_t existsSD = 0; 
  
  file = LittleFS.open(fileName, "r");
  if (file) {
    if (file.size() == EEPROM_MAX) {
      existsFS = 1;
    }
    file.close();
  }

  #if (USE_SD == 1 && FS_AS_SD == 0)
    file = SD.open(fileName);
    if (file) {
      if (file.size() == EEPROM_MAX) {
        existsSD = 2;
      }
      file.close();
    }
  #endif
  
  return existsFS + existsSD;
}

// Сохранить eeprom в файл
// storage = "FS" - внутренняя файловая система
// storage = "SD" - на SD-карту
// возврат: true - успех; false - ошибка
bool saveEepromToFile(String storage) {

  const uint8_t part_size = 128;
  bool ok = true;
  uint8_t buf[part_size];
  String message = "", fileName = F("/eeprom.bin");
  size_t len = 0;
  uint16_t cnt = 0, idx = 0;  
  File file;

  saveSettings();
  if (USE_SD == 0 || USE_SD == 1 && FS_AS_SD == 1) storage = "FS";

  DEBUG(F("Сохранение файла: "));
  DEBUGLN(storage + String(F(":/")) + fileName);

  memset(buf, 0, part_size);

  if (storage == "FS") {

    // Если файл с таким именем уже есть - удалить (перезапись файла новым)
    if (LittleFS.exists(fileName)) {
      ok = LittleFS.remove(fileName);
      if (!ok) {
        message = String(F("Ошибка создания файла '")) + fileName + "'";
        DEBUGLN(message);
        return false;
      }
    }
  
    file = LittleFS.open(fileName, "w");
  }

  #if (USE_SD == 1 && FS_AS_SD == 0) 
  if (storage == "SD") {

    // Если файл с таким именем уже есть - удалить (перезапись файла новым)
    if (SD.exists(fileName)) {
      ok = SD.remove(fileName);
      if (!ok) {
        message = String(F("Ошибка создания файла '")) + fileName + "'";
        DEBUGLN(message);
        return false;
      }
    }

    file = SD.open(fileName, FILE_WRITE);
  }
  #endif

  if (!file) {
    message = String(F("Ошибка создания файла '")) + fileName + "'";
    DEBUGLN(message);
    return false;
  }

  while (idx < EEPROM_MAX) {
    delay(0);
    if (cnt >= part_size) {
      len = file.write(buf, cnt);
      ok = len == cnt;
      if (!ok) break;
      cnt = 0;
      memset(buf, 0, part_size);
    }
    buf[cnt++] = EEPROMread(idx++);
  }

  // Дописываем остаток
  if (ok && cnt > 0) {
    len = file.write(buf, cnt);
    ok = len == cnt;
  }
  
  if (!ok) {
    message = String(F("Ошибка записи в файл '")) + fileName + "'";
    DEBUGLN(message);
    file.close();
    return false;
  }          
  
  file.close();
  DEBUGLN(F("Файл сохранен."));

  eeprom_backup = checkEepromBackup();
  
  return true;
}

// Загрузить eeprom из файла
// storage = "FS" - внутренняя файловая система
// storage = "SD" - на SD-карту
// возврат: true - успех; false - ошибка
bool loadEepromFromFile(String storage) {

  const uint8_t part_size = 128;
  bool ok = true;
  uint8_t buf[part_size];
  String message = "", fileName = F("/eeprom.bin");
  size_t len = 0;
  uint16_t idx = 0;  
  File file;

  if (USE_SD == 0 || USE_SD == 1 && FS_AS_SD == 1) storage = "FS";

  DEBUG(F("Загрузка файла: "));
  DEBUGLN(storage + String(F(":/")) + fileName);

  if (storage == "FS") {
    file = LittleFS.open(fileName, "r");
  }

  #if (USE_SD == 1 && FS_AS_SD == 0) 
  if (storage == "SD") {
    file = SD.open(fileName, FILE_READ);
  }
  #endif

  if (!file) {
    message = String(F("Файл '")) + fileName + String(F("' не найден."));
    DEBUGLN(message);
    return false;
  }
  
  clearEEPROM();
  
  while (idx < EEPROM_MAX) {
    delay(0);
    memset(buf, 0, part_size);
    len = file.read(buf, part_size);
    for (uint8_t i=0; i<len; i++) {
      EEPROMwrite(idx++, buf[i]);
    }
  }
  file.close();

  ok = idx == EEPROM_MAX;

  if (!ok) {
    message = String(F("Ошибка чтения файла '")) + fileName + "'";
    DEBUGLN(message);
    return false;
  }          

  // Записать в 0 текущее значение EEPROM_OK, иначе при несовпадении версии
  // после перезагрузки будут восстановлены значения по-умолчанию
  EEPROMwrite(0, EEPROM_OK);
  
  saveSettings();
  
  return true;
}

// ----------------------------------------------------------

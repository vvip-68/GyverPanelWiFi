#define EEPROM_OK 0x55                     // Флаг, показывающий, что EEPROM инициализирована корректными данными 
#define EFFECT_EEPROM 300                  // начальная ячейка eeprom с параметрами эффектов, 4 байта на эффект
#define TEXT_EEPROM 800                    // начальная ячейка eeprom с текстом бегущих строк

void loadSettings() {

  // Адреса в EEPROM:
  //    0 - если EEPROM_OK - EEPROM инициализировано, если другое значение - нет                             // EEPROMread(0)                 // EEPROMWrite(0, EEPROM_OK)
  //    1 - максимальная яркость ленты 1-255                                                                 // getMaxBrightness()            // saveMaxBrightness(globalBrightness)
  //    2 - автосмена режима в демо: вкл/выкл                                                                // getAutoplay();                // saveAutoplay(AUTOPLAY)
  //    3 - время автосмены режимов в сек                                                                    // getAutoplayTime()             // saveAutoplayTime(autoplayTime / 1000L)     // autoplayTime - мс; в ячейке - в сек
  //    4 - время бездействия до переключения в авторежим в минутах                                          // getIdleTime()                 // saveIdleTime(idleTime / 60L / 1000L)       // idleTime - мс; в ячейке - в мин
  //    5 - использовать синхронизацию времени через NTP                                                     // getUseNtp()                   // saveUseNtp(useNtp)
  //  6,7 - период синхронизации NTP (int16_t - 2 байта) в минутах                                           // getNtpSyncTime()              // saveNtpSyncTime(SYNC_TIME_PERIOD)
  //    8 - time zone UTC+X                                                                                  // getTimeZone();                // saveTimeZone(timeZoneOffset)
  //    9 - выключать индикатор часов при выключении лампы true - выключать / false - не выключать           // getTurnOffClockOnLampOff()    // setTurnOffClockOnLampOff(needTurnOffClock)
  //   10 - IP[0]                                                                                            // loadStaticIP()                // saveStaticIP(IP_STA[0], IP_STA[1], IP_STA[2], IP_STA[3])
  //   11 - IP[1]                                                                                            // - " -                         // - " -
  //   12 - IP[2]                                                                                            // - " -                         // - " -
  //   13 - IP[3]                                                                                            // - " -                         // - " -
  //   14 - Использовать режим точки доступа                                                                 // getUseSoftAP()                // setUseSoftAP(useSoftAP)
  //   15 - ориентация часов горизонтально / веритикально                                                    // getClockOrientation()         // saveClockOrientation(CLOCK_ORIENT)
  //   16 - Отображать с часами текущую дату                                                                 // getShowDateInClock()          // setShowDateInClock(showDateInClock)
  //   17 - Кол-во секунд отображения даты                                                                   // getShowDateDuration()         // setShowDateDuration(showDateDuration)
  //   18 - Отображать дату каждые N секунд                                                                  // getShowDateInterval()         // setShowDateInterval(showDateInterval)
  //   19 - тип часов горизонтальной ориентации 0-авто 1-малые 3х5 2 - большие 5х7                           // getClockSize()                // saveClockSize(CLOCK_SIZE)
  //   20 - Будильник, дни недели                                                                            // getAlarmWeekDay()             // saveAlarmParams(alarmWeekDay, dawnDuration, alarmEffect, alarmDuration)
  //   21 - Будильник, продолжительность "рассвета"                                                          // getDawnDuration()             // saveAlarmParams(alarmWeekDay, dawnDuration, alarmEffect, alarmDuration)
  //   22 - Будильник, эффект "рассвета"                                                                     // getAlarmEffect()              // saveAlarmParams(alarmWeekDay, dawnDuration, alarmEffect, alarmDuration)
  //   23 - Будильник, использовать звук                                                                     // getUseAlarmSound()            // saveAlarmSounds(useAlarmSound, maxAlarmVolume, alarmSound, dawnSound)
  //   24 - Будильник, играть звук N минут после срабатывания                                                // getAlarmDuration()            // saveAlarmParams(alarmWeekDay, dawnDuration, alarmEffect, alarmDuration)
  //   25 - Будильник, Номер мелодии будильника (из папки 01 на SD карте)                                    // getAlarmSound()               // saveAlarmSounds(useAlarmSound, maxAlarmVolume, alarmSound, dawnSound)
  //   26 - Будильник, Номер мелодии рассвета (из папки 02 на SD карте)                                      // getDawnSound()                // saveAlarmSounds(useAlarmSound, maxAlarmVolume, alarmSound, dawnSound)
  //   27 - Будильник, Максимальная громкость будильника                                                     // getMaxAlarmVolume()           // saveAlarmSounds(useAlarmSound, maxAlarmVolume, alarmSound, dawnSound)
  //   28 - Номер последнего активного спец-режима или -1, если были включены обычные эффекты                // getCurrentSpecMode()          // setCurrentSpecMode(xxx)                     
  //   29 - Номер последнего активированного вручную режима                                                  // getCurrentManualMode()        // setCurrentManualMode(xxx)
  //   30 - Отображать часы в режимах                                                                        // getClockOverlayEnabled()      // saveClockOverlayEnabled(overlayEnabled)
  //   31 - Использовать случайную последовательность в демо-режиме                                          // getRandomMode()               // saveRandomMode(useRandomSequence)
  //   32 - Формат часов в бегущей строке 0 - только часы; 1 - часы и дата коротко; 2 - часы и дата строкой  // getFormatClock()              // setFormatClock(formatClock)
  //   33 - Режим 1 по времени - часы                                                                        // getAM1hour()                  // setAM1hour(AM1_hour)
  //   34 - Режим 1 по времени - минуты                                                                      // getAM1minute()                // setAM1minute(AM1_minute) 
  //   35 - Режим 1 по времени - -3 - выкл. (не исп.); -2 - выкл. (черный экран); -1 - ночн.часы, 0 - случ., // getAM1effect()                // setAM1effect(AM1_effect_id)
  //   36 - Режим 2 по времени - часы >>>                                   ^^^ 1,2..N - эффект EFFECT_LIST  // getAM2hour()                  // setAM2hour(AM2_hour) 
  //   37 - Режим 2 по времени - минуты                                                                      // getAM2minute()                // setAM2minute(AM2_minute)
  //   38 - Режим 2 по времени - = " = как для режима 1                                                      // getAM2effect()                // setAM2effect(AM2_effect_id)
  //   39 - Цвет ночных часов:  0 - R; 1 - G; 2 - B; 3 - C; 4 - M; 5 - Y; 6 - W;                             // getNightClockColor()          // setNightClockColor(nightClockColor)          
  //   40 - Будильник, время: понедельник : часы                                                             // getAlarmHour(1)               // setAlarmTime(1, alarmHour[0], alarmMinute[0])  // for (byte i=0; i<7; i++) alarmHour[i] = getAlarmHour(i+1)
  //   41 - Будильник, время: понедельник : минуты                                                           // getAlarmMinute(1)             // setAlarmTime(1, alarmHour[0], alarmMinute[0])  // for (byte i=0; i<7; i++) alarmMinute[i] = getAlarmMinute(i+1)
  //   42 - Будильник, время: вторник : часы                                                                 // getAlarmHour(2)               // setAlarmTime(2, alarmHour[1], alarmMinute[1])  // for (byte i=0; i<7; i++) setAlarmTime(i+1, alarmHour[i], alarmMinute[i])
  //   43 - Будильник, время: вторник : минуты                                                               // getAlarmMinute(2)             // setAlarmTime(2, alarmHour[1], alarmMinute[1])
  //   44 - Будильник, время: среда : часы                                                                   // getAlarmHour(3)               // setAlarmTime(3, alarmHour[2], alarmMinute[2])
  //   45 - Будильник, время: среда : минуты                                                                 // getAlarmMinute(3)             // setAlarmTime(3, alarmHour[2], alarmMinute[2])
  //   46 - Будильник, время: четверг : часы                                                                 // getAlarmHour(4)               // setAlarmTime(4, alarmHour[3], alarmMinute[3])
  //   47 - Будильник, время: четверг : минуты                                                               // getAlarmMinute(4)             // setAlarmTime(4, alarmHour[3], alarmMinute[3])
  //   48 - Будильник, время: пятница : часы                                                                 // getAlarmHour(5)               // setAlarmTime(5, alarmHour[4], alarmMinute[4])
  //   49 - Будильник, время: пятница : минуты                                                               // getAlarmMinute(5)             // setAlarmTime(5, alarmHour[4], alarmMinute[4])
  //   50 - Будильник, время: суббота : часы                                                                 // getAlarmHour(6)               // setAlarmTime(6, alarmHour[5], alarmMinute[5])
  //   51 - Будильник, время: суббота : минуты                                                               // getAlarmMinute(6)             // setAlarmTime(6, alarmHour[5], alarmMinute[5])
  //   52 - Будильник, время: воскресенье : часы                                                             // getAlarmHour(7)               // setAlarmTime(7, alarmHour[6], alarmMinute[6])
  //   53 - Будильник, время: воскресенье : минуты                                                           // getAlarmMinute(7)             // setAlarmTime(7, alarmHour[6], alarmMinute[6])
  //  54-63   - имя точки доступа    - 10 байт                                                               // getSoftAPName().toCharArray(apName, 10)       // setSoftAPName(String(apName))       // char apName[11] = ""
  //  64-79   - пароль точки доступа - 16 байт                                                               // getSoftAPPass().toCharArray(apPass, 17)       // setSoftAPPass(String(apPass))       // char apPass[17] = "" 
  //  80-103  - имя сети  WiFi       - 24 байта                                                              // getSsid().toCharArray(ssid, 25)               // setSsid(String(ssid))               // char ssid[25]   = ""
  //  104-119 - пароль сети  WiFi    - 16 байт                                                               // getPass().toCharArray(pass, 17)               // setPass(String(pass))               // char pass[17]   = ""
  //  120-149 - имя NTP сервера      - 30 байт                                                               // getNtpServer().toCharArray(ntpServerName, 31) // setNtpServer(String(ntpServerName)) // char ntpServerName[31] = ""
  //  150,151 - лимит по току в миллиамперах                                                                 // getPowerLimit()                // setPowerLimit(CURRENT_LIMIT)
  //  152 - globalClockColor.r -  цвет часов в режиме MC_COLOR, режим цвета "Монохром"                       // getGlobalClockColor()          // setGlobalClockColor(globalClockColor)              // uint32_t globalClockColor
  //  153 - globalClockColor.g                                                                               // - " -                          // - " -
  //  154 - globalClockColor.b                                                                               // - " -                          // - " -
  //  155 - globalTextColor.r -  цвет текста в режиме MC_TEXT, режим цвета "Монохром"                        //  getGlobalTextColor()          // setGlobalTextColor(globalTextColor)                // uint32_t globalTextColor 
  //  156 - globalTextColor.g                                                                                // - " -                          // - " -
  //  157 - globalTextColor.b                                                                                // - " -                          // - " -
  //  158 - globalColor.r     -  цвет панели в режиме "лампа"                                                // getGlobalColor()               // setGlobalColor(globalColor)                        // uint32_t globalColor
  //  159 - globalColor.g                                                                                    // - " -                          // - " -
  //  160 - globalColor.b                                                                                    // - " -                          // - " -
  //  161 - Режим 3 по времени - часы                                                                        // getAM3hour()                   // setAM3hour(AM3_hour)
  //  162 - Режим 3 по времени - минуты                                                                      // getAM3minute()                 // setAM3minute(AM3_minute) 
  //  163 - Режим 3 по времени - так же как для режима 1                                                     // getAM3effect()                 // setAM3effect(AM3_effect_id)
  //  164 - Режим 4 по времени - часы                                                                        // getAM4hour()                   // setAM4hour(AM4_hour)
  //  165 - Режим 4 по тайвременимеру - минуты                                                               // getAM4minute()                 // setAM4minute(AM4_minute)
  //  166 - Режим 4 по времени - так же как для режима 1                                                     // getAM4effect()                 // setAM4effect(AM4_effect_id)
  //  167 - Яркость свечения эффектов                                                                        // getEffectBrightness();        // saveEffectBrightness(effectBrightness)
  //**168 - не используется
  //  ...
  //**299 - не используется
  //  300 - 300+(Nэфф*4)   - скорость эффекта
  //  301 - 300+(Nэфф*4)+1 - эффект в авторежиме: 1 - использовать; 0 - не использовать
  //  302 - 300+(Nэфф*4)+2 - специальный параметр эффекта #1
  //  303 - 300+(Nэфф*4)+3 - специальный параметр эффекта #2
  //********
  //  800 - текст строк бегущей строки сплошным массивом, строки разделены символом '\r'
  //********

  // Сначала инициализируем имя сети/точки доступа, пароли и имя NTP-сервера значениями по умолчанию.
  // Ниже, если EEPROM уже инициализирован - из него будут загружены актуальные значения
  strcpy(apName, DEFAULT_AP_NAME);
  strcpy(apPass, DEFAULT_AP_PASS);
  strcpy(ssid, NETWORK_SSID);
  strcpy(pass, NETWORK_PASS);
  strcpy(ntpServerName, DEFAULT_NTP_SERVER);    

  // Инициализировано ли EEPROM
  bool isInitialized = EEPROMread(0) == EEPROM_OK;  
    
  if (isInitialized) {    
    globalBrightness = getMaxBrightness();
    effectBrightness = getEffectBrightness();

    autoplayTime = getAutoplayTime();
    idleTime = getIdleTime();    

    useNtp = getUseNtp();
    timeZoneOffset = getTimeZone();
    overlayEnabled = getClockOverlayEnabled();

    SYNC_TIME_PERIOD = getNtpSyncTime();
    AUTOPLAY = getAutoplay();
    CLOCK_ORIENT = getClockOrientation();
    COLOR_MODE = getScaleForEffect(MC_CLOCK);
    COLOR_TEXT_MODE = getScaleForEffect(MC_TEXT);
    CURRENT_LIMIT = getPowerLimit();
    useRandomSequence = getRandomMode();
    formatClock = getFormatClock();
    nightClockColor = getNightClockColor();
    showDateInClock = getShowDateInClock();  
    showDateDuration = getShowDateDuration();
    showDateInterval = getShowDateInterval();

    alarmWeekDay = getAlarmWeekDay();
    alarmEffect = getAlarmEffect();
    alarmDuration = getAlarmDuration();
    dawnDuration = getDawnDuration();

    needTurnOffClock = getTurnOffClockOnLampOff();

    // Загрузить недельные будильники / часы, минуты /
    for (byte i=0; i<7; i++) {
      alarmHour[i] = getAlarmHour(i+1);
      alarmMinute[i] = getAlarmMinute(i+1);
    }
 
    // Загрузить параметры эффектов #1, #2
    for (byte i=0; i<MAX_EFFECT; i++) {
      effectScaleParam[i] = getScaleForEffect(i); 
      effectScaleParam2[i] = getScaleForEffect2(i);
    }

    #if (USE_MP3 == 1)
      useAlarmSound = getUseAlarmSound();
      alarmSound = getAlarmSound();
      dawnSound = getDawnSound();
      maxAlarmVolume = getMaxAlarmVolume();
    #endif

    globalColor = getGlobalColor();           // цвет лампы, задаваемый пользователем
    globalClockColor = getGlobalClockColor(); // цвет часов в режиме MC_COLOR? режим цвета "Монохром"
    globalTextColor = getGlobalTextColor();   // цвет часов бегущей строки в режиме цвета "Монохром"

    useSoftAP = getUseSoftAP();
    getSoftAPName().toCharArray(apName, 10);        //  54-63   - имя точки доступа    ( 9 байт макс) + 1 байт '\0'
    getSoftAPPass().toCharArray(apPass, 17);        //  64-79   - пароль точки доступа (16 байт макс) + 1 байт '\0'
    getSsid().toCharArray(ssid, 25);                //  80-103  - имя сети  WiFi       (24 байта макс) + 1 байт '\0'
    getPass().toCharArray(pass, 17);                //  104-119 - пароль сети  WiFi    (16 байт макс) + 1 байт '\0'
    getNtpServer().toCharArray(ntpServerName, 31);  //  120-149 - имя NTP сервера      (30 байт макс) + 1 байт '\0'

    if (strlen(apName) == 0) strcpy(apName, DEFAULT_AP_NAME);
    if (strlen(apPass) == 0) strcpy(apPass, DEFAULT_AP_PASS);
    if (strlen(ntpServerName) == 0) strcpy(ntpServerName, DEFAULT_NTP_SERVER);

    AM1_hour      = getAM1hour();
    AM1_minute    = getAM1minute();
    AM1_effect_id = getAM1effect();
    AM2_hour      = getAM2hour();
    AM2_minute    = getAM2minute();
    AM2_effect_id = getAM2effect();
    AM3_hour      = getAM3hour();
    AM3_minute    = getAM3minute();
    AM3_effect_id = getAM3effect();
    AM4_hour      = getAM4hour();
    AM4_minute    = getAM4minute();
    AM4_effect_id = getAM4effect();

    loadStaticIP();
    
  } else {
    
    globalBrightness = BRIGHTNESS;
    effectBrightness = BRIGHTNESS;
    
    autoplayTime = ((long)AUTOPLAY_PERIOD * 1000L);     // секунды -> миллисек
    idleTime = ((long)IDLE_TIME * 60L * 1000L);         // минуты -> миллисек

    useNtp = true;
    timeZoneOffset = 7;
    overlayEnabled = true;

    SYNC_TIME_PERIOD = 60;
    AUTOPLAY = true;
    CLOCK_ORIENT = 0;
    COLOR_MODE = 0;
    COLOR_TEXT_MODE = 0;
    CURRENT_LIMIT = 5000;
    useRandomSequence = true;
    formatClock = 0;
    nightClockColor = 0;
    showDateInClock = true;  
    showDateDuration = 3;
    showDateInterval = 240;
    needTurnOffClock = false;

    alarmWeekDay = 0;
    alarmEffect = MC_DAWN_ALARM;
    alarmDuration = 1;
    dawnDuration = 20;
        
    #if (USE_MP3 == 1)
      useAlarmSound = false;
      alarmSound = 1;
      dawnSound = 1;
      maxAlarmVolume = 30;
    #endif

    for (byte i=0; i<7; i++) {
      alarmHour[i] = 0;
      alarmMinute[i] = 0;
    }
    
    for (byte i=0; i<MAX_EFFECT; i++) {
      effectScaleParam[i]  = 50;  // среднее значение для параметра. Конкретное значение зависит от эффекта
      effectScaleParam2[i] = 0;   // второй параметр эффекта по умолчанию равен 0. Конкретное значение зависит от эффекта
    }

    globalColor = 0xFF;
    globalClockColor = 0xFF;
    globalTextColor = 0xFF;

    useSoftAP = false;
    
    AM1_hour = 0;
    AM1_minute = 0;
    AM1_effect_id = -3;
    AM2_hour = 0;
    AM2_minute = 0;
    AM2_effect_id = -3;    
    AM3_hour = 0;
    AM3_minute = 0;
    AM3_effect_id = -3;
    AM4_hour = 0;
    AM4_minute = 0;
    AM4_effect_id = -3;    

    IP_STA[0] = 192; IP_STA[1] = 168; IP_STA[2] = 0; IP_STA[3] = 116;    
  }

  idleTimer.setInterval(idleTime);  
  ntpSyncTimer.setInterval(1000 * 60 * SYNC_TIME_PERIOD);
  
  // После первой инициализации значений - сохранить их принудительно
  if (!isInitialized) {
    saveDefaults();
    saveSettings();
  }
}

void saveDefaults() {

  saveMaxBrightness(globalBrightness);
  saveEffectBrightness(effectBrightness);

  saveAutoplayTime(autoplayTime / 1000L);
  saveIdleTime(constrain(idleTime / 60L / 1000L, 0, 255));

  saveUseNtp(useNtp);
  saveTimeZone(timeZoneOffset);
  saveClockOverlayEnabled(overlayEnabled);

  saveNtpSyncTime(SYNC_TIME_PERIOD);
  saveAutoplay(AUTOPLAY);

  saveClockOrientation(CLOCK_ORIENT);
  setPowerLimit(CURRENT_LIMIT);
  saveRandomMode(useRandomSequence);
  setFormatClock(formatClock);          // Формат отображения часов в бегущей строке: 0 - только часы: 1 - часы и дата кратко; 2 - часы и полная дата;
  setNightClockColor(nightClockColor);  // Цвет ночных часов: 0 - R; 1 - G; 2 - B; 3 - C; 4 - M; 5 - Y;
  setShowDateInClock(showDateInClock);
  setShowDateDuration(showDateDuration);
  setShowDateInterval(showDateInterval);
  setTurnOffClockOnLampOff(needTurnOffClock);

  saveAlarmParams(alarmWeekDay,dawnDuration,alarmEffect,alarmDuration);
  #if (USE_MP3 == 1)
    saveAlarmSounds(useAlarmSound, maxAlarmVolume, alarmSound, dawnSound);
  #endif

  for (byte i=0; i<7; i++) {
    setAlarmTime(i+1, alarmHour[i], alarmMinute[i]);
  }

  // Настройки по умолчанию для эффектов
  for (int i = 0; i < MAX_EFFECT; i++) {
    saveEffectParams(i, effectSpeed, true, 50, 0);
  }

  // Специальные настройки отдельных эффектов
  setScaleForEffect(MC_FIRE, 0);                 // Огонь красного цвета
  setScaleForEffect(MC_CLOCK, COLOR_MODE);
  setScaleForEffect(MC_TEXT, COLOR_TEXT_MODE);
  setScaleForEffect2(MC_PAINTBALL, 1);           // Использовать сегменты для эффекта Пэйнтбол на широких матрицах
  setScaleForEffect2(MC_SWIRL, 1);               // Использовать сегменты для эффекта Водоворот на широких матрицах
  setScaleForEffect2(MC_RAINBOW, 0);             // Использовать рандомный выбор эффекта радуга 0 - random; 1 - диагональная; 2 - горизонтальная; 3 - вертикальная; 4 - вращающаяся
  
  setGlobalColor(globalColor);
  setGlobalClockColor(globalClockColor);
  setGlobalTextColor(globalTextColor);

  setUseSoftAP(useSoftAP);

  strcpy(apName, DEFAULT_AP_NAME);
  strcpy(apPass, DEFAULT_AP_PASS);
  strcpy(ssid, NETWORK_SSID);
  strcpy(pass, NETWORK_PASS);

  setSoftAPName(String(apName));
  setSoftAPPass(String(apPass));
  setSsid(String(ssid));
  setPass(String(pass));
  
  strcpy(ntpServerName, DEFAULT_NTP_SERVER);
  setNtpServer(String(ntpServerName));

  setAM1hour(AM1_hour);                 // Режим 1 по времени - часы
  setAM1minute(AM1_minute);             // Режим 1 по времени - минуты
  setAM1effect(AM1_effect_id);          // Режим 1 по времени - действие: -3 - выключено (не используется); -2 - выключить матрицу (черный экран); -1 - огонь, 0 - случайный, 1 и далее - эффект EFFECT_LIST
  setAM2hour(AM2_hour);                 // Режим 2 по времени - часы
  setAM2minute(AM2_minute);             // Режим 2 по времени - минуты
  setAM2effect(AM2_effect_id);          // Режим 2 по времени - действие: -3 - выключено (не используется); -2 - выключить матрицу (черный экран); -1 - огонь, 0 - случайный, 1 и далее - эффект EFFECT_LIST
  setAM3hour(AM3_hour);                 // Режим 3 по времени - часы
  setAM3minute(AM3_minute);             // Режим 3 по времени - минуты
  setAM3effect(AM3_effect_id);          // Режим 3 по времени - действие: -3 - выключено (не используется); -2 - выключить матрицу (черный экран); -1 - огонь, 0 - случайный, 1 и далее - эффект EFFECT_LIST
  setAM4hour(AM4_hour);                 // Режим 4 по времени - часы
  setAM4minute(AM4_minute);             // Режим 4 по времени - минуты
  setAM4effect(AM4_effect_id);          // Режим 4 по времени - действие: -3 - выключено (не используется); -2 - выключить матрицу (черный экран); -1 - огонь, 0 - случайный, 1 и далее - эффект EFFECT_LIST
  
  saveStaticIP(IP_STA[0], IP_STA[1], IP_STA[2], IP_STA[3]);

  setCurrentSpecMode(-1);               // Текущий спец-режим - это не спец-режим
  setCurrentManualMode(-1);             // Текущий вручную включенный спец-режим

  eepromModified = true;
}

void saveSettings() {

  saveSettingsTimer.reset();
  if (!eepromModified) return;
  
  // Поставить отметку, что EEPROM инициализировано параметрами эффектов
  EEPROMwrite(0, EEPROM_OK);
  
  EEPROM.commit();
  Serial.println(F("Настройки сохранены в EEPROM"));
  eepromModified = false;
}

void saveEffectParams(byte effect, int speed, boolean use, byte value1, byte value2) {
  const int addr = EFFECT_EEPROM;  
  EEPROMwrite(addr + effect*4, constrain(map(speed, D_EFFECT_SPEED_MIN, D_EFFECT_SPEED_MAX, 0, 255), 0, 255));        // Скорость эффекта
  EEPROMwrite(addr + effect*4 + 1, use ? 1 : 0);                                                                      // Вкл/Выкл эффект в демо-режиме
  EEPROMwrite(addr + effect*4 + 2, value1);                                                                           // Параметр эффекта #1 
  EEPROMwrite(addr + effect*4 + 3, value2);                                                                           // Параметр эффекта #2 
  effectScaleParam[effect] = value1;
  effectScaleParam2[effect] = value2;
}

void saveEffectSpeed(byte effect, int speed) {
  if (speed != getEffectSpeed(effect)) {
    const int addr = EFFECT_EEPROM;  
    EEPROMwrite(addr + effect*4, constrain(map(speed, D_EFFECT_SPEED_MIN, D_EFFECT_SPEED_MAX, 0, 255), 0, 255));        // Скорость эффекта
  }
}

byte getEffectSpeed(byte effect) {
  const int addr = EFFECT_EEPROM;
  return map8(EEPROMread(addr + effect*4),D_EFFECT_SPEED_MIN,D_EFFECT_SPEED_MAX);
}

void saveEffectUsage(byte effect, boolean use) {
  if (use != getEffectUsage(effect)) {
    const int addr = EFFECT_EEPROM;  
    EEPROMwrite(addr + effect*4 + 1, use ? 1 : 0);             // По умолчанию оверлей часов для эффекта отключен  
  }
}

boolean getEffectUsage(byte effect) {
  const int addr = EFFECT_EEPROM;
  return EEPROMread(addr + effect*4 + 1) == 1;
}

void setScaleForEffect(byte effect, byte value) {
  if (value != getScaleForEffect(effect)) {
    const int addr = EFFECT_EEPROM;
    EEPROMwrite(addr + effect*4 + 2, value);
    effectScaleParam[effect] = value;
  }  
}

byte getScaleForEffect(byte effect) {
  const int addr = EFFECT_EEPROM;
  byte value = EEPROMread(addr + effect*4 + 2);
  effectScaleParam[effect] = value;
  return value;
}

void setScaleForEffect2(byte effect, byte value) {
  if (value != getScaleForEffect2(effect)) {
    const int addr = EFFECT_EEPROM;
    EEPROMwrite(addr + effect*4 + 3, value);
    effectScaleParam2[effect] = value;
  }  
}

byte getScaleForEffect2(byte effect) {
  const int addr = EFFECT_EEPROM;
  byte value = EEPROMread(addr + effect*4 + 3);
  effectScaleParam2[effect] = value;
  return value;
}

byte getMaxBrightness() {
  return EEPROMread(1);
}

void saveMaxBrightness(byte brightness) {
  if (brightness != getMaxBrightness()) {
    EEPROMwrite(1, brightness);
  }
}

byte getEffectBrightness() {
  return EEPROMread(167);
}

void saveEffectBrightness(byte brightness) {
  if (brightness != getEffectBrightness()) {
    EEPROMwrite(167, brightness);
  }
}

void saveAutoplay(boolean value) {
  if (value != getAutoplay()) {
    EEPROMwrite(2, value ? 1 : 0);
  }
}

bool getAutoplay() {
  return EEPROMread(2) == 1;
}

void saveAutoplayTime(long value) {
  if (value != getAutoplayTime()) {
    EEPROMwrite(3, constrain(value / 1000L, 0, 255));
  }
}

long getAutoplayTime() {
  long time = EEPROMread(3) * 1000L;  
  if (time == 0) time = ((long)AUTOPLAY_PERIOD * 1000L);
  return time;
}

void saveIdleTime(long value) {
  if (value != getIdleTime()) {
    EEPROMwrite(4, constrain(value / 60L / 1000L, 0, 255));
  }
}

long getIdleTime() {
  long time = EEPROMread(4) * 60 * 1000L;  
  return time;
}

boolean getClockOverlayEnabled() {
  return (EEPROMread(30) == 1) && (allowVertical || allowHorizontal);
}

void saveClockOverlayEnabled(boolean enabled) {
  if (enabled != getClockOverlayEnabled()) {
    EEPROMwrite(30, enabled ? 1 : 0);
  }
}
void saveUseNtp(boolean value) {
  if (value != getUseNtp()) {
    EEPROMwrite(5, value);
  }
}

bool getUseNtp() {
  return EEPROMread(5) == 1;
}

void saveNtpSyncTime(uint16_t value) {
  if (value != getNtpSyncTime()) {
    EEPROM_int_write(6, SYNC_TIME_PERIOD);
  }
}

uint16_t getNtpSyncTime() {
  uint16_t time = EEPROM_int_read(6);  
  if (time == 0) time = 60;
  return time;
}

void saveTimeZone(int8_t value) {
  if (value != getTimeZone()) {
    EEPROMwrite(8, (byte)value);
  }
}

int8_t getTimeZone() {
  return (int8_t)EEPROMread(8);
}

bool getTurnOffClockOnLampOff() {
  return EEPROMread(9) == 1;
}

void setTurnOffClockOnLampOff(bool flag) {
  if (flag != getTurnOffClockOnLampOff()) {
    EEPROMwrite(9, flag ? 1 : 0);
  }  
}

byte getClockOrientation() {
  
  byte val = EEPROMread(15) == 1 ? 1 : 0;
  
  if (val == 0 && !allowHorizontal) val = 1;
  if (val == 1 && !allowVertical) val = 0;

  return val;
}

void saveClockOrientation(byte orientation) {
  if (orientation != getClockOrientation()) {
    EEPROMwrite(15, orientation == 1 ? 1 : 0);
  }
}

bool getShowDateInClock() {
  bool val = EEPROMread(16) == 1;
  if (val && HEIGHT < 11) val = 0;
  return val;
}

void setShowDateInClock(boolean use) {  
  if (use != getShowDateInClock()) {
    EEPROMwrite(16, use ? 1 : 0);
    eepromModified = true;
  }
}

byte getShowDateDuration() {
  return EEPROMread(17);
}

void setShowDateDuration(byte Duration) {
  if (Duration != getShowDateDuration()) {
    EEPROMwrite(17, Duration);
    eepromModified = true;
  }
}

byte getShowDateInterval() {
  return EEPROMread(18);
}

void setShowDateInterval(byte Interval) {
  if (Interval != getShowDateInterval()) {
    EEPROMwrite(18, Interval);
    eepromModified = true;
  }
}

byte getClockSize() {
  return EEPROMread(19);
}

void saveClockSize(byte c_size) {
  if (c_size != getClockSize()) {
    EEPROMwrite(19, c_size);
  }
}

void saveAlarmParams(byte alarmWeekDay, byte dawnDuration, byte alarmEffect, byte alarmDuration) {  
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

byte getAlarmHour(byte day) { 
  return constrain(EEPROMread(40 + 2 * (day - 1)), 0, 23);
}

byte getAlarmMinute(byte day) { 
  return constrain(EEPROMread(40 + 2 * (day - 1) + 1), 0, 59);
}

void setAlarmTime(byte day, byte hour, byte minute) { 
  if (hour != getAlarmHour(day)) {
    EEPROMwrite(40 + 2 * (day - 1), constrain(hour, 0, 23));
  }
  if (minute != getAlarmMinute(day)) {
    EEPROMwrite(40 + 2 * (day - 1) + 1, constrain(minute, 0, 59));
  }
}

byte getAlarmWeekDay() { 
  return EEPROMread(20);
}

byte getDawnDuration() { 
  return constrain(EEPROMread(21),1,59);
}

void saveAlarmSounds(boolean useSound, byte maxVolume, int8_t alarmSound, int8_t dawnSound) {
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
    EEPROMwrite(25, (byte)alarmSound);
  }
  if (dawnSound != getDawnSound()) {
    EEPROMwrite(26, (byte)dawnSound);
  }
  if (maxVolume != getMaxAlarmVolume()) {
    EEPROMwrite(27, maxVolume);
  }
}

byte getAlarmEffect() { 
  return EEPROMread(22);
}

bool getUseAlarmSound() { 
  return EEPROMread(23) == 1;
}

byte getAlarmDuration() { 
  return constrain(EEPROMread(24),1,10);
}

byte getMaxAlarmVolume() { 
  return constrain(EEPROMread(27),0,30);
}

int8_t getAlarmSound() { 
  return constrain((int8_t)EEPROMread(25),-1,127);
}

int8_t getDawnSound() { 
  return constrain((int8_t)EEPROMread(26),-1,127);
}

bool getUseSoftAP() {
  return EEPROMread(14) == 1;
}

void setUseSoftAP(boolean use) {  
  if (use != getUseSoftAP()) {
    EEPROMwrite(14, use ? 1 : 0);
  }
}

String getSoftAPName() {
  return EEPROM_string_read(54, 10);
}

void setSoftAPName(String SoftAPName) {
  if (SoftAPName != getSoftAPName()) {
    EEPROM_string_write(54, SoftAPName, 10);
  }
}

String getSoftAPPass() {
  return EEPROM_string_read(64, 16);
}

void setSoftAPPass(String SoftAPPass) {
  if (SoftAPPass != getSoftAPPass()) {
    EEPROM_string_write(64, SoftAPPass, 16);
  }
}

String getSsid() {
  return EEPROM_string_read(80, 24);
}

void setSsid(String Ssid) {
  if (Ssid != getSsid()) {
    EEPROM_string_write(80, Ssid, 24);
  }
}

String getPass() {
  return EEPROM_string_read(104, 16);
}

void setPass(String Pass) {
  if (Pass != getPass()) {
    EEPROM_string_write(104, Pass, 16);
  }
}

String getNtpServer() {
  return EEPROM_string_read(120, 30);
}

void setNtpServer(String server) {
  if (server != getNtpServer()) {
    EEPROM_string_write(120, server, 30);
  }
}

void setAM1params(byte hour, byte minute, int8_t effect) { 
  setAM1hour(hour);
  setAM1minute(minute);
  setAM1effect(effect);
}

byte getAM1hour() { 
  byte hour = EEPROMread(33);
  if (hour>23) hour = 0;
  return hour;
}

void setAM1hour(byte hour) {
  if (hour != getAM1hour()) {
    EEPROMwrite(33, hour);
  }
}

byte getAM1minute() {
  byte minute = EEPROMread(34);
  if (minute > 59) minute = 0;
  return minute;
}

void setAM1minute(byte minute) {
  if (minute != getAM1minute()) {
    EEPROMwrite(34, minute);
  }
}

int8_t getAM1effect() {
  return (int8_t)EEPROMread(35);
}

void setAM1effect(int8_t effect) {
  if (effect != getAM1effect()) {
    EEPROMwrite(35, (byte)effect);
  }
}

void setAM2params(byte hour, byte minute, int8_t effect) { 
  setAM2hour(hour);
  setAM2minute(minute);
  setAM2effect(effect);
}

byte getAM2hour() { 
  byte hour = EEPROMread(36);
  if (hour>23) hour = 0;
  return hour;
}

void setAM2hour(byte hour) {
  if (hour != getAM2hour()) {
    EEPROMwrite(36, hour);
  }
}

byte getAM2minute() {
  byte minute = EEPROMread(37);
  if (minute > 59) minute = 0;
  return minute;
}

void setAM2minute(byte minute) {
  if (minute != getAM2minute()) {
    EEPROMwrite(37, minute);
  }
}

int8_t getAM2effect() {
  return (int8_t)EEPROMread(38);
}

void setAM2effect(int8_t effect) {
  if (effect != getAM2effect()) {
    EEPROMwrite(38, (byte)effect);
  }
}

void setAM3params(byte hour, byte minute, int8_t effect) { 
  setAM3hour(hour);
  setAM3minute(minute);
  setAM3effect(effect);
}

byte getAM3hour() { 
  byte hour = EEPROMread(161);
  if (hour>23) hour = 0;
  return hour;
}

void setAM3hour(byte hour) {
  if (hour != getAM3hour()) {
    EEPROMwrite(161, hour);
  }
}

byte getAM3minute() {
  byte minute = EEPROMread(162);
  if (minute > 59) minute = 0;
  return minute;
}

void setAM3minute(byte minute) {
  if (minute != getAM3minute()) {
    EEPROMwrite(162, minute);
  }
}

int8_t getAM3effect() {
  return (int8_t)EEPROMread(163);
}

void setAM3effect(int8_t effect) {
  if (effect != getAM3effect()) {
    EEPROMwrite(163, (byte)effect);
  }
}

void setAM4params(byte hour, byte minute, int8_t effect) { 
  setAM4hour(hour);
  setAM4minute(minute);
  setAM4effect(effect);
}

byte getAM4hour() { 
  byte hour = EEPROMread(164);
  if (hour>23) hour = 0;
  return hour;
}

void setAM4hour(byte hour) {
  if (hour != getAM4hour()) {
    EEPROMwrite(164, hour);
  }
}

byte getAM4minute() {
  byte minute = EEPROMread(165);
  if (minute > 59) minute = 0;
  return minute;
}

void setAM4minute(byte minute) {
  if (minute != getAM4minute()) {
    EEPROMwrite(165, minute);
  }
}

int8_t getAM4effect() {
  return (int8_t)EEPROMread(166);
}

void setAM4effect(int8_t effect) {
  if (effect != getAM4effect()) {
    EEPROMwrite(166, (byte)effect);
  }
}

int8_t getCurrentManualMode() {
  return (int8_t)EEPROMread(29);
}

void setCurrentManualMode(int8_t mode) {
  if (mode != getCurrentManualMode()) {
    EEPROMwrite(29, (byte)mode);
  }
}
void loadStaticIP() {
  IP_STA[0] = EEPROMread(10);
  IP_STA[1] = EEPROMread(11);
  IP_STA[2] = EEPROMread(12);
  IP_STA[3] = EEPROMread(13);
}

void saveStaticIP(byte p1, byte p2, byte p3, byte p4) {
  if (IP_STA[0] != p1 || IP_STA[1] != p2 || IP_STA[2] != p3 || IP_STA[3] != p4) {
    IP_STA[0] = p1;
    IP_STA[1] = p2;
    IP_STA[2] = p3;
    IP_STA[3] = p4;
    EEPROMwrite(10, p1);
    EEPROMwrite(11, p2);
    EEPROMwrite(12, p3);
    EEPROMwrite(13, p4);
  }
}

uint32_t getGlobalColor() {
  byte r,g,b;
  r = EEPROMread(158);
  g = EEPROMread(159);
  b = EEPROMread(160);
  return (uint32_t)r<<16 | (uint32_t)g<<8 | (uint32_t)b;
}

void setGlobalColor(uint32_t color) {
  globalColor = color;
  if (color != getGlobalColor()) {
    CRGB cl = CRGB(color);
    EEPROMwrite(158, cl.r); // R
    EEPROMwrite(159, cl.g); // G
    EEPROMwrite(160, cl.b); // B
  }
}

uint32_t getGlobalClockColor() {
  byte r,g,b;
  r = EEPROMread(152);
  g = EEPROMread(153);
  b = EEPROMread(154);
  return (uint32_t)r<<16 | (uint32_t)g<<8 | (uint32_t)b;
}

void setGlobalClockColor(uint32_t color) {
  globalClockColor = color;
  if (color != getGlobalClockColor()) {
    CRGB cl = CRGB(color);
    EEPROMwrite(152, cl.r); // R
    EEPROMwrite(153, cl.g); // G
    EEPROMwrite(154, cl.b); // B
  }
}

uint32_t getGlobalTextColor() {
  byte r,g,b;
  r = EEPROMread(155);
  g = EEPROMread(156);
  b = EEPROMread(157);
  return (uint32_t)r<<16 | (uint32_t)g<<8 | (uint32_t)b;
}

void setGlobalTextColor(uint32_t color) {
  globalTextColor = color;
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

void setCurrentSpecMode(int8_t mode) {
  if (mode != getCurrentSpecMode()) {
    EEPROMwrite(28, (byte)mode);
  }
}

void saveRandomMode(bool randomMode) {
  if (randomMode != getRandomMode()) {
    EEPROMwrite(31, randomMode ? 1 : 0);
  }  
}

bool getRandomMode() {
 return EEPROMread(31) != 0;
}

void setFormatClock(byte fmt) {
  if (fmt != getFormatClock()) {
    EEPROMwrite(32, fmt);
  }  
}

byte getFormatClock() {
  byte fmt = EEPROMread(32);
  if (fmt > 2) fmt=0;
  return fmt;
}

void setPowerLimit(uint16_t limit) {
  if (limit != getPowerLimit()) {
    EEPROM_int_write(150, limit);
    eepromModified = true;
  }
}

uint16_t getPowerLimit() {
  uint16_t val = (uint16_t)EEPROM_int_read(150);
  if (val !=0 && val < 1000) val = 1000;
  return val;
}

void setNightClockColor(byte clr) {
  if (clr != getNightClockColor()) {
    EEPROMwrite(39, clr);
  }  
}

byte getNightClockColor() {
  byte clr = EEPROMread(39);
  if (clr > 6) clr=0;
  return clr;
}

// ----------------------------------------------------------

byte EEPROMread(uint16_t addr) {    
  return EEPROM.read(addr);
}

void EEPROMwrite(uint16_t addr, byte value) {    
  EEPROM.write(addr, value);
  eepromModified = true;
  saveSettingsTimer.reset();
}

// чтение uint16_t
uint16_t EEPROM_int_read(uint16_t addr) {    
  byte raw[2];
  for (byte i = 0; i < 2; i++) raw[i] = EEPROMread(addr+i);
  uint16_t &num = (uint16_t&)raw;
  return num;
}

// запись uint16_t
void EEPROM_int_write(uint16_t addr, uint16_t num) {
  byte raw[2];
  (uint16_t&)raw = num;
  for (byte i = 0; i < 2; i++) EEPROMwrite(addr+i, raw[i]);
  eepromModified = true;
  saveSettingsTimer.reset();
}

String EEPROM_string_read(uint16_t addr, int16_t len) {
   char buffer[len+1];
   memset(buffer,'\0',len+1);
   int16_t i = 0;
   while (i < len) {
     byte c = EEPROMread(addr+i);
     if (c == 0) break;
     buffer[i++] = c;
     // if (c != 0 && (isAlphaNumeric(c) || isPunct(c) || isSpace(c)))
   }
   return String(buffer);
}

void EEPROM_string_write(uint16_t addr, String buffer, int16_t max_len) {
  uint16_t len = buffer.length();
  int16_t i = 0;
  if (len > max_len) len = max_len;
  while (i < len) {
    EEPROMwrite(addr+i, buffer[i++]);
  }
  if (i < max_len) EEPROMwrite(addr+i,0);
  eepromModified = true;
  saveSettingsTimer.reset();
}

// ----------------------------------------------------------

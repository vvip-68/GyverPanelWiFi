// Для поддержки работы MQTT нужно знать какой параметр изменился, чтобы отправить изменения на сервер
// По этой причине нельзя напрямую присваивать новое значение переменной, нужно выполнить дополнительные действия,
// чтобы зафиксировать изменение значения. Для этой цели данная страница содержит функции - сеттеры, 
// устанавливающие значения переменных.

// Добавление ключа (параметра) в список изменившихся параметров, чьи новые значения необходимо отправить на сервер
void addKeyToChanged(String key) {
  #if (USE_MQTT == 1)  
    String search = "|" + key + "|";
    // Добавляемый ключ есть в списке ключей, которыми интересуется клиент (STATE_KEYS)?
    if (String(STATE_KEYS).indexOf(search) >= 0) {
      // Если ключа еще нет в строке измененных параметров - добавить 
      if      (changed_keys.length() == 0)       changed_keys = search;
      else if (changed_keys.indexOf(search) < 0) changed_keys += key + "|";
    }
  #endif  
}

// PS isTurnedOff
void set_isTurnedOff(bool value) {
  if (isTurnedOff == value) return;
  isTurnedOff = value;
  addKeyToChanged("PS");
  #if (USE_E131 == 1)
    commandTurnOnOff(value);
  #endif
}

// BR isNightClock
void set_isNightClock(bool value) {
  if (isNightClock == value) return;
  isNightClock = value;
  addKeyToChanged("BR");
}

// BR globalBrightness
void set_globalBrightness(uint8_t value) {
  #if (USE_E131 == 1)
    commandSetBrightness(value);
  #endif
  if (globalBrightness == value) return;
  putMaxBrightness(value);
  globalBrightness = getMaxBrightness();
  addKeyToChanged("BR");
}

// BR specialBrightness
void set_specialBrightness(uint8_t value) {
  #if (USE_E131 == 1)
    commandSetSpecialBrightness(value);
  #endif
  if (specialBrightness == value) return;;
  specialBrightness = value;
  addKeyToChanged("BR");
}

// DM manualMode
void set_manualMode(bool value) {
  if (manualMode == value) return;
  putAutoplay(value);
  manualMode = getAutoplay();
  addKeyToChanged("DM");
}

// PD autoplayTime
void set_autoplayTime(uint32_t value) {
  if (autoplayTime == value) return;
  putAutoplayTime(value);
  autoplayTime = getAutoplayTime();
  addKeyToChanged("PD");
}

// IT idleTime
void set_idleTime(uint32_t value) {
  if (idleTime == value) return;;
  putIdleTime(value);
  idleTime = getIdleTime();
  addKeyToChanged("IT");
}

// AL isAlarming 
void set_isAlarming(bool value) {
  if (isAlarming == value) return;
  isAlarming = value;
  addKeyToChanged("AL");
}

// AL isPlayAlarmSound
void set_isPlayAlarmSound(bool value) {
  if (isPlayAlarmSound == value) return;
  isPlayAlarmSound = value;
  addKeyToChanged("AL");
}

// AL isAlarmStopped
void set_isAlarmStopped(bool value) {
  if (isAlarmStopped == value) return;
  isAlarmStopped = value;
  addKeyToChanged("AL");
}

// RM useRandomSequence
void set_useRandomSequence(bool value) {
  if (useRandomSequence == value) return;
  putRandomMode(value);
  useRandomSequence = getRandomMode();
  addKeyToChanged("RM");
}

// PW CURRENT_LIMIT
void set_CURRENT_LIMIT(uint16_t value) {
  if (CURRENT_LIMIT == value) return;
  putPowerLimit(value);
  CURRENT_LIMIT = getPowerLimit();
  addKeyToChanged("PW");
}

#if (USE_WEATHER == 1)
// WU useWeather
void set_useWeather(uint8_t value) {
  if (useWeather == value) return;
  putUseWeather(value);
  useWeather = getUseWeather();
  addKeyToChanged("WU");
}

// WT SYNC_WEATHER_PERIOD
void set_SYNC_WEATHER_PERIOD(uint16_t value) {
  if (SYNC_WEATHER_PERIOD == value) return;
  putWeatherInterval(value);
  SYNC_WEATHER_PERIOD = getWeatherInterval();
  addKeyToChanged("WT");
}

// WR regionID
void set_regionID(uint32_t value) {
  if (regionID == value) return;
  putWeatherRegion(value);
  regionID = getWeatherRegion();
  addKeyToChanged("WR");
}

// WS regionID2
void set_regionID2(uint32_t value) {
  if (regionID2 == value) return;
  putWeatherRegion2(value);
  regionID2 = getWeatherRegion2();
  addKeyToChanged("WS");
}

// WC useTemperatureColor
void set_useTemperatureColor(bool value) {
  if (useTemperatureColor == value) return;
  putUseTemperatureColor(value);
  useTemperatureColor = getUseTemperatureColor(); 
  addKeyToChanged("WC");
}

// WN useTemperatureColorNight
void set_useTemperatureColorNight(bool value) {
  if (useTemperatureColorNight == value) return;
  putUseTemperatureColorNight(value);
  useTemperatureColorNight = getUseTemperatureColorNight(); 
  addKeyToChanged("WN");
}

// W1 weather
void set_weather(String value) {
  if (weather == value) return;
  weather = value;
  addKeyToChanged("W1");
}

// W2 temperature
void set_temperature(int8_t value) {
  if (temperature == value) return;
  temperature = value;
  addKeyToChanged("W2");
}
#endif

// EF thisMode
// EN thisMode
// UE thisMode
// UT thisMode
// UC thisMode
// SE thisMode
// BE thisMode
// SS thisMode
// SQ thisMode
void set_thisMode(int8_t value) {
  if (thisMode == value) return;
  
  bool valid = (value == -1) || (value >= 0 && value < MAX_EFFECT) || (value >= SPECIAL_EFFECTS_START);
  if (!valid) return;

  valid = (value >= 0 && value < MAX_EFFECT);

  bool old_UE;
  String keySE = "SE", keyBE = "BE", keyUT = "UT", keyUC = "UC", old_SQ, old_SS, old_SE, old_BE, old_UT, old_UC;

  if (valid) {
    old_UE = getEffectUsage(thisMode); 
    old_UT = getStateValue(keyUT, thisMode);
    old_UC = getStateValue(keyUC, thisMode);
    old_SE = getStateValue(keySE, thisMode);
    old_BE = getStateValue(keyBE, thisMode);
    old_SS = getParamForMode(thisMode);
    old_SQ = getParam2ForMode(thisMode);
  }

  thisMode = value;
  
  if (valid && !isTurnedOff) {
    effect_name = getEffectName(value);
  } else if (isTurnedOff) {
    effect_name = F("Выключено");
  } else {
    switch (value) {
      case MC_DRAW:              effect_name = F("Рисование"); break;
      case MC_LOADIMAGE:         effect_name = F("Загрузка изображения"); break;
      case MC_TEXT:              effect_name = F("Бегущая строка"); break;
      case MC_DAWN_ALARM_SPIRAL: effect_name = F("Рассвет"); break;
      case MC_DAWN_ALARM_SQUARE: effect_name = F("Рассвет"); break;
      default:                   effect_name = ""; break;
    }
  }

  addKeyToChanged("EF");
  addKeyToChanged("EN");

  if (value < 0 || (valid && old_UE != getEffectUsage(value)))        addKeyToChanged("UE");
  if (value < 0 || (valid && old_UT != getStateValue(keyUT, value)))  addKeyToChanged("UT");
  if (value < 0 || (valid && old_UC != getStateValue(keyUC, value)))  addKeyToChanged("UC");
  if (value < 0 || (valid && old_SE != getStateValue(keySE, value)))  addKeyToChanged("SE");
  if (value < 0 || (valid && old_BE != getStateValue(keyBE, value)))  addKeyToChanged("BE");
  if (value < 0 || (valid && old_SS != getParamForMode(value)))       addKeyToChanged("SS");
  if (value < 0 || (valid && old_SQ != getParam2ForMode(value)))      addKeyToChanged("SQ");

  #if (USE_E131 == 1)
    commandSetMode(thisMode);
  #endif      
}

// UE
void set_EffectUsage(uint8_t effect, bool value) {
  bool valid = effect < MAX_EFFECT;
  if (!valid) return;
  bool old_value = getEffectUsage(effect);
  if (old_value != value) {
    putEffectUsage(effect, value);
    if (effect == thisMode) addKeyToChanged("UE");
  }
}

// UT 
void set_EffectTextOverlayUsage(uint8_t effect, bool value) {
  bool valid = effect < MAX_EFFECT;
  if (!valid) return;
  String key = "UT";
  bool old_value = getEffectTextOverlayUsage(effect);
  String old_s_value = getStateValue(key, effect);
  if (old_value != value) {
    putEffectTextOverlayUsage(effect, value);
    if (effect == thisMode && old_s_value != getStateValue(key, effect)) addKeyToChanged("UT");
  }
}

// UC
void set_EffectClockOverlayUsage(uint8_t effect, bool value) {
  bool valid = effect < MAX_EFFECT;
  if (!valid) return;
  String key = "UC";
  bool old_value = getEffectClockOverlayUsage(effect);
  String old_s_value = getStateValue(key, effect);
  if (old_value != value) {
    putEffectClockOverlayUsage(effect, value);
    if (effect == thisMode && old_s_value != getStateValue(key, effect)) addKeyToChanged("UC");
  }
}

// SE
void set_EffectSpeed(uint8_t effect, uint8_t value) {
  bool valid = effect < MAX_EFFECT;
  if (!valid) return;
  String key = "SE";
  uint8_t old_value = effectSpeed[effect];
  String old_s_value = getStateValue(key, effect);
  if (old_value != value) {
    effectSpeed[effect] = value;
    putEffectSpeed(effect, value);
  }
  if (effect == thisMode && old_s_value != getStateValue(key, effect)) addKeyToChanged("SE");
  #if (USE_E131 == 1)
    commandSetEffectSpeed(value);
    syncEffectSpeed = value;
  #endif
}

// BE
void set_EffectContrast(uint8_t effect, uint8_t value) {
  bool valid = effect < MAX_EFFECT;
  if (!valid) return;
  String key = "BE";
  uint8_t old_value = effectContrast[effect];
  String old_s_value = getStateValue(key, effect);
  if (old_value != value) {
    effectContrast[effect] = value;
    putEffectContrast(effect, value);
  }
  if (effect == thisMode && old_s_value != getStateValue(key, effect)) addKeyToChanged("BE");
  #if (USE_E131 == 1)
    commandSetEffectContrast(value);
    syncEffectContrast = value;
  #endif
}

// SS
void set_EffectScaleParam(uint8_t effect, uint8_t value) {
  bool valid = effect < MAX_EFFECT;
  if (!valid) return;
  uint8_t old_value = effectScaleParam[effect];
  String old_s_value = getParamForMode(effect);
  if (old_value != value) {
    effectScaleParam[effect] = value;
    putScaleForEffect(effect, value);
    if (effect == thisMode && old_s_value != getParamForMode(effect)) addKeyToChanged("SS");
  }
  #if (USE_E131 == 1)
    commandSetEffectParam(value);
    syncEffectParam1 = value;
  #endif
}

// SQ
void set_EffectScaleParam2(uint8_t effect, uint8_t value) {
  bool valid = effect < MAX_EFFECT;
  if (!valid) return;
  uint8_t old_value = effectScaleParam2[effect];
  String old_s_value = getParam2ForMode(effect);
  if (old_value != value) {
    effectScaleParam2[effect] = value;
    putScaleForEffect2(effect, value);
    if (effect == thisMode && old_s_value != getParam2ForMode(effect)) addKeyToChanged("SQ");
  }
  #if (USE_E131 == 1)
    commandSetEffectParam2(value);
    syncEffectParam2 = value;
  #endif
}

// TE textOverlayEnabled
void set_textOverlayEnabled(bool value) {
  if (textOverlayEnabled == value) return;
  putTextOverlayEnabled(value);
  textOverlayEnabled = getTextOverlayEnabled();
  addKeyToChanged("TE");
}

// TI TEXT_INTERVAL
void set_TEXT_INTERVAL(uint16_t value) {
  if (TEXT_INTERVAL == value) return;
  putTextInterval(value);
  TEXT_INTERVAL = getTextInterval();
  addKeyToChanged("TI");
}

// CT COLOR_TEXT_MODE
void set_COLOR_TEXT_MODE(uint8_t value) {
  if (COLOR_TEXT_MODE == value) return;
  putTextColor(value);
  COLOR_TEXT_MODE = getTextColor();
  addKeyToChanged("CT");
}

// ST textScrollSpeed
void set_textScrollSpeed(uint8_t value) {
  if (textScrollSpeed == value) return;
  putTextScrollSpeed(value);
  textScrollSpeed = getTextScrollSpeed();
  addKeyToChanged("ST");
  #if (USE_E131 == 1)
    commandSetTextSpeed(value);
  #endif
}

// ?? globalColor
void set_globalColor(uint32_t value) {
  if (globalColor == value) return;
  putGlobalColor(value);
  globalColor = getGlobalColor();
  #if (USE_E131 == 1)
    commandSetGlobalColor(globalColor);
  #endif
}

// C1 globalClockColor
void set_globalClockColor(uint32_t value) {
  if (globalClockColor == value) return;
  putGlobalClockColor(value);
  globalClockColor = getGlobalClockColor();
  addKeyToChanged("C1");
}

// C2 globalTextColor
void set_globalTextColor(uint32_t value) {
  if (globalTextColor == value) return;
  putGlobalTextColor(value);
  globalTextColor = getGlobalTextColor();
  addKeyToChanged("C2");
}

// OM memoryAvail
void set_memoryAvail(uint16_t value) {
  if (memoryAvail == value) return;
  memoryAvail = value;
  addKeyToChanged("OM");
}

// CE clockOverlayEnabled
void set_clockOverlayEnabled(bool value) {
  String key = "CE";
  bool old_value = clockOverlayEnabled;
  String old_s_value = getStateValue(key, thisMode);
  if (old_value != value) {
    putClockOverlayEnabled(value);
    clockOverlayEnabled = getClockOverlayEnabled();
  }
  if (old_s_value != getStateValue(key, thisMode)) addKeyToChanged("CE");
}

// CC COLOR_MODE
void set_COLOR_MODE(uint8_t value) {
  if (COLOR_MODE == value) return;
  putClockColor(value);
  COLOR_MODE = getClockColor();
  addKeyToChanged("CC");
}

// CL drawColor
void set_drawColor(uint32_t value) {
  if (drawColor == value) return;
  drawColor = value;
  addKeyToChanged("CL");
}

// CO CLOCK_ORIENT
void set_CLOCK_ORIENT(uint8_t value) {
  String key = "CO";
  uint8_t old_value = CLOCK_ORIENT;
  String old_s_value = getStateValue(key, thisMode);
  if (old_value != value) {
    putClockOrientation(value);
    CLOCK_ORIENT = getClockOrientation();
  }
  if (old_s_value != getStateValue(key, thisMode)) addKeyToChanged("CO");
}

// CK CLOCK_SIZE
void set_CLOCK_SIZE(uint8_t value) {
  if (CLOCK_SIZE == value) return;
  putClockSize(value);
  CLOCK_SIZE = getClockSize();
  addKeyToChanged("CK");
}

// NB nightClockBrightness
void set_nightClockBrightness(uint8_t value) {
  if (nightClockBrightness == value) return;
  putNightClockBrightness(value);
  nightClockBrightness = getNightClockBrightness();
  addKeyToChanged("NB");
  addKeyToChanged("BR");
}

// NC nightClockColor
void set_nightClockColor(uint8_t value) {
  if (nightClockColor == value) return;
  putNightClockColor(value);
  nightClockColor = getNightClockColor();
  addKeyToChanged("NC");
}

// SC clockScrollSpeed
void set_clockScrollSpeed(uint8_t value) {
  if (clockScrollSpeed == value) return;
  putClockScrollSpeed(value);
  clockScrollSpeed = getClockScrollSpeed();
  addKeyToChanged("SC");
  #if (USE_E131 == 1)
    commandSetClockSpeed(value);
  #endif
}

// DC showDateInClock
void set_showDateInClock(bool value) {
  if (showDateInClock == value) return;
  putShowDateInClock(value);
  showDateInClock = getShowDateInClock();
  addKeyToChanged("DC");
}

// DD showDateDuration
void set_showDateDuration(uint8_t value) {
  if (showDateDuration == value) return;
  putShowDateDuration(value);
  showDateDuration = getShowDateDuration();
  addKeyToChanged("DD");
}

// DI showDateInterval
void set_showDateInterval(uint8_t value) {
  if (showDateInterval == value) return;
  putShowDateInterval(value);
  showDateInterval = getShowDateInterval();
  addKeyToChanged("DI");
}

// NP useNtp
void set_useNtp(bool value) {
  if (useNtp == value) return;
  putUseNtp(value);
  useNtp = getUseNtp();
  addKeyToChanged("NP");
}

// NT SYNC_TIME_PERIOD
void set_SYNC_TIME_PERIOD(uint16_t value) {
  if (SYNC_TIME_PERIOD == value) return;
  putNtpSyncTime(value);
  SYNC_TIME_PERIOD = getNtpSyncTime();
  addKeyToChanged("NT");
}

// NZ timeZoneOffset
void set_timeZoneOffset(int16_t value) {
  if (timeZoneOffset == value) return;
  putTimeZone(value);
  timeZoneOffset = getTimeZone();
  addKeyToChanged("NZ");
}

// NS ntpServerName
void set_ntpServerName(String value) {
  if (getNtpServer() == value) return;
  putNtpServer(value);  
  getNtpServer().toCharArray(ntpServerName, 31);
  addKeyToChanged("NS");
}

// NW ssid
void set_Ssid(String value) {
  putSsid(value);
  ssid = value;
  addKeyToChanged("NW");
}

// NA pass
void set_pass(String value) {
  putPass(value);
  pass = value;
  addKeyToChanged("NA");
}
              
// AN apName
void set_SoftAPName(String value) {
  if (getSoftAPName() == value) return;
  putSoftAPName(value);
  getSoftAPName().toCharArray(apName, 10);
  addKeyToChanged("AN");
}              

// AA apPass
void set_SoftAPPass(String value) {
  if (getSoftAPPass() == value) return;
  putSoftAPPass(value);
  getSoftAPPass().toCharArray(apPass, 16);
  addKeyToChanged("AA");
}              

// IP wifi_connected
void set_wifi_connected(bool value) {
  if (wifi_connected == value) return;
  wifi_connected = value;
  addKeyToChanged("IP");
}              

// IP IP_STA[]
void set_StaticIP(uint8_t p1, uint8_t p2, uint8_t p3, uint8_t p4) {
  IP_STA[0] = p1; 
  IP_STA[1] = p2; 
  IP_STA[2] = p3; 
  IP_STA[3] = p4; 
  putStaticIP(p1, p2, p3, p4);
  addKeyToChanged("IP");
}              

// DW showWeatherInClock
void set_showWeatherInClock(bool value) {
  if (showWeatherInClock == value) return;
  putShowWeatherInClock(value);
  showWeatherInClock = getShowWeatherInClock();
  addKeyToChanged("DW");
}

// OF needTurnOffClock
void set_needTurnOffClock(bool value) {
  if (needTurnOffClock == value) return;
  putTurnOffClockOnLampOff(value);
  needTurnOffClock = getTurnOffClockOnLampOff();
  addKeyToChanged("OF");
}

// AD dawnDuration
void set_dawnDuration(uint8_t value) {
  if (dawnDuration == value) return;
  uint8_t alarmWeekDay = getAlarmWeekDay();
  uint8_t alarmEffect = getAlarmEffect();
  uint8_t alarmDuration = getAlarmDuration();  
  putAlarmParams(alarmWeekDay,value,alarmEffect,alarmDuration);
  dawnDuration = getDawnDuration();
  addKeyToChanged("AD");
}

// AW alarmWeekDay
void set_alarmWeekDay(uint8_t value) {
  if (alarmWeekDay == value) return;
  uint8_t dawnDuration = getDawnDuration();
  uint8_t alarmEffect = getAlarmEffect();
  uint8_t alarmDuration = getAlarmDuration();  
  putAlarmParams(value,dawnDuration,alarmEffect,alarmDuration);
  alarmWeekDay = getAlarmWeekDay();
  addKeyToChanged("AW");
}

// AE alarmEffect
void set_alarmEffect(uint8_t value) {
  if (alarmEffect == value) return;
  uint8_t alarmWeekDay = getAlarmWeekDay();
  uint8_t dawnDuration = getDawnDuration();
  uint8_t alarmDuration = getAlarmDuration();  
  putAlarmParams(alarmWeekDay,dawnDuration,value,alarmDuration);
  alarmEffect = getAlarmEffect();
  addKeyToChanged("AE");
}

// MD alarmDuration
void set_alarmDuration(uint8_t value) {
  if (alarmDuration == value) return;
  uint8_t alarmWeekDay = getAlarmWeekDay();
  uint8_t dawnDuration = getDawnDuration();
  uint8_t alarmEffect = getAlarmEffect();
  putAlarmParams(alarmWeekDay,dawnDuration,alarmEffect,value);
  alarmDuration = getAlarmDuration();  
  addKeyToChanged("MD");
}

// AT alarmHour[], alarmMinute[]
void set_alarmTime(uint8_t wd, uint8_t hour_value, uint8_t minute_value) {
  uint8_t old_hour   = getAlarmHour(wd);
  uint8_t old_minute = getAlarmMinute(wd);
  if (old_hour == hour_value && old_minute == minute_value) return;
  putAlarmTime(wd, hour_value, minute_value);
  alarmHour[wd-1] = getAlarmHour(wd);
  alarmMinute[wd-1] = getAlarmMinute(wd);
  addKeyToChanged("AT");
}

// MX isDfPlayerOk
void set_isDfPlayerOk(bool value) {
  if (isDfPlayerOk == value) return;
  isDfPlayerOk = value;
  addKeyToChanged("MX");
}

#if (USE_MP3 == 1)
// MU useAlarmSound
void set_useAlarmSound(bool value) {
  if (useAlarmSound == value) return;  
  uint8_t alarmSound = getAlarmSound();
  uint8_t dawnSound = getDawnSound();
  uint8_t maxAlarmVolume = getMaxAlarmVolume();
  putAlarmSounds(value, maxAlarmVolume, alarmSound, dawnSound);
  useAlarmSound = getUseAlarmSound();
  addKeyToChanged("MU");
}

// MV maxAlarmVolume
void set_maxAlarmVolume(uint8_t value) {
  if (maxAlarmVolume == value) return;  
  bool    useAlarmSound = getUseAlarmSound();
  uint8_t alarmSound = getAlarmSound();
  uint8_t dawnSound = getDawnSound();
  putAlarmSounds(useAlarmSound, value, alarmSound, dawnSound);
  maxAlarmVolume = getMaxAlarmVolume();
  addKeyToChanged("MV");
}

// MA alarmSound
void set_alarmSound(int8_t value) {
  if (alarmSound == value) return;  
  bool useAlarmSound = getUseAlarmSound();
  uint8_t dawnSound = getDawnSound();
  uint8_t maxAlarmVolume = getMaxAlarmVolume();
  putAlarmSounds(useAlarmSound, maxAlarmVolume, value, dawnSound);
  alarmSound = getAlarmSound();
  addKeyToChanged("MA");
}

// MB dawnSound
void set_dawnSound(int8_t value) {
  if (dawnSound == value) return;  
  bool    useAlarmSound = getUseAlarmSound();
  uint8_t alarmSound = getAlarmSound();
  uint8_t maxAlarmVolume = getMaxAlarmVolume();
  putAlarmSounds(useAlarmSound, maxAlarmVolume, alarmSound, value);
  dawnSound = getDawnSound();
  addKeyToChanged("MB");
}

// MP soundFolder
void set_soundFolder(uint8_t value) {
  if (soundFolder == value) return;  
  soundFolder = value;
  addKeyToChanged("MP");
}

// MP soundFile
void set_soundFile(uint8_t value) {
  if (soundFile == value) return;  
  soundFile = value;
  addKeyToChanged("MP");
}
#endif

// AU useSoftAP
void set_useSoftAP(bool value) {
  if (useSoftAP == value) return;
  putUseSoftAP(value);
  useSoftAP = getUseSoftAP();
  addKeyToChanged("AU");
}

// AM1T AM1_hour
void set_AM1_hour(uint8_t value) {
  if (AM1_hour == value) return;
  putAM1hour(value);
  AM1_hour = getAM1hour();
  addKeyToChanged("AM1T");
}

// AM1T AM1_minute
void set_AM1_minute(uint8_t value) {
  if (AM1_minute == value) return;
  putAM1minute(value);
  AM1_minute = getAM1minute();
  addKeyToChanged("AM1T");
}

// AM1A AM1_effect_id
void set_AM1_effect_id(int8_t value) {
  if (AM1_effect_id == value) return;
  putAM1effect(value);
  AM1_effect_id = getAM1effect();  
  addKeyToChanged("AM1A");
}

// AM2T AM2_hour
void set_AM2_hour(uint8_t value) {
  if (AM2_hour == value) return;
  putAM2hour(value);
  AM2_hour = getAM2hour();
  addKeyToChanged("AM2T");
}

// AM2T AM2_minute
void set_AM2_minute(uint8_t value) {
  if (AM2_minute == value) return;
  putAM2minute(value);
  AM2_minute = getAM2minute();
  addKeyToChanged("AM2T");
}

// AM2A AM2_effect_id
void set_AM2_effect_id(int8_t value) {
  if (AM2_effect_id == value) return;
  putAM2effect(value);
  AM2_effect_id = getAM2effect();
  addKeyToChanged("AM2A");
}

// AM3T AM3_hour
void set_AM3_hour(uint8_t value) {
  if (AM3_hour == value) return;
  putAM3hour(value);
  AM3_hour = getAM3hour();
  addKeyToChanged("AM3T");
}

// AM3T AM3_minute
void set_AM3_minute(uint8_t value) {
  if (AM3_minute == value) return;
  putAM3minute(value);
  AM3_minute = getAM3minute();
  addKeyToChanged("AM3T");
}

// AM3A AM3_effect_id
void set_AM3_effect_id(int8_t value) {
  if (AM3_effect_id == value) return;
  putAM3effect(value);
  AM3_effect_id = getAM3effect();
  addKeyToChanged("AM3A");
}

// AM4T AM4_hour
void set_AM4_hour(uint8_t value) {
  if (AM4_hour == value) return;
  putAM4hour(value);
  AM4_hour = getAM4hour();
  addKeyToChanged("AM4T");
}

// AM4T AM4_minute
void set_AM4_minute(uint8_t value) {
  if (AM4_minute == value) return;
  putAM4minute(value);
  AM4_minute = getAM4minute();
  addKeyToChanged("AM4T");
}

// AM4A AM4_effect_id
void set_AM4_effect_id(int8_t value) {
  if (AM4_effect_id == value) return;
  putAM4effect(value);
  AM4_effect_id = getAM4effect();
  addKeyToChanged("AM4A");
}

// AM5A dawn_effect_id
void set_dawn_effect_id(int8_t value) {
  if (dawn_effect_id == value) return;
  putAM5effect(value);
  dawn_effect_id = getAM5effect();
  addKeyToChanged("AM5A");
}

// AM6A dusk_effect_id
void set_dusk_effect_id(int8_t value) {
  if (dusk_effect_id == value) return;
  putAM6effect(value);
  dusk_effect_id = getAM6effect();
  addKeyToChanged("AM6A");
}

#if (USE_SD == 1)
// SD isSdCardReady
void set_isSdCardReady(bool value) {
  if (isSdCardReady == value) return;  
  isSdCardReady = value;
  addKeyToChanged("SD");
}
#endif

#if (USE_MQTT == 1)
// QA useMQTT
void set_useMQTT(bool value) {
  if (useMQTT == value) return;  
  if (useMQTT || value) stopMQTT = false;
  putUseMqtt(value);
  useMQTT = getUseMqtt();
  
  addKeyToChanged("QA");
}

// QP mqtt_port
void set_mqtt_port(int16_t value) {
  if (mqtt_port == value) return;  
  putMqttPort(value);
  mqtt_port = getMqttPort();
  addKeyToChanged("QP");
}

// QS mqtt_server
void set_MqttServer(String value) {
  if (getMqttServer() == value) return;
  putMqttServer(value);
  memset(mqtt_server, '\0', 25);
  getMqttServer().toCharArray(mqtt_server, 24);
  addKeyToChanged("QS");
}

// QU mqtt_user
void set_MqttUser(String value) {
  if (getMqttUser() == value) return;
  putMqttUser(value);
  memset(mqtt_user, '\0', 15);
  getMqttUser().toCharArray(mqtt_user, 14);
  addKeyToChanged("QU");
}

// QW mqtt_pass
void set_MqttPass(String value) {
  if (getMqttPass() == value) return;
  putMqttPass(value);
  memset(mqtt_pass, '\0', 15);
  getMqttPass().toCharArray(mqtt_pass, 14);
  addKeyToChanged("QW");
}

// QR mqtt_prefix
void set_MqttPrefix(String value) {
  if (getMqttPrefix() == value) return;
  putMqttPrefix(value);
  memset(mqtt_prefix, '\0', 31);
  getMqttPrefix().toCharArray(mqtt_prefix, 30);
  addKeyToChanged("QR");
}

#endif

#if (USE_E131 == 1)

// E1 - workMode
void set_SyncWorkMode(eWorkModes value) {
  if (workMode == value) return;
  putSyncWorkMode(value);
  workMode = getSyncWorkMode();
  addKeyToChanged("E1");  
}

// E2 - syncMode
void set_SyncDataMode(eSyncModes value) {
  if (syncMode == value) return;
  putSyncDataMode(value);
  syncMode = getSyncDataMode();
  addKeyToChanged("E2");  
}

// E3 - syncGroup
void set_SyncGroup(uint8_t value) {
  if (syncGroup == value) return;
  putSyncGroup(value);
  syncGroup = getSyncGroup();
  addKeyToChanged("E3");  
}

#endif

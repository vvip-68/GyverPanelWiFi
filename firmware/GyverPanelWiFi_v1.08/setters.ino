// Для поддержки работы MQTT нужно знать какой параметр изменился, чтобы отправить изменения на сервер
// По этой причмне нельзя напрямую присваивать новое значение переменной, нужно выполнить дополнительный действия,
// чтобы зафиксировать изменение значения. Для этой цели данная страница содержит функции - сеттеры, 
// устанавливающие значения переменных.
// Ниже представлена таблица списка параметров состояния и какие переменные оказывают вдияние

/*
// Константы - не изменяются во время работы скетча
W  WIDTH
H  HEIGHT 
WZ USE_WEATHER
TM USE_TM1637
S1 ALARM_SOUND_LIST
S2 DAWN_SOUND_LIST
LE EFFECT_LIST
QZ USE_MQTT

PS isTurnedOff
BR isNightClock, nightClockBrightness, globalBrightness
DM manualMode
PD autoplayTime
IT idleTime
AL isAlarming, isPlayAlarmSound, isAlarmStopped
RM useRandomSequence
PW CURRENT_LIMIT
WU useWeather
WT SYNC_WEATHER_PERIOD
WR regionID
WS regionID2
WC useTemperatureColor
WN useTemperatureColorNight
W1 weather
W2 temperature

EN effect, effect_name
UE effect, getEffectUsage(effect) 
UT effect, getEffectTextOverlayUsage(effect)
UC effect, getEffectClockOverlayUsage(effect)
SE effect, getEffectSpeed(effect)
BE effect, effectContrast[effect]
SS effect, getParamForMode(effect)
SQ effect, getParam2ForMode(effect)

TE textOverlayEnabled
TI TEXT_INTERVAL
CT COLOR_TEXT_MODE
ST textScrollSpeed
C1 globalClockColor
C2 globalTextColor
OM memoryAvail
TS textLines[]

CE clockOverlayEnabled
CC COLOR_MODE
CL drawColor
CO CLOCK_ORIENT
CK CLOCK_SIZE
NB nightClockBrightness
NC nightClockColor
SC clockScrollSpeed
DC showDateInClock
DD showDateDuration
DI showDateInterval
NP useNtp
NT SYNC_TIME_PERIOD
NZ timeZoneOffset
NS ntpServerName
DW showWeatherInClock
OF needTurnOffClock
AD dawnDuration
AW alarmWeekDay
AT alarmHour[] alarmMinute[]
AE alarmEffect
MX isDfPlayerOk
MU useAlarmSound
MD alarmDuration
MV maxAlarmVolume
MA alarmSound
MB dawnSoun
MP soundFolder, soundFile
AU useSoftAP
AN apName
AA apPass
NW ssid
NA pass
IP wifi_connected, IP_STA[]
AM1T AM1_hour, AM1_minute
AM1A AM1_effect_id
AM2T AM2_hour, AM2_minute
AM2A AM2_effect_id
AM3T AM3_hour, AM3_minute
AM3A AM3_effect_id
AM4T AM4_hour, AM4_minute
AM4A AM4_effect_id
SD isSdCardReady
QA useMQTT
QP mqtt_port
QS mqtt_server
QU mqtt_user
QW mqtt_pass
QD mqtt_send_delay
QR mqtt_prefix
QK mqtt_state_packet
*/

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
}

// BR isNightClock
void set_isNightClock(bool value) {
  if (isNightClock == value) return;
  isNightClock = value;
  addKeyToChanged("BR");
}

// BR globalBrightness
void set_globalBrightness(byte value) {
  if (globalBrightness == value) return;
  putMaxBrightness(value);
  globalBrightness = getMaxBrightness();
  addKeyToChanged("BR");
}

// BR specialBrightness
void set_specialBrightness(byte value) {
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
void set_useWeather(byte value) {
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

  valid = value >= 0 && value < MAX_EFFECT;

  bool old_UE, old_UT, old_UC;
  byte old_SE, old_BE, old_SS, old_SQ;

  if (valid) {
    old_UE = getEffectUsage(thisMode); 
    old_UT = getEffectTextOverlayUsage(thisMode);
    old_UC = getEffectClockOverlayUsage(thisMode);
    old_SE = getEffectSpeed(thisMode);
    old_BE = effectContrast[thisMode];
    old_SS = effectScaleParam[thisMode];
    old_SQ = effectScaleParam2[thisMode];
  }

  thisMode = value;
  effect_name = valid ? getEffectName(value) : "";

  addKeyToChanged("EF");
  addKeyToChanged("EN");

  if (value < 0 || (valid && old_UE != getEffectUsage(value)))             addKeyToChanged("UE");
  if (value < 0 || (valid && old_UT != getEffectTextOverlayUsage(value)))  addKeyToChanged("UT");
  if (value < 0 || (valid && old_UC != getEffectClockOverlayUsage(value))) addKeyToChanged("UC");
  if (value < 0 || (valid && old_SE != getEffectSpeed(value)))             addKeyToChanged("SE");
  if (value < 0 || (valid && old_BE != effectContrast[value]))             addKeyToChanged("BE");
  if (value < 0 || (valid && old_SS != effectScaleParam[value]))           addKeyToChanged("SS");
  if (value < 0 || (valid && old_SQ != effectScaleParam2[value]))          addKeyToChanged("SQ");
}

// UE
void set_EffectUsage(byte effect, bool value) {
  bool valid = value >= 0 && value < MAX_EFFECT;
  if (!valid) return;
  bool old_value = getEffectUsage(effect);
  if (old_value != value) {
    putEffectUsage(effect, value);
    if (effect == thisMode) addKeyToChanged("UE");
  }
}

// UT 
void set_EffectTextOverlayUsage(byte effect, bool value) {
  bool valid = value >= 0 && value < MAX_EFFECT;
  if (!valid) return;
  bool old_value = getEffectTextOverlayUsage(effect);
  if (old_value != value) {
    putEffectTextOverlayUsage(effect, value);
    if (effect == thisMode) addKeyToChanged("UT");
  }
}

// UC
void set_EffectClockOverlayUsage(byte effect, bool value) {
  bool valid = value >= 0 && value < MAX_EFFECT;
  if (!valid) return;
  bool old_value = getEffectClockOverlayUsage(effect);
  if (old_value != value) {
    putEffectClockOverlayUsage(effect, value);
    if (effect == thisMode) addKeyToChanged("UC");
  }
}

// SE
void set_EffectSpeed(byte effect, byte value) {
  bool valid = value >= 0 && value < MAX_EFFECT;
  if (!valid) return;
  byte old_value = getEffectSpeed(effect);
  if (old_value != value) {
    putEffectSpeed(effect, value);
    if (effect == thisMode) addKeyToChanged("SE");
  }
}

// BE
void set_EffectContrast(byte effect, byte value) {
  bool valid = value >= 0 && value < MAX_EFFECT;
  if (!valid) return;
  byte old_value = effectContrast[effect];
  if (old_value != value) {
    effectContrast[effect] = value;
    putEffectContrast(effect, value);
    if (effect == thisMode) addKeyToChanged("BE");
  }
}

// SS
void set_EffectScaleParam(byte effect, byte value) {
  bool valid = value >= 0 && value < MAX_EFFECT;
  if (!valid) return;
  byte old_value = effectScaleParam[effect];
  if (old_value != value) {
    effectScaleParam[effect] = value;
    putScaleForEffect(effect, value);
    if (effect == thisMode) addKeyToChanged("SS");
  }
}

// SQ
void set_EffectScaleParam2(byte effect, byte value) {
  bool valid = value >= 0 && value < MAX_EFFECT;
  if (!valid) return;
  byte old_value = effectScaleParam2[effect];
  if (old_value != value) {
    effectScaleParam2[effect] = value;
    putScaleForEffect2(effect, value);
    if (effect == thisMode) addKeyToChanged("SQ");
  }
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
void set_COLOR_TEXT_MODE(byte value) {
  if (COLOR_TEXT_MODE == value) return;
  putTextColor(value);
  COLOR_TEXT_MODE = getTextColor();
  addKeyToChanged("CT");
}

// ST textScrollSpeed
void set_textScrollSpeed(byte value) {
  if (textScrollSpeed == value) return;
  putTextScrollSpeed(value);
  textScrollSpeed = getTextScrollSpeed();
  addKeyToChanged("ST");
}

// ?? globalColor
void set_globalColor(uint32_t value) {
  if (globalColor == value) return;
  putGlobalColor(value);
  globalColor = getGlobalColor();
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
  if (clockOverlayEnabled == value) return;
  putClockOverlayEnabled(value);
  clockOverlayEnabled = getClockOverlayEnabled();
  addKeyToChanged("CE");
}

// CC COLOR_MODE
void set_COLOR_MODE(byte value) {
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
void set_CLOCK_ORIENT(byte value) {
  if (CLOCK_ORIENT == value) return;
  putClockOrientation(value);
  CLOCK_ORIENT = getClockOrientation();
  addKeyToChanged("CO");
}

// CK CLOCK_SIZE
void set_CLOCK_SIZE(byte value) {
  if (CLOCK_SIZE == value) return;
  putClockSize(value);
  CLOCK_SIZE = getClockSize();
  addKeyToChanged("CK");
}

// NB nightClockBrightness
void set_nightClockBrightness(byte value) {
  if (nightClockBrightness == value) return;
  putNightClockBrightness(value);
  nightClockBrightness = getNightClockBrightness();
  addKeyToChanged("NB");
  addKeyToChanged("BR");
}

// NC nightClockColor
void set_nightClockColor(byte value) {
  if (nightClockColor == value) return;
  putNightClockColor(value);
  nightClockColor = getNightClockColor();
  addKeyToChanged("NC");
}

// SC clockScrollSpeed
void set_clockScrollSpeed(byte value) {
  if (clockScrollSpeed == value) return;
  putClockScrollSpeed(value);
  clockScrollSpeed = getClockScrollSpeed();
  addKeyToChanged("SC");
}

// DC showDateInClock
void set_showDateInClock(bool value) {
  if (showDateInClock == value) return;
  putShowDateInClock(value);
  showDateInClock = getShowDateInClock();
  addKeyToChanged("DC");
}

// DD showDateDuration
void set_showDateDuration(byte value) {
  if (showDateDuration == value) return;
  putShowDateDuration(value);
  showDateDuration = getShowDateDuration();
  addKeyToChanged("DD");
}

// DI showDateInterval
void set_showDateInterval(byte value) {
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
void set_SYNC_TIME_PERIOD(int8_t value) {
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
  if (getSsid() == value) return;
  putSsid(value);
  getSsid().toCharArray(ssid, 24);
  addKeyToChanged("NW");
}

// NA pass
void set_pass(String value) {
  if (getPass() == value) return;
  putPass(value);
  getPass().toCharArray(pass, 16);
  addKeyToChanged("NA");
}
              
// AN apName
void set_SoftAPName(String value) {
  if (getSoftAPName() == value) return;
  putSoftAPName(value);
  getSoftAPName().toCharArray(pass, 16);
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
void set_StaticIP(byte p1, byte p2, byte p3, byte p4) {
  if (IP_STA[0] == p1 && IP_STA[1] == p2 && IP_STA[2] == p3 &&IP_STA[3] == p4) return;
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
void set_dawnDuration(byte value) {
  if (dawnDuration == value) return;
  byte alarmWeekDay = getAlarmWeekDay();
  byte alarmEffect = getAlarmEffect();
  byte alarmDuration = getAlarmDuration();  
  putAlarmParams(alarmWeekDay,value,alarmEffect,alarmDuration);
  dawnDuration = getDawnDuration();
  addKeyToChanged("AD");
}

// AW alarmWeekDay
void set_alarmWeekDay(byte value) {
  if (alarmWeekDay == value) return;
  byte dawnDuration = getDawnDuration();
  byte alarmEffect = getAlarmEffect();
  byte alarmDuration = getAlarmDuration();  
  putAlarmParams(value,dawnDuration,alarmEffect,alarmDuration);
  alarmWeekDay = getAlarmWeekDay();
  addKeyToChanged("AW");
}

// AE alarmEffect
void set_alarmEffect(byte value) {
  if (alarmEffect == value) return;
  byte alarmWeekDay = getAlarmWeekDay();
  byte dawnDuration = getDawnDuration();
  byte alarmDuration = getAlarmDuration();  
  putAlarmParams(alarmWeekDay,dawnDuration,value,alarmDuration);
  alarmEffect = getAlarmEffect();
  addKeyToChanged("AE");
}

// MD alarmDuration
void set_alarmDuration(byte value) {
  if (alarmDuration == value) return;
  byte alarmWeekDay = getAlarmWeekDay();
  byte dawnDuration = getDawnDuration();
  byte alarmEffect = getAlarmEffect();
  putAlarmParams(alarmWeekDay,dawnDuration,alarmEffect,value);
  alarmDuration = getAlarmDuration();  
  addKeyToChanged("MD");
}

// AT alarmHour[], alarmMinute[]
void set_alarmTime(byte wd, byte hour_value, byte minute_value) {
  byte old_hour   = getAlarmHour(wd);
  byte old_minute = getAlarmMinute(wd);
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
  byte alarmSound = getAlarmSound();
  byte dawnSound = getDawnSound();
  byte maxAlarmVolume = getMaxAlarmVolume();
  putAlarmSounds(value, maxAlarmVolume, alarmSound, dawnSound);
  useAlarmSound = getUseAlarmSound();
  addKeyToChanged("MU");
}

// MV maxAlarmVolume
void set_maxAlarmVolume(bool value) {
  if (maxAlarmVolume == value) return;  
  bool useAlarmSound = getUseAlarmSound();
  byte alarmSound = getAlarmSound();
  byte dawnSound = getDawnSound();
  putAlarmSounds(useAlarmSound, value, alarmSound, dawnSound);
  maxAlarmVolume = getMaxAlarmVolume();
  addKeyToChanged("MV");
}

// MA alarmSound
void set_alarmSound(bool value) {
  if (alarmSound == value) return;  
  bool useAlarmSound = getUseAlarmSound();
  byte dawnSound = getDawnSound();
  byte maxAlarmVolume = getMaxAlarmVolume();
  putAlarmSounds(useAlarmSound, maxAlarmVolume, value, dawnSound);
  alarmSound = getAlarmSound();
  addKeyToChanged("MA");
}

// MB dawnSound
void set_dawnSound(bool value) {
  if (dawnSound == value) return;  
  bool useAlarmSound = getUseAlarmSound();
  byte alarmSound = getAlarmSound();
  byte maxAlarmVolume = getMaxAlarmVolume();
  putAlarmSounds(useAlarmSound, maxAlarmVolume, alarmSound, value);
  dawnSound = getDawnSound();
  addKeyToChanged("MB");
}

// MP soundFolder
void set_soundFolder(byte value) {
  if (soundFolder == value) return;  
  soundFolder = value;
  addKeyToChanged("MP");
}

// MP soundFile
void set_soundFile(byte value) {
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
void set_AM1_hour(byte value) {
  if (AM1_hour == value) return;
  putAM1hour(value);
  AM1_hour = getAM1hour();
  addKeyToChanged("AM1T");
}

// AM1T AM1_minute
void set_AM1_minute(byte value) {
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
void set_AM2_hour(byte value) {
  if (AM2_hour == value) return;
  putAM2hour(value);
  AM2_hour = getAM2hour();
  addKeyToChanged("AM2T");
}

// AM2T AM2_minute
void set_AM2_minute(byte value) {
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
void set_AM3_hour(byte value) {
  if (AM3_hour == value) return;
  putAM3hour(value);
  AM3_hour = getAM3hour();
  addKeyToChanged("AM3T");
}

// AM3T AM3_minute
void set_AM3_minute(byte value) {
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
void set_AM4_hour(byte value) {
  if (AM4_hour == value) return;
  putAM4hour(value);
  AM4_hour = getAM4hour();
  addKeyToChanged("AM4T");
}

// AM4T AM4_minute
void set_AM4_minute(byte value) {
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
  getMqttServer().toCharArray(mqtt_server, 24);
  addKeyToChanged("QS");
}

// QU mqtt_user
void set_MqttUser(String value) {
  if (getMqttUser() == value) return;
  putMqttUser(value);
  getMqttUser().toCharArray(mqtt_user, 14);
  addKeyToChanged("QU");
}

// QW mqtt_pass
void set_MqttPass(String value) {
  if (getMqttPass() == value) return;
  putMqttPass(value);
  getMqttPass().toCharArray(mqtt_pass, 14);
  addKeyToChanged("QW");
}

// QD mqtt_send_delay
void set_mqtt_send_delay(int16_t value) {
  if (mqtt_send_delay == value) return;  
  putMqttSendDelay(value);
  mqtt_send_delay = getMqttSendDelay();
  addKeyToChanged("QD");
}

// QR mqtt_prefix
void set_MqttPrefix(String value) {
  if (getMqttPrefix() == value) return;
  putMqttPrefix(value);
  getMqttPrefix().toCharArray(mqtt_prefix, 30);
  addKeyToChanged("QR");
}

// QK mqtt_state_packet
void set_mqtt_state_packet(bool value) {
  if (mqtt_state_packet == value) return;  
  putSendStateInPacket(value);
  mqtt_state_packet = getSendStateInPacket();
  addKeyToChanged("QK");
}
#endif

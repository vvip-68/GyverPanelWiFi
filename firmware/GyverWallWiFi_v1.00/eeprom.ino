#define EEPROM_OK 0xA5                     // Флаг, показывающий, что EEPROM инициализирована корректными данными 
#define EFFECT_EEPROM 200                  // начальная ячейка eeprom с параметрами эффектов

void loadSettings() {

  // Адреса в EEPROM:
  //    0 - если EEPROM_OK - EEPROM инициализировано, если другое значение - нет 
  //    1 - максимальная яркость ленты 1-255
  //    2 - автосмена режима в демо: вкл/выкл
  //    3 - время автосмены режимов
  //    4 - время бездействия до переключения в авторежим
  //    5 - использовать синхронизацию времени через NTP
  //  6,7 - период синхронизации NTP (int16_t - 2 байта)
  //    8 - time zone
  //    9 - выключать индикатор часов при выключении лампы true - выключать / false - не выключать
  //   10 - IP[0]
  //   11 - IP[1]
  //   12 - IP[2]
  //   13 - IP[3]
  //   14 - Использовать режим точки доступа
  //   15 - ориентация часов горизонтально / веритикально
  //***16 - не используется
  //   17 - globalColor.r
  //   18 - globalColor.g
  //   19 - globalColor.b
  //   20 - Будильник, дни недели
  //   21 - Будильник, продолжительность "рассвета"
  //   22 - Будильник, эффект "рассвета"
  //   23 - Будильник, использовать звук
  //   24 - Будильник, играть звук N минут после срабатывания
  //   25 - Будильник, Номер мелодии будильника (из папки 01 на SD карте)
  //   26 - Будильник, Номер мелодии рассвета (из папки 02 на SD карте) 
  //   27 - Будильник, Максимальная громкость будильника
  //   28 - Номер последнего активного спец-режима или -1, если были включены обычные эффекты
  //   29 - Номер последнего активированного вручную режима
  //   30 - Отображать часы в режимах
  //   31 - Использовать случайную последовательность в демо-режиме
  //   32 - Формат часов в бегущей строке 0 - только часы; 1 - часы и дата коротко; 2 - часы и дата строкой  
  //   33 - Режим 1 по времени - часы
  //   34 - Режим 1 по времени - минуты
  //   35 - Режим 1 по времени - ID эффекта или -1 - выключено; 0 - случайный;
  //   36 - Режим 2 по времени - часы
  //   37 - Режим 2 по тайвременимеру - минуты
  //   38 - Режим 2 по времени - ID эффекта или -1 - выключено; 0 - случайный;
  //   39 - Цвет ночных часов:  0 - R; 1 - G; 2 - B; 3 - C; 4 - M; 5 - Y; 6 - W;
  //   40 - Будильник, время: понедельник : часы
  //   41 - Будильник, время: понедельник : минуты
  //   42 - Будильник, время: вторник : часы
  //   43 - Будильник, время: вторник : минуты
  //   44 - Будильник, время: среда : часы
  //   45 - Будильник, время: среда : минуты
  //   46 - Будильник, время: четверг : часы
  //   47 - Будильник, время: четверг : минуты
  //   48 - Будильник, время: пятница : часы
  //   49 - Будильник, время: пятница : минуты
  //   50 - Будильник, время: суббота : часы
  //   51 - Будильник, время: суббота : минуты
  //   52 - Будильник, время: воскресенье : часы
  //   53 - Будильник, время: воскресенье : минуты
  //  54-63   - имя точки доступа    - 10 байт
  //  64-79   - пароль точки доступа - 16 байт
  //  80-103  - имя сети  WiFi       - 24 байта
  //  104-119 - пароль сети  WiFi    - 16 байт
  //  120-149 - имя NTP сервера      - 30 байт
  //  150,151 - лимит по току
  //  152 - globalClockColor.r
  //  153 - globalClockColor.g
  //  154 - globalClockColor.b
  //  155 - globalTextColor.r
  //  156 - globalTextColor.g
  //  157 - globalTextColor.b
  //**158 - не используется
  //  ...
  //**199 - не используется
  //  200 - 200+(Nэфф*3)   - скорость эффекта
  //  201 - 200+(Nэфф*3)+1 - специальный параметр эффекта
  //  202 - 200+(Nэфф*3)+2 - эффект в авторежиме: 1 - использовать; 0 - не использовать

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

    autoplayTime = getAutoplayTime();
    idleTime = getIdleTime();    

    useNtp = getUseNtp();
    overlayEnabled = getClockOverlayEnabled();
    timeZoneOffset = getTimeZone();

    SYNC_TIME_PERIOD = getNtpSyncTime();
    AUTOPLAY = getAutoplay();
    CLOCK_ORIENT = getClockOrientation();
    COLOR_MODE = getScaleForEffect(MC_CLOCK);
    COLOR_TEXT_MODE = getScaleForEffect(MC_TEXT);
    CURRENT_LIMIT = getPowerLimit();
    useRandomSequence = getRandomMode();
    formatClock = getFormatClock();
    nightClockColor = getNightClockColor();
    
    alarmWeekDay = getAlarmWeekDay();
    alarmEffect = getAlarmEffect();
    alarmDuration = getAlarmDuration();

    needTurnOffClock = getTurnOffClockOnLampOff();

    for (byte i=0; i<7; i++) {
      alarmHour[i] = getAlarmHour(i+1);
      alarmMinute[i] = getAlarmMinute(i+1);
    }
 
    for (byte i=0; i<MAX_EFFECT; i++) {
      effectScaleParam[i] = getScaleForEffect(i);
    }

    dawnDuration = getDawnDuration();

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

    AM1_hour = getAM1hour();
    AM1_minute = getAM1minute();
    AM1_effect_id = getAM1effect();
    AM2_hour = getAM2hour();
    AM2_minute = getAM2minute();
    AM2_effect_id = getAM2effect();

    loadStaticIP();
    
  } else {
    globalBrightness = BRIGHTNESS;

    autoplayTime = ((long)AUTOPLAY_PERIOD * 1000L);     // секунды -> миллисек
    idleTime = ((long)IDLE_TIME * 60L * 1000L);         // минуты -> миллисек
    overlayEnabled = true;

    useNtp = true;
    CLOCK_ORIENT = 0;
    COLOR_MODE = 0;

    AUTOPLAY = true;
    SYNC_TIME_PERIOD = 60;

    alarmWeekDay = 0;
    dawnDuration = 20;
    alarmEffect = MC_DAWN_ALARM;
    useSoftAP = false;
    
    #if (USE_MP3 == 1)
    useAlarmSound = false;
    alarmDuration = 1;
    alarmSound = 1;
    dawnSound = 1;
    maxAlarmVolume = 30;
    #endif
    
    needTurnOffClock = false;

    useRandomSequence = true;
    formatClock = 0;

    for (byte i=0; i<MAX_EFFECT; i++) {
      effectScaleParam[i] = 50;
    }

    AM1_hour = 0;
    AM1_minute = 0;
    AM1_effect_id = -5;
    AM2_hour = 0;
    AM2_minute = 0;
    AM2_effect_id = -5;    
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

  EEPROMwrite(1, globalBrightness);

  EEPROMwrite(2, AUTOPLAY ? 1 : 0);
  EEPROMwrite(3, autoplayTime / 1000L);
  EEPROMwrite(4, constrain(idleTime / 60L / 1000L, 0, 255));
  
  EEPROMwrite(5, useNtp ? 1 : 0);
  EEPROM_int_write(6, SYNC_TIME_PERIOD);
  EEPROMwrite(8, (byte)timeZoneOffset);
  
  EEPROMwrite(9, false);

  EEPROMwrite(10, IP_STA[0]);
  EEPROMwrite(11, IP_STA[1]);
  EEPROMwrite(12, IP_STA[2]);
  EEPROMwrite(13, IP_STA[3]);

  EEPROMwrite(14, 0);    // Использовать режим точки доступа: 0 - нет; 1 - да
  
  saveAlarmParams(alarmWeekDay,dawnDuration,alarmEffect,alarmDuration);
  for (byte i=0; i<7; i++) {
      setAlarmTime(i+1, alarmHour[i], alarmMinute[i]);
  }

  EEPROMwrite(32, formatClock);         // Формат отображения часов в бегущей строке: 0 - только часы: 1 - часы и дата кратко; 2 - часы и полная дата;
  EEPROMwrite(39, nightClockColor);     // Цвет ночных часов
  
  EEPROMwrite(28, (byte)-1);            // Текущий спец-режим - это не спец-режим
  EEPROMwrite(33, AM1_hour);            // Режим 1 по времени - часы
  EEPROMwrite(34, AM1_minute);          // Режим 1 по времени - минуты
  EEPROMwrite(35, (byte)AM1_effect_id); // Режим 1 по времени - действие: -3 - выключено (не используется); -2 - выключить матрицу (черный экран); -1 - огонь, 0 - случайный, 1 и далее - эффект EFFECT_LIST
  EEPROMwrite(36, AM2_hour);            // Режим 2 по времени - часы
  EEPROMwrite(37, AM2_minute);          // Режим 2 по времени - минуты
  EEPROMwrite(38, (byte)AM2_effect_id); // Режим 2 по времени - действие: -3 - выключено (не используется); -2 - выключить матрицу (черный экран); -1 - огонь, 0 - случайный, 1 и далее - эффект EFFECT_LIST
  
  // Настройки по умолчанию для эффектов
  for (int i = 0; i < MAX_EFFECT; i++) {
    saveEffectParams(i, effectSpeed, 50, true);
  }
    
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

  EEPROMwrite(31, useRandomSequence ? 1 : 0);

  setPowerLimit(CURRENT_LIMIT);
  
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

void saveEffectParams(byte effect, int speed, byte value, boolean use) {
  const int addr = EFFECT_EEPROM;  
  EEPROMwrite(addr + effect*3, constrain(map(speed, D_EFFECT_SPEED_MIN, D_EFFECT_SPEED_MAX, 0, 255), 0, 255));        // Скорость эффекта
  EEPROMwrite(addr + effect*3 + 1, value);                       // Параметр эффекта  
  EEPROMwrite(addr + effect*3 + 2, use ? 1 : 0);                 // По умолчанию эффект доступен в демо-режиме
  effectScaleParam[effect] = value;
}

void saveEffectSpeed(byte effect, int speed) {
  if (speed != getEffectSpeed(effect)) {
    const int addr = EFFECT_EEPROM;  
    EEPROMwrite(addr + effect*3, constrain(map(speed, D_EFFECT_SPEED_MIN, D_EFFECT_SPEED_MAX, 0, 255), 0, 255));        // Скорость эффекта
  }
}

byte getEffectSpeed(byte effect) {
  const int addr = EFFECT_EEPROM;
  return map(EEPROMread(addr + effect*3),0,255,D_EFFECT_SPEED_MIN,D_EFFECT_SPEED_MAX);
}

void saveEffectUsage(byte effect, boolean use) {
  if (use != getEffectUsage(effect)) {
    const int addr = EFFECT_EEPROM;  
    EEPROMwrite(addr + effect*3 + 2, use ? 1 : 0);             // По умолчанию оверлей часов для эффекта отключен  
  }
}

boolean getEffectUsage(byte effect) {
  const int addr = EFFECT_EEPROM;
  return EEPROMread(addr + effect*3 + 2) == 1;
}

void setScaleForEffect(byte effect, byte value) {
  if (value != getScaleForEffect(effect)) {
    const int addr = EFFECT_EEPROM;
    EEPROMwrite(addr + effect*3 + 1, value);
    effectScaleParam[effect] = value;
  }  
}

byte getScaleForEffect(byte effect) {
  const int addr = EFFECT_EEPROM;
  byte value = EEPROMread(addr + effect*3 + 1);
  effectScaleParam[effect] = value;
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
  if (time == 0) time = ((long)AUTOPLAY_PERIOD * 1000);
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
  r = EEPROMread(17);
  g = EEPROMread(18);
  b = EEPROMread(19);
  return (uint32_t)r<<16 | (uint32_t)g<<8 | (uint32_t)b;
}

void setGlobalColor(uint32_t color) {
  globalColor = color;
  if (color != getGlobalColor()) {
    CRGB cl = CRGB(color);
    EEPROMwrite(17, cl.r); // R
    EEPROMwrite(18, cl.g); // G
    EEPROMwrite(19, cl.b); // B
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

bool getTurnOffClockOnLampOff() {
  return EEPROMread(9) == 1;
}

void setTurnOffClockOnLampOff(bool flag) {
  if (flag != getTurnOffClockOnLampOff()) {
    EEPROMwrite(9, flag ? 1 : 0);
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

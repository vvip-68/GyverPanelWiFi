void InitializeDfPlayer1() {
#if (USE_MP3 == 1)
  DEBUGLN(F("\nИнициализация MP3-плеера..."));
  isDfPlayerOk = true;
  #if defined(ESP32)
    mp3Serial.begin(9600);
  #else  
    mp3Serial.begin(9600, SWSERIAL_8N1, SRX, STX);
  #endif
  mp3Serial.setTimeout(1000);
  
  dfPlayer.begin(9600);
  delay(1000);  
  // Попытка СЧИТАТЬ данные с плеера, чтобы если плеера нет - получить ошибку, т.к.
  // при отправке команд в плеер библиотека плеера не ждет ответа (подтверждения выполнения команды)
  // и нет возможности выяснить есть ли плеер на самом деле.
  uint32_t ms1 = millis();    
  dfPlayer.getVolume(); delay(GUARD_DELAY); 
  uint32_t ms2 = millis();      
  if (ms2 - ms1 > 5000) {
    set_isDfPlayerOk(false);  
  } else {
    dfPlayer.setPlaybackSource(DfMp3_PlaySource_Sd); delay(GUARD_DELAY);
    dfPlayer.setEq(DfMp3_Eq_Normal);                 delay(GUARD_DELAY);
    dfPlayer.setVolume(1);                           delay(GUARD_DELAY);
  }
#endif  
}

void InitializeDfPlayer2() {    
#if (USE_MP3 == 1)
  refreshDfPlayerFiles();    
  DEBUGLN(String(F("Звуков будильника найдено: ")) + String(alarmSoundsCount));
  DEBUGLN(String(F("Звуков рассвета найдено: ")) + String(dawnSoundsCount));
  DEBUGLN(String(F("Звуков сообщений найдено: ")) + String(noteSoundsCount));
  set_isDfPlayerOk(alarmSoundsCount + dawnSoundsCount + noteSoundsCount > 0);
#else  
  set_isDfPlayerOk(false);
#endif  
}

#if (USE_MP3 == 1)

void refreshDfPlayerFiles() {
  // Чтение почему-то не всегда работает, иногда возвращает 0 или число от какого-то предыдущего запроса
  // Для того, чтобы наверняка считать значение - первое прочитанное игнорируем, потом читаем несколько раз до повторения.
  DEBUGLN(F("Поиск файлов на карте плеера..."));
  // Папка с файлами для будильника
  int16_t cnt = 0, val = 0, new_val = 0; 
  do {
    // Если запрос выполнялся более 5 секунд - DFPlayer не ответил - он неработоспособен
    // Выполнять чтение дальше смысла нет - это только "завешивает" прошивку.
    mp3Serial.setTimeout(1000);
    uint32_t ms1 = millis();    
    val = dfPlayer.getFolderTrackCount(1);       delay(GUARD_DELAY);
    uint32_t ms2 = millis();  
    // Плеер не ответил за отведенное время? - Завершить процедуру опроса.      
    if (ms2 - ms1 > 1000) return;
    new_val = dfPlayer.getFolderTrackCount(1);   delay(GUARD_DELAY);    
    if (val == new_val && val != 0) break;
    cnt++;
  } while ((val == 0 || new_val == 0 || val != new_val) && cnt < 3);
  alarmSoundsCount = val < 0 ? 0 : val;
  
  // Папка с файлами для рассвета
  cnt = 0, val = 0, new_val = 0; 
  do {
    val = dfPlayer.getFolderTrackCount(2);       delay(GUARD_DELAY);
    new_val = dfPlayer.getFolderTrackCount(2);   delay(GUARD_DELAY);     
    if (val == new_val && val != 0) break;
    cnt++;
  } while ((val == 0 || new_val == 0 || val != new_val) && cnt < 3);    
  dawnSoundsCount = val < 0 ? 0 : val;

  // Папка с файлами для звуков в бегущей строке
  cnt = 0, val = 0, new_val = 0; 
  do {
    val = dfPlayer.getFolderTrackCount(3);       delay(GUARD_DELAY);
    new_val = dfPlayer.getFolderTrackCount(3);   delay(GUARD_DELAY);     
    if (val == new_val && val != 0) break;
    cnt++;
  } while ((val == 0 || new_val == 0 || val != new_val) && cnt < 3);    
  noteSoundsCount = val < 0 ? 0 : val;

  DEBUGLN();  
}
#endif

void PlayAlarmSound() {
  
  if (!isDfPlayerOk) return;

  #if (USE_MP3 == 1)  
  int8_t sound = alarmSound;
  // Звук будильника - случайный?
  if (sound == 0) {
    sound = random8(1, alarmSoundsCount);     // -1 - нет звука; 0 - случайный; 1..alarmSoundsCount - звук
  }
  // Установлен корректный звук?
  if (sound > 0) {
    dfPlayer.stop();                                    delay(GUARD_DELAY);  // Без этих задержек между вызовами функция dfPlayer приложение крашится.
    dfPlayer.setVolume(constrain(maxAlarmVolume,1,30)); delay(GUARD_DELAY);
    dfPlayer.playFolderTrack(1, sound);                 delay(GUARD_DELAY);
    dfPlayer.setRepeatPlayCurrentTrack(true);           delay(GUARD_DELAY);    
    alarmSoundTimer.setInterval(alarmDuration * 60L * 1000L);
    alarmSoundTimer.reset();
    set_isPlayAlarmSound(true);
  } else {
    // Звука будильника нет - плавно выключить звук рассвета
    StopSound(1000);
  }
  #endif
}

void PlayDawnSound() {
  if (!isDfPlayerOk) return;

  #if (USE_MP3 == 1)
  // Звук рассвета отключен?
  int8_t sound = dawnSound;
  // Звук рассвета - случайный?
  if (sound == 0) {
    sound = random8(1, dawnSoundsCount);     // -1 - нет звука; 0 - случайный; 1..alarmSoundsCount - звук
  }
  // Установлен корректный звук?
  if (sound > 0) {
    dfPlayer.stop();                          delay(GUARD_DELAY); // Без этих задержек между вызовами функция dfPlayer приложение крашится.
    dfPlayer.setVolume(1);                    delay(GUARD_DELAY);
    dfPlayer.playFolderTrack(2, sound);       delay(GUARD_DELAY);
    dfPlayer.setRepeatPlayCurrentTrack(true); delay(GUARD_DELAY);
    // Установить время приращения громкости звука - от 1 до maxAlarmVolume за время продолжительности рассвета realDawnDuration
    fadeSoundDirection = 1;   
    fadeSoundStepCounter = maxAlarmVolume;
    if (fadeSoundStepCounter <= 0) fadeSoundStepCounter = 1;
    fadeSoundTimer.setInterval(realDawnDuration * 60L * 1000L / fadeSoundStepCounter);
    alarmSoundTimer.stopTimer();
  } else {
    StopSound(1000);
  }
  #endif
}

void StopSound(int16_t duration) {

  if (!isDfPlayerOk) return;

  #if (USE_MP3 == 1)
  
  set_isPlayAlarmSound(false);

  if (duration <= 0) {
    dfPlayer.stop();       delay(GUARD_DELAY);
    dfPlayer.setVolume(0); delay(GUARD_DELAY);
    return;
  }
  
  // Чтение текущего уровня звука часто глючит и возвращает 0. Тогда использовать maxAlarmVolume
  fadeSoundStepCounter = dfPlayer.getVolume(); delay(GUARD_DELAY);
  if (fadeSoundStepCounter <= 0) fadeSoundStepCounter = maxAlarmVolume;
  if (fadeSoundStepCounter <= 0) fadeSoundStepCounter = 1;
    
  fadeSoundDirection = -1;     
  fadeSoundTimer.setInterval(duration / fadeSoundStepCounter);  
  
  #endif
}

#if (USE_MP3 == 1)

class Mp3Notify {
public:
  static void PrintlnSourceAction(DfMp3_PlaySources source, const char* action)
  {
    if (source & DfMp3_PlaySources_Sd) 
    {
        DEBUG(F("SD-карта, "));
    }
    if (source & DfMp3_PlaySources_Usb) 
    {
        DEBUG(F("USB диск, "));
    }
    if (source & DfMp3_PlaySources_Flash) 
    {
        DEBUG(F("Flash-память, "));
    }
    DEBUGLN(action);
  }
  static void printErrorDetail(uint8_t type){
    DEBUG(F("DFPlayerError: "));
    switch (type) {
      case DfMp3_Error_RxTimeout:
        DEBUGLN(F("Таймаут!"));
        break;
      case DfMp3_Error_SerialWrongStack:
        DEBUGLN(F("Ошибка стека!"));
        break;
      case DfMp3_Error_Busy:
        DEBUGLN(F("Нет карты"));
        break;
      case DfMp3_Error_Sleeping:
        DEBUGLN(F("Ожидание..."));
        break;
      case DfMp3_Error_CheckSumNotMatch:
        DEBUGLN(F("Ошибка контрольной суммы"));
        break;
      case DfMp3_Error_FileIndexOut:
        DEBUGLN(F("Неверный индекс файла"));
        break;
      case DfMp3_Error_FileMismatch:
        DEBUGLN(F("Файл не найден"));
        break;
      case DfMp3_Error_Advertise:
        DEBUGLN(F("Реклама"));
        break;
      case DfMp3_Error_PacketSize:
        DEBUGLN(F("Неверный размер пакета команды"));
        break;
      case DfMp3_Error_PacketHeader:
        DEBUGLN(F("Неверный заголовок пакета команды"));
        break;
      case DfMp3_Error_PacketChecksum:
        DEBUGLN(F("Ошибка CRC пакета команды"));
        break;
      default:
        DEBUGLN(F("Что-то пошло не так..."));
        break;
    }
  }  
  static void OnError(DfMp3& mp3, uint16_t errorCode)
  {
    // see DfMp3_Error for code meaning
    printErrorDetail(errorCode);
  }
  static void OnPlayFinished(DfMp3& mp3, DfMp3_PlaySources source, uint16_t track)
  {
    DEBUG(F("Трек завершен #"));
    DEBUGLN(track);  
    if (!(isAlarming || isPlayAlarmSound) && soundFolder == 0 && soundFile == 0 && runTextSound <= 0) {
      dfPlayer.stop(); delay(GUARD_DELAY);
    } else
    // Перезапустить звук, если установлен его повтор
    if (runTextSound > 0 && runTextSoundRepeat) {
      dfPlayer.playFolderTrack(3, runTextSound); delay(GUARD_DELAY);
    }    
  }
  static void OnPlaySourceOnline(DfMp3& mp3, DfMp3_PlaySources source)
  {
    PrintlnSourceAction(source, "готова");
    InitializeDfPlayer2();
    if (!isDfPlayerOk) DEBUGLN(F("MP3 плеер недоступен."));    
  }
  static void OnPlaySourceInserted(DfMp3& mp3, DfMp3_PlaySources source)
  {
    PrintlnSourceAction(source, "вставлена");
    InitializeDfPlayer2();
    if (!isDfPlayerOk) DEBUGLN(F("MP3 плеер недоступен."));    
  }
  static void OnPlaySourceRemoved(DfMp3& mp3, DfMp3_PlaySources source)
  {
    PrintlnSourceAction(source, "удалена");
    // Карточка "отвалилась" - делаем недоступным все что связано с MP3 плеером
    set_isDfPlayerOk(false);
    alarmSoundsCount = 0;
    dawnSoundsCount = 0;
    noteSoundsCount = 0;
    DEBUGLN(F("MP3 плеер недоступен."));    
  }
};
#endif
